#include <perft.hpp>
#include <movegen.hpp>
#include <board.hpp>
#include <fen_conversion.hpp>
#include <cassert>
#include <cstdio>

void test_default_perft() {
    Board board;
    board.setup();

    uint64_t input[5] = {1,2,3,4,5};
    uint64_t output[5] = {20,400,8902,197281,4865609};

    for(int i = 0; i < 5; ++i) {
        assert(perft(board, input[i]) == output[i]);
    }
}

void test_from_fen_perft() {
    std::string fens[5] = {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"
    };

    uint64_t input[5] = {1,2,3,4,5};
    uint64_t output[5][5] = {
        {48, 2039, 97862, 4085603, 193690690},
        {14, 191, 2812, 43238, 674624},
        {6, 264, 9467, 422333, 15833292},
        {44, 1486, 62379, 2103487, 89941194},
        {46, 2079, 89890, 3894594, 164075551}
    };

    for(int i = 0; i < 5; ++i) {
        Board board;
        from_FEN(board, fens[i]);

        for(int j = 0; j < 5; ++j) {
            assert(perft(board, input[j]) == output[i][j]);
        }
    }
}

int main() {
    test_default_perft();
    test_from_fen_perft();

    printf("test_perft: all tests passed\n");
    return 0;
}
