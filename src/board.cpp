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

    for(int sq = 0; sq < 64; ++sq) {
        piece_on_square[sq] = -1;
    }
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
    initEvalScore();
    initPieceOnSquare();

    active_color = WHITE;
    castling_rights = 15;
    en_passant_square = -1;
    halfmove_clock = 0;
    fullmove_number = 1;

    initZobrist();

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

    int moved_piece = piece_on_square[start];
    int captured_piece = (all_pieces & (1ULL << end)) ? piece_on_square[end] : -1;

    UndoState state;
    state.move = move;
    state.moved_piece = moved_piece;
    state.captured_piece = captured_piece;
    state.castling_rights = castling_rights;
    state.en_passant_square = en_passant_square;
    state.halfmove_clock = halfmove_clock;
    state.zobrist_key = zobrist_key;
    state.eval_score = eval_score;

    if(captured_piece != -1) {
        clearPiece(captured_piece, enemy_color, end);
        zobrist_key ^= zobrist_pieces[captured_piece][enemy_color][end]; 
    }

    if(move.flags & EN_PASSANT) {
        int captured_square = (active_color == WHITE) ? end - 8 : end + 8;
        clearPiece(PAWN, enemy_color, captured_square);
        zobrist_key ^= zobrist_pieces[PAWN][enemy_color][captured_square];
        captured_piece = PAWN;
        state.captured_piece = PAWN;
    }

    clearPiece(moved_piece, active_color, start);
    zobrist_key ^= zobrist_pieces[moved_piece][active_color][start];
    setPiece(moved_piece, active_color, end);
    zobrist_key ^= zobrist_pieces[moved_piece][active_color][end];

    if(move.promotion != -1) {
        clearPiece(moved_piece, active_color, end);
        zobrist_key ^= zobrist_pieces[moved_piece][active_color][end];
        setPiece(move.promotion, active_color, end);
        zobrist_key ^= zobrist_pieces[move.promotion][active_color][end];
    }

    if(moved_piece == KING && (move.flags & CASTLE)) {
        if(end == 6) {
            clearPiece(ROOK, WHITE, 7);
            zobrist_key ^= zobrist_pieces[ROOK][WHITE][7];
            setPiece(ROOK, WHITE, 5);
            zobrist_key ^= zobrist_pieces[ROOK][WHITE][5];
        } else if (end == 2) {
            clearPiece(ROOK, WHITE, 0);
            zobrist_key ^= zobrist_pieces[ROOK][WHITE][0];
            setPiece(ROOK, WHITE, 3);
            zobrist_key ^= zobrist_pieces[ROOK][WHITE][3];
        } else if (end == 62) {
            clearPiece(ROOK, BLACK, 63);
            zobrist_key ^= zobrist_pieces[ROOK][BLACK][63];
            setPiece(ROOK, BLACK, 61);
            zobrist_key ^= zobrist_pieces[ROOK][BLACK][61];
        } else {
            clearPiece(ROOK, BLACK, 56);
            zobrist_key ^= zobrist_pieces[ROOK][BLACK][56];
            setPiece(ROOK, BLACK, 59);
            zobrist_key ^= zobrist_pieces[ROOK][BLACK][59];
        }
    }

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

    zobrist_key ^= zobrist_castling[state.castling_rights];
    zobrist_key ^= zobrist_castling[castling_rights];

    if(state.en_passant_square != -1) {
        zobrist_key ^= zobrist_ep_file[state.en_passant_square % 8];
    }

    if(move.flags & DOUBLE_PAWN_PUSH) {
        en_passant_square = (start + end) / 2;
    } else {
        en_passant_square = -1;
    }

    if(en_passant_square != -1) {
        zobrist_key ^= zobrist_ep_file[en_passant_square % 8];
    }

    if(moved_piece == PAWN || captured_piece != -1) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }

    if(active_color == BLACK) {
        fullmove_number++;
    }

    zobrist_key ^= zobrist_side_to_move;
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
            clearPiece(ROOK, WHITE, 5);
            setPiece(ROOK, WHITE, 7);
        } else if(state.move.end == 2) {
            clearPiece(ROOK, WHITE, 3);
            setPiece(ROOK, WHITE, 0);
        } else if(state.move.end == 62) {
            clearPiece(ROOK, BLACK, 61);
            setPiece(ROOK, BLACK, 63);
        } else {
            clearPiece(ROOK, BLACK, 59);
            setPiece(ROOK, BLACK, 56);
        }
    }

    if(state.move.promotion != -1) {
        clearPiece(state.move.promotion, active_color, state.move.end);
    }

    clearPiece(state.moved_piece, active_color, state.move.end);
    setPiece(state.moved_piece, active_color, state.move.start);

    if(state.move.flags & EN_PASSANT) {
        int captured_square = (active_color == WHITE) ? state.move.end - 8 : state.move.end + 8;
        setPiece(PAWN, enemy_color, captured_square);
    } else if(state.captured_piece != -1) {
        setPiece(state.captured_piece, enemy_color, state.move.end);
    }

    castling_rights = state.castling_rights;
    en_passant_square = state.en_passant_square;
    halfmove_clock = state.halfmove_clock;
    zobrist_key = state.zobrist_key;
    eval_score = state.eval_score;
}

void Board::setPiece(int piece, int color, int sq) {
    uint64_t mask = 1ULL << sq;
    bitboards[piece][color] |= mask;
    (color == WHITE ? white_pieces : black_pieces) |= mask;
    all_pieces |= mask;
    piece_on_square[sq] = piece;

    int pst_sq = (color == WHITE) ? sq : (sq ^ 56);
    int delta = piece_value[piece] + piece_pst[piece][pst_sq];
    eval_score += (color == WHITE) ? delta : -delta;
}

void Board::clearPiece(int piece, int color, int sq) {
    uint64_t mask = ~(1ULL << sq);
    bitboards[piece][color] &= mask;
    (color == WHITE ? white_pieces : black_pieces) &= mask;
    all_pieces &= mask;
    piece_on_square[sq] = -1;

    int pst_sq = (color == WHITE) ? sq : (sq ^ 56);
    int delta = piece_value[piece] + piece_pst[piece][pst_sq];
    eval_score -= (color == WHITE) ? delta : -delta;
}

void Board::initPieceOnSquare() {
    for(int sq = 0; sq < 64; ++sq) {
        piece_on_square[sq] = -1;
    }

    for(int p = PAWN; p <= KING; ++p) {
        for(int c = WHITE; c <= BLACK; ++c) {
            uint64_t temp = bitboards[p][c];
            while(temp) {
                int sq = __builtin_ctzll(temp);
                piece_on_square[sq] = p;
                temp &= (temp - 1);
            }
        }
    }
}

void Board::initEvalScore() {
    eval_score = 0;

    for(int p = PAWN; p <= KING; ++p) {
        uint64_t temp = bitboards[p][WHITE];
        while(temp) {
            int sq = __builtin_ctzll(temp);
            eval_score += piece_value[p] + piece_pst[p][sq];
            temp &= (temp - 1);
        }
    }

    for(int p = PAWN; p <= KING; ++p) {
        uint64_t temp = bitboards[p][BLACK];
        while(temp) {
            int sq = __builtin_ctzll(temp) ^ 56;
            eval_score -= piece_value[p] + piece_pst[p][sq];
            temp &= (temp - 1);
        }
    }
}

void Board::initZobrist() {
    if(!zobrist_tables_ready) {
        std::mt19937_64 rng(123456789ULL);

        for(int p = PAWN; p <= KING; ++p) {
            for(int c = WHITE; c <= BLACK; ++c) {
                for(int sq = 0; sq <= 63; ++sq) {
                    zobrist_pieces[p][c][sq] = rng();
                }
            }
        }

        zobrist_side_to_move = rng();

        for(int i = 0; i <= 15; ++i) {
            zobrist_castling[i] = rng();
        }

        for(int file = 0; file <= 7; ++file) {
            zobrist_ep_file[file] = rng();
        }

        zobrist_tables_ready = true;
    }

    zobrist_key = initZobristKey();
}

uint64_t Board::initZobristKey() {
    uint64_t res = 0ULL;

    for(int sq = 0; sq <= 63; ++sq) {
        int occupied_piece = -1;
        int occupied_color = -1;

        for(int p = PAWN; p <= KING; ++p) {
            if(occupied_piece != -1 && occupied_color != -1) 
                break;

            for(int c = WHITE; c <= BLACK; ++c) {
                if(bitboards[p][c] & (1ULL << sq)) {
                    occupied_piece = p;
                    occupied_color = c;
                    break;
                }
            }
        }

        if(occupied_piece != -1 && occupied_color != -1) {
            res ^= zobrist_pieces[occupied_piece][occupied_color][sq];
        }
    }

    if(active_color == BLACK) {
        res ^= zobrist_side_to_move;
    }

    res ^= zobrist_castling[castling_rights];
    
    if(en_passant_square != -1) {
        res ^= zobrist_ep_file[en_passant_square % 8];
    }

    return res;
}