#pragma once

#include "board.hpp"
#include <vector>

//File / rank masks

constexpr uint64_t file_A = 0x0101010101010101ULL;
constexpr uint64_t file_B = 0x0202020202020202ULL;
constexpr uint64_t file_G = 0x4040404040404040ULL;
constexpr uint64_t file_H = 0x8080808080808080ULL;

constexpr uint64_t rank_1 = 0x00000000000000FFULL;
constexpr uint64_t rank_2 = 0x000000000000FF00ULL;
constexpr uint64_t rank_7 = 0x00FF000000000000ULL;
constexpr uint64_t rank_8 = 0xFF00000000000000ULL;

//Attack tables

uint64_t kingAttacks(int square);
uint64_t knightAttacks(int square);

//Check detection

bool isSquareAttacked(const Board& board, int square, int by_color);
bool isInCheck(const Board& board, int color);

//Pseudo-legal move generation, per piece type
//Each appends onto the caller-owned vector instead of returning a new one, so
//generatePseudoLegalMoves can fill a single buffer rather than allocating one per piece type.
//captures_only skips quiet-move push_backs (sliding pieces still walk through empty squares
//to reach a capture further down the ray - only the push_back itself is gated).

void generatePawnMoves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
void generateKnightMoves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
void generateKingMoves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
void generateBishopMoves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
void generateRookMoves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
void generateQueenMoves(const Board& board, std::vector<Move>& moves, bool captures_only = false);

//Aggregate move generation

void generatePseudoLegalMoves(const Board& board, std::vector<Move>& moves);
void generatePseudoLegalCaptures(const Board& board, std::vector<Move>& moves);
void generateLegalMoves(Board& board, std::vector<Move>& out);
void generateLegalCaptures(Board& board, std::vector<Move>& out);