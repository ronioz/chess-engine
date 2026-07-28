#pragma once

#include "board.hpp"
#include <vector>

constexpr uint64_t file_G = 0x4040404040404040ULL;
constexpr uint64_t file_H = 0x8080808080808080ULL;

constexpr uint64_t file_A = 0x0101010101010101ULL;
constexpr uint64_t file_B = 0x0202020202020202ULL;

constexpr uint64_t rank_1 = 0x00000000000000FFULL;
constexpr uint64_t rank_2 = 0x000000000000FF00ULL;

constexpr uint64_t rank_7 = 0x00FF000000000000ULL;
constexpr uint64_t rank_8 = 0xFF00000000000000ULL;

uint64_t kingAttacks(int square);
uint64_t knightAttacks(int square);

std::vector<Move> generatePawnMoves(const Board& board);
std::vector<Move> generateKnightMoves(const Board& board);
std::vector<Move> generateKingMoves(const Board& board);
std::vector<Move> generateBishopMoves(const Board& board);
std::vector<Move> generateRookMoves(const Board& board);
std::vector<Move> generateQueenMoves(const Board& board);

std::vector<Move> generatePseudoLegalMoves(const Board& board);
std::vector<Move> generateLegalMoves(const Board& board);