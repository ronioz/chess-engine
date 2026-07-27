#pragma once

#include "types.hpp"
#include <stdint.h>
#include <stdio.h>
#include <string>

/*
Board is represented as 64-bit integer (uint64_t)

SQUARE CALCULATION:
0-th bit: A1
1-st bit: B1
.
.
.
45-th bit: F5
Formula: Bit Index = (Rank * 8) + File
*/

class Board {
public:
    //REPRESENTATION
    uint64_t bitboards[6][2]; // [piece, color]
    uint64_t white_pieces = 0ULL;
    uint64_t black_pieces = 0ULL;
    uint64_t all_pieces = 0ULL;

    //GAMEPLAY
    int active_color = WHITE;
    void switchColor(); //Cycles BLACK and WHITE

    bool white_can_castle_kingside = false;
    bool white_can_castle_queenside = false;
    bool black_can_castle_kingside = false;
    bool black_can_castle_queenside = false;

    int en_passant_square = -1; //-1 means no en passant square
    int halfmove_clock = 0;
    int fullmove_number = 1;

    Board();
    char board[8][8]; //representation with symbols
    void clear(); //empties the board. Everything set to 0
    void setup(); //sets up the pieces. End result is starting position
    void fillBoard(); //fills the board from bitboards to boards[][] matrix
    void printBoard(); //prints the board in terminal

    /*
    FEN is represented as 
    [Piece Placement] each rank is separated by /
    [Active Color] w, b - color
    [Castling] KQ - white can castle (K)ingside and (Q)ueenside, logic applies to blacks too
    [En passant] - square with en passant. - if no en passant capture is available
    [Halfmove Clock]
    [Fullmove Number] starts at 1 and increments after Black's turn

    rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    */
    bool from_FEN(const std::string& FEN);
    std::string to_FEN() const;
};