#include <board.hpp>

Board::Board() {
    clear();
}

void Board::switchColor() {
    active_color = (active_color == WHITE) ? BLACK : WHITE;
}

void Board::updateBitboards() {
    white_pieces = 0ULL;
    black_pieces = 0ULL;
    all_pieces = 0ULL;

    for(int p = PAWN; p <= KING; ++p) {
        white_pieces |= bitboards[p][WHITE];
        black_pieces |= bitboards[p][BLACK];
    }

    all_pieces = white_pieces | black_pieces;
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

    updateBitboards();

    active_color = WHITE;
    castling_rights = 15;
    en_passant_square = -1;
    halfmove_clock = 0;
    fullmove_number = 1;

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

UndoState Board::makeMove(const Move& move) {
    int start = move.start;
    int end = move.end;
    int enemy_color = (active_color == WHITE) ? BLACK : WHITE;

    int moved_piece = -1;
    int captured_piece = -1;

    for(int p = PAWN; p <= KING; ++p) {
        if(bitboards[p][active_color] & (1ULL << start)) {
            moved_piece = p;
            break;
        }
    }

    if(all_pieces & (1ULL << end)) {
        for(int p = PAWN; p <= KING; ++p) {
            if(bitboards[p][enemy_color] & (1ULL << end)) {
                captured_piece = p;
                break;
            }
        }
    }

    UndoState state;
    state.move = move;
    state.moved_piece = moved_piece;
    state.captured_piece = captured_piece;
    state.castling_rights = castling_rights;
    state.en_passant_square = en_passant_square;
    state.halfmove_clock = halfmove_clock;

    if(captured_piece != -1) {
        bitboards[captured_piece][enemy_color] &= ~(1ULL << end);
    }

    if(move.flags & EN_PASSANT) {
        int captured_square = (active_color == WHITE) ? end - 8 : end + 8;
        bitboards[PAWN][enemy_color] &= ~(1ULL << captured_square);
        captured_piece = PAWN;
        state.captured_piece = PAWN;
    }

    bitboards[moved_piece][active_color] &= ~(1ULL << start);
    bitboards[moved_piece][active_color] |= (1ULL << end);

    if(move.promotion != -1) {
        bitboards[moved_piece][active_color] &= ~(1ULL << end);
        bitboards[move.promotion][active_color] |= (1ULL << end);
    }

    if(moved_piece == KING && (move.flags & CASTLE)) {
        if(end == 6) {
            bitboards[ROOK][WHITE] &= ~(1ULL << 7);
            bitboards[ROOK][WHITE] |= (1ULL << 5);
        } else if (end == 2) {
            bitboards[ROOK][WHITE] &= ~(1ULL << 0);
            bitboards[ROOK][WHITE] |= (1ULL << 3);
        } else if (end == 62) {
            bitboards[ROOK][BLACK] &= ~(1ULL << 63);
            bitboards[ROOK][BLACK] |= (1ULL << 61);
        } else {
            bitboards[ROOK][BLACK] &= ~(1ULL << 56);
            bitboards[ROOK][BLACK] |= (1ULL << 59);
        }
    }

    updateBitboards();

    if(moved_piece == KING) {
        if(active_color == WHITE) {
            castling_rights &= 3;
        } else {
            castling_rights &= 12;
        }
    } 

    if (moved_piece == ROOK) {
        if(active_color == WHITE) {
            if(start == 0) {
                castling_rights &= 11;
            } else if (start == 7) {
                castling_rights &= 7;
            }
        } else {
            if(start == 56) {
                castling_rights &= 14;
            } else if (start == 63) {
                castling_rights &= 13;
            }
        }
    }

    if(captured_piece == ROOK) {
        if(end == 0) {
            castling_rights &= 11;
        } else if(end == 7) {
            castling_rights &= 7;
        } else if(end == 56) {
            castling_rights &= 14;
        } else if(end == 63) {
            castling_rights &= 13;
        }
    }

    if(move.flags & DOUBLE_PAWN_PUSH) {
        en_passant_square = (start + end) / 2;
    } else {
        en_passant_square = -1;
    }

    if(moved_piece == PAWN || captured_piece != -1) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }

    if(active_color == BLACK) {
        fullmove_number++;
    }

    switchColor();

    return state;
}

void Board::unmakeMove(const UndoState& state) {
    switchColor();

    if(active_color == BLACK) {
        fullmove_number--;
    }

    int enemy_color = (active_color == WHITE) ? BLACK : WHITE;

    if(state.move.flags & CASTLE) {
        if(state.move.end == 6) {
            bitboards[ROOK][WHITE] &= ~(1ULL << 5);
            bitboards[ROOK][WHITE] |= (1ULL << 7);
        } else if(state.move.end == 2) {
            bitboards[ROOK][WHITE] &= ~(1ULL << 3);
            bitboards[ROOK][WHITE] |= (1ULL << 0);
        } else if(state.move.end == 62) {
            bitboards[ROOK][BLACK] &= ~(1ULL << 61);
            bitboards[ROOK][BLACK] |= (1ULL << 63);
        } else {
            bitboards[ROOK][BLACK] &= ~(1ULL << 59);
            bitboards[ROOK][BLACK] |= (1ULL << 56);
        }
    }

    if(state.move.promotion != -1) {
        bitboards[state.move.promotion][active_color] &= ~(1ULL << state.move.end);
    }

    bitboards[state.moved_piece][active_color] &= ~(1ULL << state.move.end);
    bitboards[state.moved_piece][active_color] |= (1ULL << state.move.start);

    if(state.move.flags & EN_PASSANT) {
        int captured_square = (active_color == WHITE) ? state.move.end - 8 : state.move.end + 8;
        bitboards[PAWN][enemy_color] |= (1ULL << captured_square);
    } else if(state.captured_piece != -1) {
        bitboards[state.captured_piece][enemy_color] |= (1ULL << state.move.end);
    }

    updateBitboards();
    castling_rights = state.castling_rights;
    en_passant_square = state.en_passant_square;
    halfmove_clock = state.halfmove_clock;
}