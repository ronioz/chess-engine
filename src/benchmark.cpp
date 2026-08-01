#include <benchmark.hpp>
#include <fen_conversion.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>

struct BenchPosition {
    const char* name;
    const char* fen;
};

//A small spread of game phases/branching factors, not just the start position.
//Kiwipete and positions 3/5 are the standard chessprogramming.org perft suite
//positions already used to validate movegen (see CLAUDE.md) - reused here since
//they're already known-good FENs with well-understood branching characteristics.
static const BenchPosition bench_positions[] = {
    {"Start position",        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"},
    {"Kiwipete (complex midgame)", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"},
    {"Endgame (sparse material)", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"},
    {"Tactical (promotions/pins)", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"},
};

static std::string squareToStr(int square) {
    std::string s;
    s += char('a' + (square % 8));
    s += char('1' + (square / 8));
    return s;
}

static std::string moveToStr(const Move& move) {
    constexpr char promo_chars[6] = {'p', 'n', 'b', 'r', 'q', 'k'};

    std::string s = squareToStr(move.start) + squareToStr(move.end);
    if(move.promotion != -1) {
        s += promo_chars[move.promotion];
    }
    return s;
}

void benchmarkSearch(Board& board, int depth, const std::string& label) {
    clearTT();
    clearKillers();
    clearHistory();
    resetNodeCount();

    auto startTime = std::chrono::high_resolution_clock::now();

    int score = 0;
    Move best = findBestMove(board, depth, &score);

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;

    double seconds = elapsed.count();
    double nps = (seconds > 0) ? (nodes_searched / seconds) : 0;

    std::cout << "--- " << label << " (depth " << depth << ") ---" << std::endl;
    std::cout << "  Best move : " << moveToStr(best) << std::endl;
    std::cout << "  Score     : " << score << std::endl;
    std::cout << "  Nodes     : " << nodes_searched << std::endl;
    std::cout << "  Time      : " << std::fixed << std::setprecision(3) << seconds << " s" << std::endl;
    std::cout << "  NPS       : " << std::fixed << std::setprecision(0) << nps << " nodes/sec ("
              << (nps / 1e6) << " MNPS)" << std::endl;
}

void runSearchBenchmark(int depth) {
    std::cout << "=== Search Benchmark (depth " << depth << ") ===" << std::endl;

    uint64_t total_nodes = 0ULL;
    auto suiteStart = std::chrono::high_resolution_clock::now();

    for(const auto& pos : bench_positions) {
        Board board;
        if(!from_FEN(board, pos.fen)) {
            std::cout << "Failed to parse FEN for " << pos.name << std::endl;
            continue;
        }

        benchmarkSearch(board, depth, pos.name);
        total_nodes += nodes_searched;
        std::cout << std::endl;
    }

    auto suiteEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = suiteEnd - suiteStart;

    double seconds = elapsed.count();
    double nps = (seconds > 0) ? (total_nodes / seconds) : 0;

    std::cout << "========================================" << std::endl;
    std::cout << "Total Nodes : " << total_nodes << std::endl;
    std::cout << "Total Time  : " << std::fixed << std::setprecision(3) << seconds << " s" << std::endl;
    std::cout << "Overall NPS : " << std::fixed << std::setprecision(0) << nps << " nodes/sec ("
              << (nps / 1e6) << " MNPS)" << std::endl;
    std::cout << "========================================" << std::endl;
}
