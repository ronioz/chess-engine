#include <fen_conversion.hpp>

bool from_FEN(Board& board, const std::string& FEN) {
    board.clear();

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

        board.bitboards[piece][color] |= mask;

        if(color == WHITE) board.white_pieces |= mask;
        else board.black_pieces |= mask;

        file++;
    }

    board.all_pieces = board.white_pieces | board.black_pieces;
    board.fillBoard();

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

    board.active_color = (token == "w") ? WHITE : BLACK;

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

    board.castling_rights = 0;

    if(token != "-") {
        for(int j = 0; j < (int)token.size(); ++j) {
            if(token[j] == 'K') board.castling_rights |= 8;
            else if(token[j] == 'Q') board.castling_rights |= 4;
            else if(token[j] == 'k') board.castling_rights |= 2;
            else if(token[j] == 'q') board.castling_rights |= 1;
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
        board.en_passant_square = -1;
    } else if(token.size() == 2 && token[0] >= 'a' && token[0] <= 'h' && token[1] >= '1' && token[1] <= '8') {
        int ep_file = token[0] - 'a';
        int ep_rank = token[1] - '1';
        board.en_passant_square = ep_rank * 8 + ep_file;
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

    board.halfmove_clock = std::stoi(token);

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

    board.fullmove_number = std::stoi(token);
    board.initZobrist();
    board.initEvalScore();
    board.initPieceOnSquare();

    return true;
}

std::string to_FEN(const Board& board) {
    std::string res = "";

    // [PIECE PLACEMENT]
    int cnt = 0;

    for(int rank = 7; rank >= 0; --rank) {
        for(int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            uint64_t mask = (1ULL << sq);

            char piece = '.';

            for(int p = PAWN; p <= KING; ++p) {
                for(int c = WHITE; c <= BLACK; ++c) {
                    if(mask & board.bitboards[p][c]) {
                        piece = board.symbols[p][c];
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


    // [ACTIVE COLOR]
    if(board.active_color == WHITE) {
        res += 'w';
    } else {
        res += 'b';
    }

    res += ' ';


    // [CASTLING]
    if(board.castling_rights & 8) {
        res += 'K';
    }

    if(board.castling_rights & 4) {
        res += 'Q';
    }

    if(board.castling_rights & 2) {
        res += 'k';
    }

    if(board.castling_rights & 1) {
        res += 'q';
    }

    if(!board.castling_rights) {
        res += '-';
    }

    res += ' ';


    // [EN PASSANT]
    if(board.en_passant_square == -1) {
        res += '-';
    } else {
        int rank = board.en_passant_square / 8;
        int file = board.en_passant_square % 8;

        res += (char)(file + 'a');
        res += (char)(rank + '1');
    }

    res += ' ';


    // [HALFMOVE CLOCK]
    res += std::to_string(board.halfmove_clock);

    res += ' ';


    // [FULLMOVE NUMBER]
    res += std::to_string(board.fullmove_number);

    return res;
}
