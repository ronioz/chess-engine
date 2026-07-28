#pragma once

struct Move {
    int start;
    int end;
};

struct UndoState {
    Move move;
    int moved_piece;
    int captured_piece;
    int castling_rights;
    int en_passant_square;
    int halfmove_clock;
};