#include <evaluation.hpp>

int evaluate_board(const Board& board) {
    int res = 0;

    for(int p = PAWN; p <= KING; ++p) {
        uint64_t temp = board.bitboards[p][WHITE];

        while(temp) {
            int sq = __builtin_ctzll(temp);

            res += piece_value[p];
            res += piece_pst[p][sq];

            temp &= (temp - 1);
        }
    }

    for(int p = PAWN; p <= KING; ++p) {
        uint64_t temp = board.bitboards[p][BLACK];

        while(temp) {
            int sq = __builtin_ctzll(temp) ^ 56;

            res -= piece_value[p];
            res -= piece_pst[p][sq];

            temp &= (temp - 1);
        }
    }

    return res;
}