#include <uci.hpp>
#include <board.hpp>
#include <fen_conversion.hpp>
#include <movegen.hpp>
#include <search.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string squareToStr(int square) {
    std::string s;
    s += char('a' + (square % 8));
    s += char('1' + (square / 8));
    return s;
}

std::string moveToStr(const Move& move) {
    constexpr char promo_chars[6] = {'p', 'n', 'b', 'r', 'q', 'k'};

    std::string s = squareToStr(move.start) + squareToStr(move.end);
    if(move.promotion != -1) {
        s += promo_chars[move.promotion];
    }
    return s;
}

//UCI moves are bare start/end squares (+ optional promotion letter) with no
//piece or flag info, so the only way to turn one into a Move is to match it
//against the position's actual legal moves.
bool findLegalMove(Board& board, const std::string& uci_move, Move& out) {
    constexpr char promo_chars[6] = {'p', 'n', 'b', 'r', 'q', 'k'};

    std::vector<Move> moves;
    generateLegalMoves(board, moves);

    for(const Move& move : moves) {
        if(squareToStr(move.start) + squareToStr(move.end) != uci_move.substr(0, 4)) {
            continue;
        }

        if(uci_move.size() == 5) {
            if(move.promotion == -1 || promo_chars[move.promotion] != uci_move[4]) {
                continue;
            }
        } else if(move.promotion != -1) {
            continue;
        }

        out = move;
        return true;
    }

    return false;
}

void handlePosition(Board& board, std::istringstream& iss) {
    std::string token;
    iss >> token;

    if(token == "startpos") {
        board.setup();
        iss >> token; //consumes "moves" if present, otherwise leaves token stale from EOF
    } else if(token == "fen") {
        std::vector<std::string> fen_fields;
        while(iss >> token && token != "moves") {
            fen_fields.push_back(token);
        }

        std::string fen;
        for(size_t i = 0; i < fen_fields.size(); ++i) {
            if(i) fen += ' ';
            fen += fen_fields[i];
        } //joined without a trailing space - from_FEN reads the fullmove field to end-of-string

        from_FEN(board, fen);
    }

    if(token == "moves") {
        while(iss >> token) {
            Move move;
            if(findLegalMove(board, token, move)) {
                board.makeMove(move);
            }
        }
    }
}

void handleGo(Board& board, std::istringstream& iss) {
    int depth = 6; //fallback for a bare "go" - real strength control comes from the caller sending "go depth N"
    std::string token;
    while(iss >> token) {
        if(token == "depth") {
            iss >> depth;
        }
    }

    Move best = findBestMove(board, depth);

    //start == -1 is findBestMove's sentinel for "no legal move" (checkmate/stalemate);
    //"0000" is the standard UCI null-move token for that case.
    std::cout << "bestmove " << (best.start == -1 ? "0000" : moveToStr(best)) << std::endl;
}

} //namespace

void runUCI() {
    Board board;
    board.setup();

    std::string line;
    while(std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if(command == "uci") {
            std::cout << "id name ChessEngine" << std::endl;
            std::cout << "id author roniosipov" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if(command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if(command == "ucinewgame") {
            clearTT();
            board.setup();
        } else if(command == "position") {
            handlePosition(board, iss);
        } else if(command == "go") {
            handleGo(board, iss);
        } else if(command == "quit") {
            break;
        }
    }
}
