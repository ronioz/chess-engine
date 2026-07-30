#include <search.hpp>

void reorderLegalMoves(Board& board, std::vector<Move>& legalMoves) {
    constexpr int values[6] = {100, 290, 310, 500, 900, 0};

    std::vector<std::pair<Move, int>> scored;
    scored.reserve(legalMoves.size());

    for(const Move& move : legalMoves) {
        if(!(move.flags & CAPTURE)) {
            scored.push_back({move, -1});
            continue;
        }

        UndoState state = board.makeMove(move);
        board.unmakeMove(state);
        scored.push_back({move, (
            10 * values[state.captured_piece] - values[state.moved_piece]
        )});
    }

    std::stable_sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    for(int i = 0; i < legalMoves.size(); ++i) {
        legalMoves[i] = scored[i].first;
    }
}

int negamax(Board& board, int alpha, int beta, int depth) {
    if(depth == 0) {
        int eval = evaluate_board(board);
        return (board.active_color == WHITE) ? eval : -eval;
    }

    int original_alpha = alpha;

    TTEntry entry;
    bool found = probeTT(board.zobrist_key, entry);
    if(found && entry.depth >= depth) {
        if(entry.bound == EXACT) {
            return entry.score;
        } else if(entry.bound == LOWER && entry.score >= beta) {
            return entry.score;
        } else if(entry.bound == UPPER && entry.score <= alpha) {
            return entry.score;
        }
    }

    std::vector<Move> moves = generateLegalMoves(board);

    if(moves.empty()) {
        if(isInCheck(board, board.active_color)) {
            return -100000;
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

        int score = -negamax(board, -beta, -alpha, depth - 1);

        board.unmakeMove(state);

        if(score >= beta) {
            storeTT(board.zobrist_key, score, depth, LOWER, move);
            return beta;
        }

        if(score > alpha) {
            alpha = score;
            best_move = move;
        }
    }

    int bound = (alpha > original_alpha) ? EXACT : UPPER;
    storeTT(board.zobrist_key, alpha, depth, bound, best_move);

    return alpha;
}

Move findBestMove(Board& board, int depth) {
    Move res;
    bool has_best = false;

    int alpha = INT_MIN + 1;
    int beta = INT_MAX;

    for(int curr_depth = 1; curr_depth <= depth; ++curr_depth) {
        int best_score = INT_MIN;

        std::vector<Move> moves = generateLegalMoves(board);
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
            int score = -negamax(board, -beta, -alpha, curr_depth-1);
            board.unmakeMove(state);

            if(score > best_score) {
                best_score = score;
                res = move;
                has_best = true;
            }
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
    transposition_table[key & (TT_SIZE - 1)].key = key;
    transposition_table[key & (TT_SIZE - 1)].score = score;
    transposition_table[key & (TT_SIZE - 1)].depth = depth;
    transposition_table[key & (TT_SIZE - 1)].bound = bound;
    transposition_table[key & (TT_SIZE - 1)].best_move = best_move;
}