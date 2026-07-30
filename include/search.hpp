#pragma once

#include <board.hpp>
#include <evaluation.hpp>
#include <movegen.hpp>
#include <limits.h>
#include <algorithm>

/*
Reordering using MVV-LVA (Most valuable victim - Less valuable attacker pruning)

The values are: 
{PAWN = 100, KNIGHT = 290, BISHOP = 310, ROOK = 500, QUEEN = 900, KING = 0}

If no capture occurs, then the score is set to -1
The formula is: 10 * values[captured] - values[moved]
*/
void reorderLegalMoves(Board& board, std::vector<Move>& legalMoves);
int negamax(Board& board, int alpha, int beta, int depth);
Move findBestMove(Board& board, int depth);