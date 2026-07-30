#pragma once

#include <board.hpp>
#include <movegen.hpp>
#include <vector>
#include <iostream>
#include <chrono>
#include <iomanip>

uint64_t perft(Board& board, int depth);
void benchmarkPerft(Board& board, int depth);