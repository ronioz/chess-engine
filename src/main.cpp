#include <iostream>
#include <string>
#include <board.hpp>
#include <movegen.hpp>
#include <search.hpp>
#include <perft.hpp>
#include <benchmark.hpp>
#include <uci.hpp>

int main(int argc, char** argv) {
    if(argc > 1 && std::string(argv[1]) == "bench") {
        Board board;
        board.setup();

        benchmarkPerft(board, 6);
        std::cout << std::endl;
        runSearchBenchmark(6);
        return 0;
    }

    runUCI();
    return 0;
}
