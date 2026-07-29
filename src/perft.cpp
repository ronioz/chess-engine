#include <perft.hpp>

uint64_t perft(Board& board, int depth) {
    if(depth == 0) {
        return 1ULL;
    }

    uint64_t res = 0ULL;

    for(const auto& move : generateLegalMoves(board)) {
        UndoState state = board.makeMove(move);
        res += perft(board, depth-1);
        board.unmakeMove(state);
    }

    return res;
}