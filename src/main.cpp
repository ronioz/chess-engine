#include <iostream>
#include <string>
#include <board.hpp>
#include <movegen.hpp>
#include <search.hpp>
#include <perft.hpp>

int main() {
    Board board;
    board.setup();

    benchmarkPerft(board, 6);
}
