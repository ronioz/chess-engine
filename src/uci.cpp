#include <uci.hpp>
#include <board.hpp>
#include <fen_conversion.hpp>
#include <movegen.hpp>
#include <search.hpp>
#include <algorithm>
#include <chrono>
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

//Simple fixed-divisor time allocator: no game-phase adjustment, no panic time,
//just "assume DEFAULT_MOVES_TO_GO moves left unless told otherwise, use a
//fraction of the remaining clock plus this move's increment, leave a safety
//margin so engine-side overhead can't flag the clock." findBestMove's own
//deadline check (every 2048 nodes) is what actually enforces the budget.
constexpr long long SAFETY_MARGIN_MS = 50;
constexpr long long MIN_TIME_MS = 10;
constexpr int DEFAULT_MOVES_TO_GO = 30;
constexpr int MAX_TIME_DEPTH = 64; //hard depth backstop once the clock is the real limiter

void handleGo(Board& board, std::istringstream& iss) {
    int depth = -1; //-1: not explicitly given
    long long movetime = -1, wtime = -1, btime = -1, winc = 0, binc = 0;
    int movestogo = -1;
    std::string token;

    while(iss >> token) {
        if(token == "depth") {
            iss >> depth;
        } else if(token == "movetime") {
            iss >> movetime;
        } else if(token == "wtime") {
            iss >> wtime;
        } else if(token == "btime") {
            iss >> btime;
        } else if(token == "winc") {
            iss >> winc;
        } else if(token == "binc") {
            iss >> binc;
        } else if(token == "movestogo") {
            iss >> movestogo;
        }
    }

    bool has_time_control = (movetime >= 0) || (wtime >= 0 && btime >= 0);
    Move best;

    if(has_time_control) {
        long long budget_ms;

        if(movetime >= 0) {
            budget_ms = movetime - SAFETY_MARGIN_MS;
        } else {
            long long time_left = (board.active_color == WHITE) ? wtime : btime;
            long long inc = (board.active_color == WHITE) ? winc : binc;
            int moves_to_go = (movestogo > 0) ? movestogo : DEFAULT_MOVES_TO_GO;

            budget_ms = time_left / moves_to_go + inc - SAFETY_MARGIN_MS;
            budget_ms = std::min(budget_ms, time_left - SAFETY_MARGIN_MS);
        }

        budget_ms = std::max(budget_ms, MIN_TIME_MS);

        int search_depth = (depth > 0) ? depth : MAX_TIME_DEPTH;
        best = findBestMove(board, search_depth, nullptr, std::chrono::milliseconds(budget_ms));
    } else {
        //fallback for a bare "go" (or "go depth N") - no clock info at all,
        //so strength is driven purely by depth, as before.
        best = findBestMove(board, depth > 0 ? depth : 6);
    }

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
            clearKillers();
            clearHistory();
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
