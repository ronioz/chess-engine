#include <search.hpp>

//Per-ply move buffers, reused across calls instead of allocating a fresh vector
//per node. Indexed by ply rather than shared globally because negamax/quiescence
//recurse - a parent at ply P is still iterating its own moves while a child at
//ply P+1 is generating its own, so each ply needs a distinct slot. negamax's ply
//parameter starts at 1 (see findBestMove), so slot 0 is free for the root's own
//move list. negamax and quiescence's in-check branch never hold a buffer at the
//same ply simultaneously (depth==0 in negamax hands off to quiescence before
//touching move_buffers[ply]), so they safely share move_buffers; quiescence's
//non-check branch needs a second, separate array since it keeps both the full
//move list and the filtered capture list alive at once.
constexpr int MAX_PLY = 128;
static std::vector<Move> move_buffers[MAX_PLY];
static std::vector<Move> capture_buffers[MAX_PLY];

void reorderLegalMoves(Board& board, std::vector<Move>& legalMoves) {
    constexpr int values[6] = {100, 290, 310, 500, 900, 0};
    constexpr int MAX_MOVES = 256;
    int enemy_color = (board.active_color == WHITE) ? BLACK : WHITE;
    std::array<std::pair<Move, int>, MAX_MOVES> scored;
    size_t n = legalMoves.size();

    for(int i = 0; i < n; ++i) {
        Move move = legalMoves[i];
        if(!(move.flags & CAPTURE)) {
            scored[i] = {move, -1};
            continue;
        }

        int moved_piece = -1;
        for(int p = PAWN; p <= KING; ++p) {
            if(board.bitboards[p][board.active_color] & (1ULL << move.start)) {
                moved_piece = p;
                break;
            }
        }

        int captured_piece = PAWN; //en passant always captures a pawn
        if(!(move.flags & EN_PASSANT)) {
            for(int p = PAWN; p <= KING; ++p) {
                if(board.bitboards[p][enemy_color] & (1ULL << move.end)) {
                    captured_piece = p;
                    break;
                }
            }
        }

        scored[i] = {move, (
            10 * values[captured_piece] - values[moved_piece]
        )};
    }

    std::stable_sort(scored.begin(), scored.begin() + n, [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    for(int i = 0; i < n; ++i) {
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

int negamax(Board& board, int alpha, int beta, int depth, int ply) {
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

    std::vector<Move>& moves = move_buffers[ply];
    generateLegalMoves(board, moves);

    if(moves.empty()) {
        if(isInCheck(board, board.active_color)) {
            return -(MATE_SCORE - ply);
        } //checkmate
        return 0; //stalemate
    }

    reorderLegalMoves(board, moves);

    if(found) {
        auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& m) {
            return m.start == entry.best_move.start && m.end == entry.best_move.end && m.promotion == entry.best_move.promotion;
        });

        if(it != moves.end()) {
            std::iter_swap(moves.begin(), it);
        }
    }

    Move best_move = moves[0];

    for(const auto& move : moves) {
        UndoState state = board.makeMove(move);

        int score = -negamax(board, -beta, -alpha, depth - 1, ply + 1);

        board.unmakeMove(state);

        if(score >= beta) {
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

        reorderLegalMoves(board, moves);

        for(const Move& move : moves) {
            UndoState state = board.makeMove(move);
            int score = -quiescence(board, -beta, -alpha, ply + 1, check_extensions + 1);
            board.unmakeMove(state);

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
    std::vector<Move>& moves = move_buffers[ply];
    generateLegalMoves(board, moves);
    std::vector<Move>& captures = capture_buffers[ply];
    captures.clear();

    for(const Move& move : moves) {
        if((move.flags & CAPTURE)) {
            captures.push_back(move);
        }
    }

    if(captures.empty()) {
        return alpha;
    }

    reorderLegalMoves(board, captures);

    for(const Move& move : captures) {
        UndoState state = board.makeMove(move);
        int score = -quiescence(board, -beta, -alpha, ply+1, 0);
        board.unmakeMove(state);

        if(score >= beta) {
            return beta;
        }

        if(score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

Move findBestMove(Board& board, int depth) {
    Move res;
    bool has_best = false;

    for(int curr_depth = 1; curr_depth <= depth; ++curr_depth) {
        int alpha = INT_MIN + 1;
        int beta = INT_MAX;
        int best_score = INT_MIN;
        
        std::vector<Move>& moves = move_buffers[0];
        generateLegalMoves(board, moves);

        if(has_best) {
            auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& m) {
                return m.start == res.start && m.end == res.end && m.promotion == res.promotion;
            });

            if(it != moves.end()) {
                std::iter_swap(moves.begin(), it);
            }
        } //Iterative deepening (moving best moves in the beginning)

        for(const auto& move : moves) {
            UndoState state = board.makeMove(move);
            int score = -negamax(board, -beta, -alpha, curr_depth-1, 1);
            board.unmakeMove(state);

            if(score > best_score) {
                best_score = score;
                res = move;
                has_best = true;
            }

            if(score > alpha) {
                alpha = score;
            } //Ratchet the root window so later moves at this depth get pruned against it
        }
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