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

    std::vector<Move> moves = generateLegalMoves(board);

    if(moves.empty()) {
        if(isInCheck(board, board.active_color)) {
            return -100000; 
        } //checkmate
        return 0; //stalemate
    }

    reorderLegalMoves(board, moves);
    for(const auto& move : moves) {
        UndoState state = board.makeMove(move);
        
        int score = -negamax(board, -beta, -alpha, depth - 1);
        
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
    int best_score = INT_MIN;

    int alpha = INT_MIN + 1;
    int beta = INT_MAX;

    std::vector<Move> moves = generateLegalMoves(board);

    for(const auto& move : moves) {
        UndoState state = board.makeMove(move);

        int score = -negamax(board, -beta, -alpha, depth-1);

        board.unmakeMove(state);

        if(score > best_score) {
            best_score = score;
            res = move;
        }
    }

    return res;
}