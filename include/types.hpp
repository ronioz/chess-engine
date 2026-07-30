#pragma once

#include <cstdint>

enum Color : int {
    WHITE = 0,
    BLACK = 1
};

enum Pieces : int {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5
};

enum Flag : int {
    CAPTURE = 8,
    CASTLE = 4,
    EN_PASSANT = 2,
    DOUBLE_PAWN_PUSH = 1
};

enum Bound : int {
    EXACT = 0,
    LOWER = 1,
    UPPER = 2
};

struct Move {
    int start;
    int end;
    int promotion = -1;
    int flags = 0;
};

struct UndoState {
    Move move;
    int moved_piece;
    int captured_piece;
    int castling_rights;
    int en_passant_square;
    int halfmove_clock;
    uint64_t zobrist_key;
};

struct TTEntry {
    uint64_t key;
    int score;
    int depth = -1;
    int bound;
    Move best_move;
};