#include <iostream>
#include <string>
#include <board.hpp>
#include <movegen.hpp>
#include <search.hpp>

static int squareFromAlgebraic(const std::string& s) {
    if(s.size() != 2) return -1;

    int file = s[0] - 'a';
    int rank = s[1] - '1';

    if(file < 0 || file > 7 || rank < 0 || rank > 7) return -1;

    return rank * 8 + file;
}

static int promotionFromChar(char c) {
    switch(c) {
        case 'q': case 'Q': return QUEEN;
        case 'r': case 'R': return ROOK;
        case 'b': case 'B': return BISHOP;
        case 'n': case 'N': return KNIGHT;
        default: return -1;
    }
}

int main() {
    const int SEARCH_DEPTH = 5;

    Board board;
    board.setup();

    while(true) {
        board.fillBoard();
        board.printBoard();

        std::vector<Move> legal = generateLegalMoves(board);

        if(legal.empty()) {
            if(isInCheck(board, board.active_color)) {
                printf("Checkmate. %s wins.\n", board.active_color == WHITE ? "Black" : "White");
            } else {
                printf("Stalemate. Draw.\n");
            }
            break;
        }

        Move chosen;

        if(board.active_color == WHITE) {
            printf("Your move (e.g. e2e4, e7e8q, or quit): ");

            std::string input;
            if(!std::getline(std::cin, input) || input == "quit") {
                break;
            }

            if(input.size() < 4) {
                printf("Invalid input.\n");
                continue;
            }

            int start = squareFromAlgebraic(input.substr(0, 2));
            int end = squareFromAlgebraic(input.substr(2, 2));
            int promo = (input.size() >= 5) ? promotionFromChar(input[4]) : -1;

            if(start == -1 || end == -1) {
                printf("Invalid squares.\n");
                continue;
            }

            bool found = false;

            for(const auto& move : legal) {
                if(move.start == start && move.end == end && move.promotion == promo) {
                    chosen = move;
                    found = true;
                    break;
                }
            }

            if(!found) {
                printf("Illegal move.\n");
                continue;
            }
        } else {
            printf("Engine is thinking...\n");
            chosen = findBestMove(board, SEARCH_DEPTH);
        }

        board.makeMove(chosen);
    }

    return 0;
}
