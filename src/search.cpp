#include <search.hpp>

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