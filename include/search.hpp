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

constexpr int MATE_SCORE = 100000;
constexpr int MATE_THRESHOLD = MATE_SCORE - 1000; //scores at least this close to MATE_SCORE are mate scores, not eval scores
constexpr int MAX_CHECK_EXTENSIONS = 2;

/*
Nega-max algorithm with alpha beta pruning. 
Keeps alpha-beta window updated throughout the whole tree and prunes
*/
int negamax(Board& board, int alpha, int beta, int depth, int ply);

/*
Quiescence search algorithm. 
Used for "tactical" sequences, with many captures/checks, etc. 
*/
int quiescence(Board& board, int alpha, int beta, int ply, int check_extensions);

/*
Finding best move inside depth one-by-one. Starting from 1
Iterative deepening technique, putting best move from previous depth
to the start of moves for next depth
*/
Move findBestMove(Board& board, int depth);

/*
Transposition tables based on Zobrist Hashing
*/
constexpr int TT_SIZE = 1 << 20;
inline TTEntry transposition_table[TT_SIZE];
bool probeTT(uint64_t key, TTEntry& outEntry);
void storeTT(uint64_t key, int score, int depth, int bound, const Move& best_move);