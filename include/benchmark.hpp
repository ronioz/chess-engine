#pragma once

#include <board.hpp>
#include <search.hpp>
#include <string>

/*
Runs findBestMove on a single position and prints nodes/time/NPS/best move/score.
Clears the transposition table first so results aren't skewed by entries left
over from a previous position or run.
*/
void benchmarkSearch(Board& board, int depth, const std::string& label);

/*
Runs benchmarkSearch over a small fixed suite of standard positions (opening,
complex midgame, endgame, tactical) at a common depth, then prints suite totals.
*/
void runSearchBenchmark(int depth);
