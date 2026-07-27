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

bool Board::from_FEN(const std::string& FEN) {
    clear();

    //[PIECE PLACEMENT]

    int rank = 7;
    int file = 0;
    int len = FEN.size();
    int i = 0;

    for(i = 0; i < len; ++i) {
        if(FEN[i] == ' ') break; //stops at first whitespace

        if(rank < 0 || file > 8 || (file == 8 && FEN[i] != '/')) {
            printf("Check FEN Piece placement. It is incorrect\n");
            return false;
        }

        int color = -1;
        int piece = -1;

        if(FEN[i] == '/') {
            file = 0;
            rank--;
            continue;
        }

        if(1 <= (FEN[i] - '0') && (FEN[i] - '0') <= 9) {
            file += (FEN[i] - '0');
            continue;
        }

        if(FEN[i] == 'p' || FEN[i] == 'P') piece = PAWN;
        else if(FEN[i] == 'n' || FEN[i] == 'N') piece = KNIGHT;
        else if(FEN[i] == 'b' || FEN[i] == 'B') piece = BISHOP;
        else if(FEN[i] == 'r' || FEN[i] == 'R') piece = ROOK;
        else if(FEN[i] == 'q' || FEN[i] == 'Q') piece = QUEEN;
        else if(FEN[i] == 'k' || FEN[i] == 'K') piece = KING;

        color = ('A' <= (int)FEN[i] && (int)FEN[i] <= 'Z') ? WHITE : BLACK;

        if(piece == -1 || color == -1) {
            printf("Check FEN Piece placement. It is incorrect\n");
            return false;
        }

        uint64_t mask = (1ULL << (rank * 8 + file));

        bitboards[piece][color] |= mask;

        if(color == WHITE) white_pieces |= mask;
        else black_pieces |= mask;

        file++;
    }

    all_pieces = white_pieces | black_pieces;
    fillBoard();

    //[ACTIVE COLOR]
    if(i >= len) {
        printf("Check FEN. It is incomplete\n");
        return false;
    }

    i++; //skip space

    int next = FEN.find(' ', i);
    std::string token = FEN.substr(i, next - i);

    if(token != "w" && token != "b") {
        printf("Check FEN Active Color. It is incorrect\n");
        return false;
    }

    active_color = (token == "w") ? WHITE : BLACK;

    i = next;

    //[CASTLING RIGHTS]
    if(i == -1 || i >= len) {
        printf("Check FEN. It is incomplete\n");
        return false;
    }

    i++; //skip space

    next = FEN.find(' ', i);
    token = FEN.substr(i, next - i);

    if(token.empty()) {
        printf("Check FEN Castling rights. It is incorrect\n");
        return false;
    }

    white_can_castle_kingside = false;
    white_can_castle_queenside = false;
    black_can_castle_kingside = false;
    black_can_castle_queenside = false;

    if(token != "-") {
        for(int j = 0; j < (int)token.size(); ++j) {
            if(token[j] == 'K') white_can_castle_kingside = true;
            else if(token[j] == 'Q') white_can_castle_queenside = true;
            else if(token[j] == 'k') black_can_castle_kingside = true;
            else if(token[j] == 'q') black_can_castle_queenside = true;
            else {
                printf("Check FEN Castling rights. It is incorrect\n");
                return false;
            }
        }
    }

    i = next;

    //[EN PASSANT]
    if(i == -1 || i >= len) {
        printf("Check FEN. It is incomplete\n");
        return false;
    }

    i++; //skip space

    next = FEN.find(' ', i);
    token = FEN.substr(i, next - i);

    if(token.empty()) {
        printf("Check FEN En passant. It is incorrect\n");
        return false;
    }

    if(token == "-") {
        en_passant_square = -1;
    } else if(token.size() == 2 && token[0] >= 'a' && token[0] <= 'h' && token[1] >= '1' && token[1] <= '8') {
        int ep_file = token[0] - 'a';
        int ep_rank = token[1] - '1';
        en_passant_square = ep_rank * 8 + ep_file;
    } else {
        printf("Check FEN En passant. It is incorrect\n");
        return false;
    }

    i = next;

    //[HALFMOVE CLOCK]
    if(i == -1 || i >= len) {
        printf("Check FEN. It is incomplete\n");
        return false;
    }

    i++; //skip space

    next = FEN.find(' ', i);
    token = FEN.substr(i, next - i);

    if(token.empty()) {
        printf("Check FEN Halfmove clock. It is incorrect\n");
        return false;
    }

    for(int j = 0; j < (int)token.size(); ++j) {
        if(token[j] < '0' || token[j] > '9') {
            printf("Check FEN Halfmove clock. It is incorrect\n");
            return false;
        }
    }

    halfmove_clock = std::stoi(token);

    i = next;

    //[FULLMOVE NUMBER]
    if(i == -1 || i >= len) {
        printf("Check FEN. It is incomplete\n");
        return false;
    }

    i++; //skip space

    token = FEN.substr(i);

    if(token.empty()) {
        printf("Check FEN Fullmove number. It is incorrect\n");
        return false;
    }

    for(int j = 0; j < (int)token.size(); ++j) {
        if(token[j] < '0' || token[j] > '9') {
            printf("Check FEN Fullmove number. It is incorrect\n");
            return false;
        }
    }

    fullmove_number = std::stoi(token);

    return true;
}

std::string Board::to_FEN() const {
    std::string res = "";

    // [Piece Placement]
    int cnt = 0;

    for(int rank = 7; rank >= 0; --rank) {
        for(int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            uint64_t mask = (1ULL << sq);

            char piece = '.';

            for(int p = PAWN; p <= KING; ++p) {
                for(int c = WHITE; c <= BLACK; ++c) {
                    if(mask & bitboards[p][c]) {
                        piece = symbols[p][c];
                    }
                }
            }

            if(piece != '.') {
                if(cnt != 0) {
                    res += (char)(cnt + '0');
                    cnt = 0;
                }

                res += piece;
            } else {
                cnt++;
            }
        }

        if(cnt > 0) {
            res += (char)(cnt + '0');
            cnt = 0;
        }

        if(rank > 0) {
            res += '/';
        }
    }

    res += ' ';


    // [Active Color]
    if(active_color == WHITE) {
        res += 'w';
    } else {
        res += 'b';
    }

    res += ' ';

    
    // [Castling]
    if(white_can_castle_kingside) {
        res += 'K';
    }

    if(white_can_castle_queenside) {
        res += 'Q';
    }

    if(black_can_castle_kingside) {
        res += 'k';
    }

    if(black_can_castle_queenside) {
        res += 'q';
    }

    if(!(white_can_castle_kingside 
        || white_can_castle_queenside
        || black_can_castle_kingside
        || black_can_castle_queenside)) {
        res += '-';
    }

    res += ' ';


    // [En passant] - square with en passant. - if no en passant capture is available
    if(en_passant_square == -1) {
        res += '-';
    } else {
        int rank = en_passant_square / 8;
        int file = en_passant_square % 8;

        res += (char)(file + 'a');
        res += (char)(rank + '1');
    }

    res += ' ';


    // [Halfmove Clock]
    res += std::to_string(halfmove_clock);

    res += ' ';


    // [Fullmove Number] starts at 1 and increments after Black's turn
    res += std::to_string(fullmove_number);

    return res;
};