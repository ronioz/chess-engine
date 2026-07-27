#pragma once

#include <string>
#include <board.hpp>

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
bool from_FEN(Board& board, const std::string& FEN); //turns FEN to bitboards
std::string to_FEN(const Board& board); //turns bitboards to FEN