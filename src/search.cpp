#include <search.hpp>

//Per-ply move buffers, reused across calls instead of allocating a fresh vector
//per node. Indexed by ply rather than shared globally because negamax/quiescence
//recurse - a parent at ply P is still iterating its own moves while a child at
//ply P+1 is generating its own, so each ply needs a distinct slot. negamax's ply
//parameter starts at 1 (see findBestMove), so slot 0 is free for the root's own
//move list. negamax and quiescence's in-check branch never hold a buffer at the
//same ply simultaneously (depth==0 in negamax hands off to quiescence before
//touching move_buffers[ply]), so they safely share move_buffers; quiescence's
//non-check branch uses capture_buffers instead, populated directly by
//generateLegalCaptures rather than filtered down from move_buffers - a separate
//array isn't strictly required anymore, but keeps the two call sites from
//stepping on each other's slot at the same ply.
constexpr int MAX_PLY = 128;
static std::vector<Move> move_buffers[MAX_PLY];
static std::vector<Move> capture_buffers[MAX_PLY];

uint64_t nodes_searched = 0;

void resetNodeCount() {
    nodes_searched = 0;
}

//Time management: time_limited/search_deadline are set once per findBestMove
//call; search_aborted latches true the first time checkTimeBudget notices the
//deadline has passed. Every negamax/quiescence call checks it - both at entry
//and right after each recursive call - and bails out immediately once it's
//set, propagating up via ordinary return values rather than exceptions. Since
//makeMove/unmakeMove (and the null-move equivalents) are always paired
//synchronously around the recursive call, this unwinds the board cleanly with
//no extra bookkeeping. The bailout must happen before any storeTT or
//alpha/best_move update, so a garbage score from a cut-off subtree can never
//pollute the transposition table or bias a parent's move choice.
static bool time_limited = false;
static std::chrono::steady_clock::time_point search_deadline;
static bool search_aborted = false;

constexpr uint64_t TIME_CHECK_INTERVAL = 2048; //power of 2, checked via bitmask

static void checkTimeBudget() {
    if(!time_limited || search_aborted) {
        return;
    }

    if((nodes_searched & (TIME_CHECK_INTERVAL - 1)) != 0) {
        return;
    }

    if(std::chrono::steady_clock::now() >= search_deadline) {
        search_aborted = true;
    }
}

//Killer moves: 2 per ply, only quiet (non-capture) moves that caused a beta
//cutoff are ever stored - captures already sort well under MVV-LVA. History:
//[color][from][to], accumulated by depth^2 on a quiet cutoff, persists across
//a game (see clearHistory), with a halve-everything guard against overflow.
static Move killer_moves[MAX_PLY][2];
static int history_table[2][64][64];

constexpr int HISTORY_MAX = 1 << 20;

void clearKillers() {
    for(int p = 0; p < MAX_PLY; ++p) {
        killer_moves[p][0] = Move{-1, -1, -1, 0};
        killer_moves[p][1] = Move{-1, -1, -1, 0};
    }
}

void clearHistory() {
    for(int c = 0; c < 2; ++c) {
        for(int f = 0; f < 64; ++f) {
            for(int t = 0; t < 64; ++t) {
                history_table[c][f][t] = 0;
            }
        }
    }
}

static void recordKillerMove(int ply, const Move& move) {
    const Move& existing = killer_moves[ply][0];
    if(existing.start == move.start && existing.end == move.end && existing.promotion == move.promotion) {
        return;
    }

    killer_moves[ply][1] = killer_moves[ply][0];
    killer_moves[ply][0] = move;
}

static void recordHistoryScore(int color, int from, int to, int depth) {
    int bonus = depth * depth;

    if(history_table[color][from][to] + bonus > HISTORY_MAX) {
        for(int c = 0; c < 2; ++c) {
            for(int f = 0; f < 64; ++f) {
                for(int t = 0; t < 64; ++t) {
                    history_table[c][f][t] /= 2;
                }
            }
        }
    }

    history_table[color][from][to] += bonus;
}

static bool hasNonPawnMaterial(const Board& board, int color) {
    return (board.bitboards[KNIGHT][color] | board.bitboards[BISHOP][color]
          | board.bitboards[ROOK][color] | board.bitboards[QUEEN][color]) != 0ULL;
}

void reorderLegalMoves(Board& board, std::vector<Move>& legalMoves, int ply) {
    constexpr int values[6] = {100, 290, 310, 500, 900, 0};
    constexpr int MAX_MOVES = 256;

    //Non-overlapping score bands: captures always outrank killers/history
    //regardless of how large the history table grows.
    constexpr int CAPTURE_BASE = 1000000;
    constexpr int KILLER1_SCORE = CAPTURE_BASE - 1000;
    constexpr int KILLER2_SCORE = CAPTURE_BASE - 2000;
    constexpr int HISTORY_CAP = CAPTURE_BASE - 3000;

    std::array<std::pair<Move, int>, MAX_MOVES> scored;
    size_t n = legalMoves.size();

    for(size_t i = 0; i < n; ++i) {
        Move move = legalMoves[i];

        if(move.flags & CAPTURE) {
            int moved_piece = board.piece_on_square[move.start];
            int captured_piece = (move.flags & EN_PASSANT) ? PAWN : board.piece_on_square[move.end];

            scored[i] = {move, CAPTURE_BASE + (
                10 * values[captured_piece] - values[moved_piece]
            )};
            continue;
        }

        const Move& killer1 = killer_moves[ply][0];
        const Move& killer2 = killer_moves[ply][1];

        if(move.start == killer1.start && move.end == killer1.end && move.promotion == killer1.promotion) {
            scored[i] = {move, KILLER1_SCORE};
        } else if(move.start == killer2.start && move.end == killer2.end && move.promotion == killer2.promotion) {
            scored[i] = {move, KILLER2_SCORE};
        } else {
            int hist = history_table[board.active_color][move.start][move.end];
            scored[i] = {move, hist > 0 ? std::min(hist, HISTORY_CAP) : -1};
        }
    }

    std::stable_sort(scored.begin(), scored.begin() + n, [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    for(size_t i = 0; i < n; ++i) {
        legalMoves[i] = scored[i].first;
    }
}

//Mate scores are stored ply-independent (distance-to-mate from the node being
//stored, not from the search root), then re-anchored to the probing node's own
//ply on lookup. Without this, a mate score cached at one ply and transposed
//into at a different ply reports the wrong distance to mate.
static int scoreToTT(int score, int ply) {
    if(score >= MATE_THRESHOLD) return score + ply;
    if(score <= -MATE_THRESHOLD) return score - ply;
    return score;
}

static int scoreFromTT(int score, int ply) {
    if(score >= MATE_THRESHOLD) return score - ply;
    if(score <= -MATE_THRESHOLD) return score + ply;
    return score;
}

int negamax(Board& board, int alpha, int beta, int depth, int ply, int extensions) {
    ++nodes_searched;
    checkTimeBudget();
    if(search_aborted) {
        return 0; //discarded by the caller once aborted - see the comment above checkTimeBudget
    }

    if(depth == 0) {
        return quiescence(board, alpha, beta, ply, 0);
    }

    int original_alpha = alpha;

    TTEntry entry;
    bool found = probeTT(board.zobrist_key, entry);
    if(found && entry.depth >= depth) {
        int tt_score = scoreFromTT(entry.score, ply);
        if(entry.bound == EXACT) {
            return tt_score;
        } else if(entry.bound == LOWER && tt_score >= beta) {
            return tt_score;
        } else if(entry.bound == UPPER && tt_score <= alpha) {
            return tt_score;
        }
    }

    //Mate-distance pruning: a mate already found elsewhere in the tree can be
    //no further away than the current ply allows, so tighten the window to
    //match before doing any real work.
    alpha = std::max(alpha, -MATE_SCORE + ply);
    beta = std::min(beta, MATE_SCORE - ply);
    if(alpha >= beta) {
        return alpha;
    }

    bool in_check = isInCheck(board, board.active_color);

    //Null-move pruning: if passing the turn entirely still lets the reduced
    //search fail high, the position is good enough that a real move isn't
    //needed to prove it. Skipped in check (no legal null move) and with only
    //pawns left (zugzwang makes the null-move assumption unsound there).
    if(!in_check && depth >= 3 && hasNonPawnMaterial(board, board.active_color)) {
        constexpr int R = 2;

        NullUndoState null_state = board.makeNullMove();
        int null_score = -negamax(board, -beta, -beta + 1, depth - 1 - R, ply + 1, extensions);
        board.unmakeNullMove(null_state);

        if(search_aborted) {
            return 0;
        }

        if(null_score >= beta) {
            return beta;
        }
    }

    std::vector<Move>& moves = move_buffers[ply];
    generateLegalMoves(board, moves);

    if(moves.empty()) {
        if(in_check) {
            return -(MATE_SCORE - ply);
        } //checkmate
        return 0; //stalemate
    }

    reorderLegalMoves(board, moves, ply);

    if(found) {
        auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& m) {
            return m.start == entry.best_move.start && m.end == entry.best_move.end && m.promotion == entry.best_move.promotion;
        });

        if(it != moves.end()) {
            std::iter_swap(moves.begin(), it);
        }
    }

    //Check extension: a check at this node is followed out one extra ply
    //instead of counting against the depth budget, capped so a long forced-
    //check chain can't blow up the tree (mirrors quiescence's check_extensions).
    int extension = (in_check && extensions < MAX_CHECK_EXTENSIONS) ? 1 : 0;
    int child_extensions = extensions + extension;
    int child_depth = depth - 1 + extension;

    Move best_move = moves[0];
    bool first_move = true;

    for(const auto& move : moves) {
        UndoState state = board.makeMove(move);

        //PVS: the first move (already ordered to the front by the TT/killer/
        //history scheme above) gets a full-window search. Later moves are
        //scouted with a null window and only re-searched at full width if the
        //scout suggests they might actually beat alpha.
        int score;
        if(first_move) {
            score = -negamax(board, -beta, -alpha, child_depth, ply + 1, child_extensions);
            first_move = false;
        } else {
            score = -negamax(board, -alpha - 1, -alpha, child_depth, ply + 1, child_extensions);
            if(score > alpha && score < beta) {
                score = -negamax(board, -beta, -alpha, child_depth, ply + 1, child_extensions);
            }
        }

        board.unmakeMove(state);

        if(search_aborted) {
            return 0;
        }

        if(score >= beta) {
            if(!(move.flags & CAPTURE)) {
                recordKillerMove(ply, move);
                recordHistoryScore(board.active_color, move.start, move.end, depth);
            }

            storeTT(board.zobrist_key, scoreToTT(score, ply), depth, LOWER, move);
            return beta;
        }

        if(score > alpha) {
            alpha = score;
            best_move = move;
        }
    }

    int bound = (alpha > original_alpha) ? EXACT : UPPER;
    storeTT(board.zobrist_key, scoreToTT(alpha, ply), depth, bound, best_move);

    return alpha;
}

int quiescence(Board& board, int alpha, int beta, int ply, int check_extensions) {
    ++nodes_searched;
    checkTimeBudget();
    if(search_aborted) {
        return 0;
    }

    bool in_check = isInCheck(board, board.active_color);

    if(in_check) {
        std::vector<Move>& moves = move_buffers[ply];
        generateLegalMoves(board, moves);

        if(moves.empty()) {
            return -(MATE_SCORE - ply);
        }

        //Being in check means there is no option to "stand pat" - every legal
        //reply must be searched, not just captures. Once the extension budget
        //runs out, fall back to a static eval instead of silently dropping
        //non-capture evasions (the old behavior here).
        if(check_extensions >= MAX_CHECK_EXTENSIONS) {
            int eval = evaluate_board(board);
            return (board.active_color == WHITE) ? eval : -eval;
        }

        reorderLegalMoves(board, moves, ply);

        for(const Move& move : moves) {
            UndoState state = board.makeMove(move);
            int score = -quiescence(board, -beta, -alpha, ply + 1, check_extensions + 1);
            board.unmakeMove(state);

            if(search_aborted) {
                return 0;
            }

            if(score >= beta) {
                return beta;
            }

            if(score > alpha) {
                alpha = score;
            }
        }

        return alpha;
    }

    int stand_pat = (board.active_color == WHITE) ? board.eval_score : -board.eval_score;

    if(stand_pat >= beta) {
        return beta;
    }

    if(stand_pat > alpha) {
        alpha = stand_pat;
    }

    std::vector<Move>& captures = capture_buffers[ply];
    generateLegalCaptures(board, captures);

    if(captures.empty()) {
        return alpha;
    }

    reorderLegalMoves(board, captures, ply);

    for(const Move& move : captures) {
        UndoState state = board.makeMove(move);
        int score = -quiescence(board, -beta, -alpha, ply+1, 0);
        board.unmakeMove(state);

        if(search_aborted) {
            return 0;
        }

        if(score >= beta) {
            return beta;
        }

        if(score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

Move findBestMove(Board& board, int depth, int* out_score, std::chrono::milliseconds time_budget) {
    clearKillers(); //ply-indexed killers are only meaningful within this search tree

    time_limited = time_budget != std::chrono::milliseconds::max();
    search_aborted = false;
    if(time_limited) {
        search_deadline = std::chrono::steady_clock::now() + time_budget;
    }

    Move res{-1, -1}; //sentinel: no legal move found (checkmate/stalemate at the root)
    bool has_best = false;
    int final_score = 0;

    for(int curr_depth = 1; curr_depth <= depth; ++curr_depth) {
        if(time_limited && search_aborted) {
            break; //previous depth already exhausted the budget
        }

        int alpha = INT_MIN + 1;
        int beta = INT_MAX;

        std::vector<Move>& moves = move_buffers[0];
        generateLegalMoves(board, moves);

        if(moves.empty()) {
            break; //no legal move at any depth either - stop iterating
        }

        if(!has_best) {
            res = moves[0];
            has_best = true;
        } //Guarantee a legal fallback move exists even if the budget runs out
          //before any depth below finishes - see the comment on findBestMove
          //in search.hpp. Harmless no-op swap at curr_depth==1 otherwise.

        auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& m) {
            return m.start == res.start && m.end == res.end && m.promotion == res.promotion;
        });

        if(it != moves.end()) {
            std::iter_swap(moves.begin(), it);
        } //Iterative deepening (moving best moves in the beginning)

        Move iteration_best = moves[0];
        int iteration_score = INT_MIN;
        bool iteration_complete = true;

        for(const auto& move : moves) {
            UndoState state = board.makeMove(move);
            int score = -negamax(board, -beta, -alpha, curr_depth-1, 1, 0);
            board.unmakeMove(state);

            if(time_limited && search_aborted) {
                iteration_complete = false; //this depth's moves weren't all compared on equal
                break;                      //footing - discard the whole iteration, not just this move
            }

            if(score > iteration_score) {
                iteration_score = score;
                iteration_best = move;
            }

            if(score > alpha) {
                alpha = score;
            } //Ratchet the root window so later moves at this depth get pruned against it
        }

        if(iteration_complete) {
            res = iteration_best;
            final_score = iteration_score;
            has_best = true;
        } //else: keep the previous depth's committed result
    }

    if(out_score) {
        *out_score = final_score;
    }

    return res;
}

bool probeTT(uint64_t key, TTEntry& outEntry) {
    TTEntry& slot = transposition_table[key & (TT_SIZE - 1)];

    if(slot.key != key) {
        return false;
    }

    outEntry = slot;
    return true;
}

void storeTT(uint64_t key, int score, int depth, int bound, const Move& best_move) {
    TTEntry& slot = transposition_table[key & (TT_SIZE - 1)];

    //Depth-preferred replacement: always refresh the same position, otherwise
    //only overwrite if the new search went at least as deep as what's stored,
    //so a cheap shallow entry can't evict an expensive deep one on collision.
    if(slot.key != key && depth < slot.depth) {
        return;
    }

    slot.key = key;
    slot.score = score;
    slot.depth = depth;
    slot.bound = bound;
    slot.best_move = best_move;
}

void clearTT() {
    for(int i = 0; i < TT_SIZE; ++i) {
        transposition_table[i] = TTEntry{};
    }
}