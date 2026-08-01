#pragma once

#include <board.hpp>
#include <evaluation.hpp>
#include <movegen.hpp>
#include <limits.h>
#include <algorithm>
#include <chrono>

/*
Reordering using MVV-LVA (Most valuable victim - Less valuable attacker pruning),
killer moves, and the history heuristic.

Captures score highest (10 * values[captured] - values[moved], offset above every
other band so history growth can never outrank a capture), then the two killer
moves for this ply, then quiet moves by history score, then unscored quiets at -1.
{PAWN = 100, KNIGHT = 290, BISHOP = 310, ROOK = 500, QUEEN = 900, KING = 0}
*/
void reorderLegalMoves(Board& board, std::vector<Move>& legalMoves, int ply);

constexpr int MATE_SCORE = 100000;
constexpr int MATE_THRESHOLD = MATE_SCORE - 1000; //scores at least this close to MATE_SCORE are mate scores, not eval scores
constexpr int MAX_CHECK_EXTENSIONS = 2;

/*
Nega-max algorithm with alpha beta pruning.
Keeps alpha-beta window updated throughout the whole tree and prunes.
extensions tracks how many check extensions have been spent along this path
(capped by MAX_CHECK_EXTENSIONS), mirroring quiescence's check_extensions.
*/
int negamax(Board& board, int alpha, int beta, int depth, int ply, int extensions);

/*
Quiescence search algorithm.
Used for "tactical" sequences, with many captures/checks, etc.
*/
int quiescence(Board& board, int alpha, int beta, int ply, int check_extensions);

/*
Finding best move inside depth one-by-one. Starting from 1
Iterative deepening technique, putting best move from previous depth
to the start of moves for next depth
out_score, if non-null, is set to the final depth's best score (for benchmarking/verification)
time_budget, if not the default (unlimited), stops iterative deepening once the budget is
spent rather than at a fixed depth - depth still acts as a hard upper cap either way. A
depth that doesn't finish inside the budget is discarded wholesale (see search.cpp); the
previous depth's result is returned instead, so a partially-searched depth can never leak
a move that wasn't compared against the rest of that depth's candidates.
*/
Move findBestMove(Board& board, int depth, int* out_score = nullptr,
                   std::chrono::milliseconds time_budget = std::chrono::milliseconds::max());

/*
Total negamax + quiescence node visits since the last resetNodeCount(), for NPS benchmarking
*/
extern uint64_t nodes_searched;
void resetNodeCount();

/*
Killer moves (2 per ply) and the history heuristic ([color][from][to]), used by
reorderLegalMoves to order quiet moves. Killers are only meaningful within a
single search tree, so clearKillers() runs at the start of every findBestMove;
history persists across a game to keep adapting, so it's only cleared on
ucinewgame (clearHistory()).
*/
void clearKillers();
void clearHistory();

/*
Transposition tables based on Zobrist Hashing
*/
constexpr int TT_SIZE = 1 << 20;
inline TTEntry transposition_table[TT_SIZE];
bool probeTT(uint64_t key, TTEntry& outEntry);
void storeTT(uint64_t key, int score, int depth, int bound, const Move& best_move);
void clearTT(); //resets every slot; used between benchmark runs so node counts aren't skewed by a previous position's cached entries