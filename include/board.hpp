#pragma once

#include "types.hpp"
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <random>

/*
Board is represented as 64-bit integer (uint64_t)

SQUARE CALCULATION:
0-th bit: A1
1-st bit: B1
.
.
.
45-th bit: F6
Formula: Bit Index = (Rank * 8) + File
*/

class Board {
public:
    //REPRESENTATION
    uint64_t bitboards[6][2]; // [piece, color]
    uint64_t white_pieces = 0ULL;
    uint64_t black_pieces = 0ULL;
    uint64_t all_pieces = 0ULL;
    int piece_on_square[64]; // mailbox, -1 if empty; kept in sync by setPiece/clearPiece

    //GAMEPLAY
    int active_color = WHITE;
    void switchColor(); //Cycles BLACK and WHITE

    int castling_rights = 0;
    int en_passant_square = -1; //-1 means no en passant square
    int halfmove_clock = 0;
    int fullmove_number = 1;

    uint64_t zobrist_key;

    Board();
    char board[8][8]; //representation with symbols
    void clear(); //empties the board. Everything set to 0
    void setup(); //sets up the pieces. End result is starting position
    void fillBoard(); //fills the board from bitboards to boards[][] matrix
    void printBoard(); //prints the board in terminal
    void updateBitboards();

    char symbols[6][2] = {
        {'P', 'p'},
        {'N', 'n'},
        {'B', 'b'},
        {'R', 'r'},
        {'Q', 'q'},
        {'K', 'k'}
    };

    int eval_score = 0;
    UndoState makeMove(const Move& move);
    void unmakeMove(const UndoState& state);
    NullUndoState makeNullMove(); //passes the turn without moving a piece, for null-move pruning
    void unmakeNullMove(const NullUndoState& state);
    void setPiece(int piece, int color, int sq);
    void clearPiece(int piece, int color, int sq);
    void initEvalScore(); //recomputes eval_score from scratch; call after placing pieces outside setPiece (setup/from_FEN)
    void initPieceOnSquare(); //recomputes piece_on_square from scratch; call after placing pieces outside setPiece (setup/from_FEN)

    /*
    Zobrist Hashing
    */
   inline static uint64_t zobrist_pieces[6][2][64]; // [piece_type][color][square]
   inline static uint64_t zobrist_side_to_move;
   inline static uint64_t zobrist_castling[16];
   inline static uint64_t zobrist_ep_file[8];
   inline static bool zobrist_tables_ready = false;
   void initZobrist();
   uint64_t initZobristKey();
};