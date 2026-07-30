#pragma once

#include <board.hpp>
#include <evaluation.hpp>
#include <movegen.hpp>
#include <limits.h>
#include <algorithm>

void reorderLegalMoves(Board& board, std::vector<Move>& legalMoves);
int negamax(Board& board, int alpha, int beta, int depth);
Move findBestMove(Board& board, int depth);