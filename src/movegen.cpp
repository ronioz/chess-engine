#include "movegen.hpp"

uint64_t kingAttacks(int square) {
    uint64_t res = 0ULL;
    uint64_t position = (1ULL << square);

    //Bottom 
    if(!(position & rank_1)) {
        res |= (position >> 8);

        if(!(position & file_H)) res |= (position >> 7);
        if(!(position & file_A)) res |= (position >> 9);
    }

    //Top
    if(!(position & rank_8)) {
        res |= (position << 8);

        if(!(position & file_H)) res |= (position << 9);
        if(!(position & file_A)) res |= (position << 7);
    }

    //Left
    if(!(position & file_A)) {
        res |= (position >> 1);
    }

    //Right
    if(!(position & file_H)) {
        res |= (position << 1);
    }

    return res;
}

uint64_t knightAttacks(int square) {
    uint64_t res = 0ULL;
    uint64_t position = (1ULL << square);

    //Up 2 ranks, Right 1 file: +17 (blocked on rank 7-8, or starting on file H)
    //Up 2 ranks, Left 1 file:  +15 (blocked on rank 7-8, or starting on file A)
    if(!(position & (rank_7 | rank_8))) {
        if(!(position & file_H)) res |= (position << 17);
        if(!(position & file_A)) res |= (position << 15);
    }

    //Up 1 rank, Right 2 files:   +10 (blocked on file G-H)
    //Down 1 rank, Right 2 files: -6  (blocked on file G-H)
    if(!(position & (file_G | file_H))) {
        if(!(position & rank_8)) res |= (position << 10);
        if(!(position & rank_1)) res |= (position >> 6);
    }

    //Up 1 rank, Left 2 files:   +6  (blocked on file A-B)
    //Down 1 rank, Left 2 files: -10 (blocked on file A-B)
    if(!(position & (file_A | file_B))) {
        if(!(position & rank_8)) res |= (position << 6);
        if(!(position & rank_1)) res |= (position >> 10);
    }

    //Down 2 ranks, Left 1 file:  -17 (blocked on rank 1-2, or starting on file A)
    //Down 2 ranks, Right 1 file: -15 (blocked on rank 1-2, or starting on file H)
    if(!(position & (rank_1 | rank_2))) {
        if(!(position & file_A)) res |= (position >> 17);
        if(!(position & file_H)) res |= (position >> 15);
    }

    return res;
}