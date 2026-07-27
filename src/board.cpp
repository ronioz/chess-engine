#include <board.hpp>

Board::Board() {
    clear();
}

void Board::switchColor() {
    active_color = (active_color == WHITE) ? BLACK : WHITE;
}

void Board::clear() {
    for(int p = PAWN; p <= KING; ++p) {
        for(int c = WHITE; c <= BLACK; ++c) {
            bitboards[p][c] = 0ULL;
        }
    }

    white_pieces = 0ULL;
    black_pieces = 0ULL;
    all_pieces = 0ULL;
}

void Board::setup() {
    bitboards[PAWN][WHITE]   = 0x000000000000FF00ULL;
    bitboards[KNIGHT][WHITE] = 0x0000000000000042ULL;
    bitboards[BISHOP][WHITE] = 0x0000000000000024ULL;
    bitboards[ROOK][WHITE]   = 0x0000000000000081ULL;
    bitboards[QUEEN][WHITE]  = 0x0000000000000008ULL;
    bitboards[KING][WHITE]   = 0x0000000000000010ULL; 

    bitboards[PAWN][BLACK]   = 0x00FF000000000000ULL;
    bitboards[KNIGHT][BLACK] = 0x4200000000000000ULL;
    bitboards[BISHOP][BLACK] = 0x2400000000000000ULL;
    bitboards[ROOK][BLACK]   = 0x8100000000000000ULL;
    bitboards[QUEEN][BLACK]  = 0x0800000000000000ULL;
    bitboards[KING][BLACK]   = 0x1000000000000000ULL; 

    for(int p = PAWN; p <= KING; ++p) {
        white_pieces |= bitboards[p][WHITE];
        black_pieces |= bitboards[p][BLACK];
    }

    all_pieces = white_pieces | black_pieces;

    active_color = WHITE;

    fillBoard();
}

void Board::fillBoard() {
    for(int rank = 7; rank >= 0; --rank) {
        for(int file = 0; file <= 7; ++file) {
            int sq = rank * 8 + file;
            uint64_t mask = (1ULL << sq);

            char piece = '.'; //empty cell

            for(int p = PAWN; p <= KING; ++p) {
                for(int c = WHITE; c <= BLACK; ++c) {
                    if(bitboards[p][c] & mask) {
                        piece = symbols[p][c];
                    }
                }
            }

            board[rank][file] = piece;
        }
    }
}

void Board::printBoard() {
    for(int rank = 7; rank >= 0; --rank) {
        for(int file = 0; file <= 7; ++file) {
            printf("%c ", board[rank][file]);
        }

        printf("\n");
    }
}