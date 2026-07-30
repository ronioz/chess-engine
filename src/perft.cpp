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

void benchmarkPerft(Board& board, int depth) {
    std::cout << "--- Running Perft Benchmark (Depth " << depth << ") ---" << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();
    
    uint64_t totalNodes = 0ULL;
    auto moves = generateLegalMoves(board);

    for (const auto& move : moves) {
        UndoState state = board.makeMove(move);
        uint64_t nodes = perft(board, depth - 1);
        board.unmakeMove(state);
        
        totalNodes += nodes;
        // Optionally print move breakdown (e.g., e2e4: 20 -> std::cout << move.toNotation() << ": " << nodes << "\n")
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;

    double seconds = elapsed.count();
    double nps = (seconds > 0) ? (totalNodes / seconds) : 0;

    std::cout << "========================================" << std::endl;
    std::cout << "Total Nodes : " << totalNodes << std::endl;
    std::cout << "Time Taken  : " << std::fixed << std::setprecision(3) << seconds << " s" << std::endl;
    std::cout << "NPS         : " << std::fixed << std::setprecision(0) << nps << " nodes/sec (" 
              << (nps / 1e6) << " MNPS)" << std::endl;
    std::cout << "========================================" << std::endl;
}