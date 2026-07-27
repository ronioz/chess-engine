#include <board.hpp>
#include <cassert>
#include <cstdio>

void test_clear() {
    Board board;
    board.setup();
    board.clear();

    for(int p = PAWN; p <= KING; ++p) {
        for(int c = WHITE; c <= BLACK; ++c) {
            assert(board.bitboards[p][c] == 0ULL);
        }
    }

    assert(board.white_pieces == 0ULL);
    assert(board.black_pieces == 0ULL);
    assert(board.all_pieces == 0ULL);
}

void test_setup() {
    Board board;
    board.setup();

    assert(board.bitboards[PAWN][WHITE]   == 0x000000000000FF00ULL);
    assert(board.bitboards[KNIGHT][WHITE] == 0x0000000000000042ULL);
    assert(board.bitboards[BISHOP][WHITE] == 0x0000000000000024ULL);
    assert(board.bitboards[ROOK][WHITE]   == 0x0000000000000081ULL);
    assert(board.bitboards[QUEEN][WHITE]  == 0x0000000000000008ULL);
    assert(board.bitboards[KING][WHITE]   == 0x0000000000000010ULL);

    assert(board.bitboards[PAWN][BLACK]   == 0x00FF000000000000ULL);
    assert(board.bitboards[KNIGHT][BLACK] == 0x4200000000000000ULL);
    assert(board.bitboards[BISHOP][BLACK] == 0x2400000000000000ULL);
    assert(board.bitboards[ROOK][BLACK]   == 0x8100000000000000ULL);
    assert(board.bitboards[QUEEN][BLACK]  == 0x0800000000000000ULL);
    assert(board.bitboards[KING][BLACK]   == 0x1000000000000000ULL);

    assert(board.active_color == WHITE);

    assert(board.all_pieces == (board.white_pieces | board.black_pieces));
    assert((board.white_pieces & board.black_pieces) == 0ULL); // no overlap

    // fillBoard() cache should match the bitboards
    assert(board.board[0][0] == 'R'); // a1
    assert(board.board[0][4] == 'K'); // e1
    assert(board.board[7][4] == 'k'); // e8
    assert(board.board[3][3] == '.'); // d4, empty
}

void test_switch_color() {
    Board board;
    board.setup();
    assert(board.active_color == WHITE);

    board.switchColor();
    assert(board.active_color == BLACK);

    board.switchColor();
    assert(board.active_color == WHITE);
}

int main() {
    test_clear();
    test_setup();
    test_switch_color();

    printf("test_board: all tests passed\n");
    return 0;
}
