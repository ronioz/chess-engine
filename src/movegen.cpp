#include "movegen.hpp"
#include <array>

static uint64_t computeKingAttacks(int square) {
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

static uint64_t computeKnightAttacks(int square) {
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

static std::array<uint64_t, 64> buildAttackTable(uint64_t (*compute)(int)) {
    std::array<uint64_t, 64> table{};
    for(int square = 0; square < 64; ++square) table[square] = compute(square);
    return table;
}

static const std::array<uint64_t, 64> KING_ATTACK_TABLE = buildAttackTable(computeKingAttacks);
static const std::array<uint64_t, 64> KNIGHT_ATTACK_TABLE = buildAttackTable(computeKnightAttacks);

uint64_t kingAttacks(int square) {
    return KING_ATTACK_TABLE[square];
}

uint64_t knightAttacks(int square) {
    return KNIGHT_ATTACK_TABLE[square];
}

bool isSquareAttacked(const Board& board, int square, int by_color) {
    if(knightAttacks(square) & board.bitboards[KNIGHT][by_color]) return true;
    if(kingAttacks(square) & board.bitboards[KING][by_color]) return true;

    uint64_t pawns = board.bitboards[PAWN][by_color];
    uint64_t pawn_attacks = (by_color == WHITE)
        ? (((pawns & ~file_A) << 7) | ((pawns & ~file_H) << 9))
        : (((pawns & ~file_A) >> 9) | ((pawns & ~file_H) >> 7));
    if(pawn_attacks & (1ULL << square)) return true;

    //Diagonals: +7, -9 (file-1, stop before file A); +9, -7 (file+1, stop before file H)
    //Straight:  +8, -8 (file never changes); +1 (file+1, stop before file H); -1 (file-1, stop before file A)
    static constexpr int offsets[8] = {7, -9, 9, -7, 8, -8, 1, -1};
    static constexpr uint64_t edge_files[8] = {file_A, file_A, file_H, file_H, 0ULL, 0ULL, file_H, file_A};

    for(int d = 0; d < 8; ++d) {
        int offset = offsets[d];
        uint64_t edge_file = edge_files[d];

        int temp = square;
        while(true) {
            if((1ULL << temp) & edge_file) break;

            temp += offset;
            if(temp < 0 || temp >= 64) break;

            uint64_t mask = (1ULL << temp);

            if(mask & board.all_pieces) {
                int slider = (d < 4) ? BISHOP : ROOK;

                if((mask & board.bitboards[slider][by_color]) || (mask & board.bitboards[QUEEN][by_color])) {
                    return true;
                }

                break;
            }
        }
    }

    return false;
}

bool isInCheck(const Board& board, int color) {
    int king_square = __builtin_ctzll(board.bitboards[KING][color]);
    int enemy_color = (color == WHITE) ? BLACK : WHITE;

    return isSquareAttacked(board, king_square, enemy_color);
}

static void addPawnMove(std::vector<Move>& moves, int start, int dest, uint64_t promotion_rank, int flags) {
    if((1ULL << dest) & promotion_rank) {
        moves.push_back({start, dest, KNIGHT, flags});
        moves.push_back({start, dest, BISHOP, flags});
        moves.push_back({start, dest, ROOK, flags});
        moves.push_back({start, dest, QUEEN, flags});
    } else {
        moves.push_back({start, dest, -1, flags});
    }
}

void generatePawnMoves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    int color = board.active_color;
    uint64_t pawns = board.bitboards[PAWN][color];
    uint64_t empty = ~board.all_pieces;
    uint64_t enemy = (color == WHITE) ? board.black_pieces : board.white_pieces;
    uint64_t ep_bb = (board.en_passant_square == -1) ? 0ULL : (1ULL << board.en_passant_square);

    uint64_t promotion_rank = (color == WHITE) ? rank_8 : rank_1;
    uint64_t start_rank = (color == WHITE) ? rank_2 : rank_7;
    int push = (color == WHITE) ? 8 : -8;

    if(!captures_only) {
        //Single push
        uint64_t single_push = (color == WHITE) ? ((pawns << 8) & empty) : ((pawns >> 8) & empty);

        while(single_push) {
            int dest = __builtin_ctzll(single_push);
            addPawnMove(moves, dest - push, dest, promotion_rank, 0);
            single_push &= (single_push - 1);
        }

        //Double push (both the intermediate square and destination must be empty)
        uint64_t double_push = (color == WHITE)
            ? (((pawns & start_rank) << 16) & empty & (empty << 8))
            : (((pawns & start_rank) >> 16) & empty & (empty >> 8));

        while(double_push) {
            int dest = __builtin_ctzll(double_push);
            moves.push_back({dest - 2 * push, dest, -1, DOUBLE_PAWN_PUSH});
            double_push &= (double_push - 1);
        }
    }

    //Captures: file-1 diagonal and file+1 diagonal
    int cap_minus_shift = (color == WHITE) ? 7 : -9; //file-1
    int cap_plus_shift  = (color == WHITE) ? 9 : -7; //file+1

    uint64_t cap_minus_targets = (color == WHITE) ? ((pawns & ~file_A) << 7) : ((pawns & ~file_A) >> 9);
    uint64_t cap_plus_targets  = (color == WHITE) ? ((pawns & ~file_H) << 9) : ((pawns & ~file_H) >> 7);

    uint64_t cap_minus_hits = cap_minus_targets & enemy;
    uint64_t cap_plus_hits  = cap_plus_targets & enemy;

    while(cap_minus_hits) {
        int dest = __builtin_ctzll(cap_minus_hits);
        addPawnMove(moves, dest - cap_minus_shift, dest, promotion_rank, CAPTURE);
        cap_minus_hits &= (cap_minus_hits - 1);
    }

    while(cap_plus_hits) {
        int dest = __builtin_ctzll(cap_plus_hits);
        addPawnMove(moves, dest - cap_plus_shift, dest, promotion_rank, CAPTURE);
        cap_plus_hits &= (cap_plus_hits - 1);
    }

    //En passant
    if(ep_bb & cap_minus_targets) {
        moves.push_back({board.en_passant_square - cap_minus_shift, board.en_passant_square, -1, CAPTURE | EN_PASSANT});
    }

    if(ep_bb & cap_plus_targets) {
        moves.push_back({board.en_passant_square - cap_plus_shift, board.en_passant_square, -1, CAPTURE | EN_PASSANT});
    }
}

void generateKnightMoves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    int color = board.active_color;
    uint64_t knights = board.bitboards[KNIGHT][color];

    uint64_t friendly_pieces = (color == WHITE) ? board.white_pieces : board.black_pieces;
    uint64_t enemy_pieces = (color == WHITE) ? board.black_pieces : board.white_pieces;

    while(knights) {
        int start = __builtin_ctzll(knights);
        uint64_t knight_mask = knightAttacks(start);

        while(knight_mask) {
            int end = __builtin_ctzll(knight_mask);

            Move move;
            move.start = start;
            move.end = end;
            move.promotion = -1;
            move.flags = 0;

            if((1ULL << end) & enemy_pieces) {
                move.flags |= CAPTURE;
            }

            if(!((1ULL << end) & friendly_pieces) && (!captures_only || (move.flags & CAPTURE))) {
                moves.push_back(move);
            }

            knight_mask &= (knight_mask - 1);
        }

        knights &= (knights - 1);
    }
}

void generateKingMoves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    int color = board.active_color;
    uint64_t king = board.bitboards[KING][color];

    uint64_t friendly_pieces = (color == WHITE) ? board.white_pieces : board.black_pieces;
    uint64_t enemy_pieces = (color == WHITE) ? board.black_pieces : board.white_pieces;

    int start = __builtin_ctzll(king);
    uint64_t king_mask = kingAttacks(start);

    while(king_mask) {
        int end = __builtin_ctzll(king_mask);

        Move move;
        move.start = start;
        move.end = end;
        move.promotion = -1;
        move.flags = 0;

        if((1ULL << end) & enemy_pieces) {
            move.flags |= CAPTURE;
        }

        if(!((1ULL << end) & friendly_pieces) && (!captures_only || (move.flags & CAPTURE))) {
            moves.push_back(move);
        }

        king_mask &= (king_mask - 1);
    }

    if(captures_only) return; //castling is never a capture

    //Castling: rights are cleared whenever the king/rook moves or the rook is captured,
    //so a set bit already implies both are on their home squares. The destination square
    //is checked by legal-move filtering (post-move isInCheck); the king's current square
    //and the square it transits are checked here, since nothing else ever re-visits them.
    int enemy_color = (color == WHITE) ? BLACK : WHITE;

    if(color == WHITE) {
        if((board.castling_rights & 8) && !(board.all_pieces & ((1ULL << 5) | (1ULL << 6)))
           && !isSquareAttacked(board, 4, enemy_color) && !isSquareAttacked(board, 5, enemy_color)) {
            moves.push_back({start, 6, -1, CASTLE});
        }
        if((board.castling_rights & 4) && !(board.all_pieces & ((1ULL << 1) | (1ULL << 2) | (1ULL << 3)))
           && !isSquareAttacked(board, 4, enemy_color) && !isSquareAttacked(board, 3, enemy_color)) {
            moves.push_back({start, 2, -1, CASTLE});
        }
    } else {
        if((board.castling_rights & 2) && !(board.all_pieces & ((1ULL << 61) | (1ULL << 62)))
           && !isSquareAttacked(board, 60, enemy_color) && !isSquareAttacked(board, 61, enemy_color)) {
            moves.push_back({start, 62, -1, CASTLE});
        }
        if((board.castling_rights & 1) && !(board.all_pieces & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59)))
           && !isSquareAttacked(board, 60, enemy_color) && !isSquareAttacked(board, 59, enemy_color)) {
            moves.push_back({start, 58, -1, CASTLE});
        }
    }
}

static void generateSlidingMoves(const Board& board, std::vector<Move>& moves,
                                  uint64_t pieces, const int* offsets,
                                  const uint64_t* edge_files, int dir_count,
                                  bool captures_only);

void generateBishopMoves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    static constexpr int offsets[4] = {7, -9, 9, -7};
    static constexpr uint64_t edge_files[4] = {file_A, file_A, file_H, file_H};
    generateSlidingMoves(board, moves, board.bitboards[BISHOP][board.active_color], offsets, edge_files, 4, captures_only);
}

void generateRookMoves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    static constexpr int offsets[4] = {8, -8, 1, -1};
    static constexpr uint64_t edge_files[4] = {0ULL, 0ULL, file_H, file_A};
    generateSlidingMoves(board, moves, board.bitboards[ROOK][board.active_color], offsets, edge_files, 4, captures_only);
}

void generateQueenMoves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    static constexpr int offsets[8] = {7, -9, 9, -7, 8, -8, 1, -1};
    static constexpr uint64_t edge_files[8] = {file_A, file_A, file_H, file_H, 0ULL, 0ULL, file_H, file_A};
    generateSlidingMoves(board, moves, board.bitboards[QUEEN][board.active_color], offsets, edge_files, 8, captures_only);
}

void generatePseudoLegalMoves(const Board& board, std::vector<Move>& moves) {
    generatePawnMoves(board, moves);
    generateKnightMoves(board, moves);
    generateKingMoves(board, moves);
    generateBishopMoves(board, moves);
    generateRookMoves(board, moves);
    generateQueenMoves(board, moves);
}

void generatePseudoLegalCaptures(const Board& board, std::vector<Move>& moves) {
    generatePawnMoves(board, moves, true);
    generateKnightMoves(board, moves, true);
    generateKingMoves(board, moves, true);
    generateBishopMoves(board, moves, true);
    generateRookMoves(board, moves, true);
    generateQueenMoves(board, moves, true);
}

//Squares/pieces relevant to whether the side to move's king is safe, computed once
//per node instead of once per candidate move. checkers_count >= 2 means only king
//moves can be legal; check_mask (valid when checkers_count == 1) is the set of
//squares a non-king move must land on to capture or block the sole checker; pinned
//marks friendly pieces pinned to the king, and pin_ray[sq] is the only line a piece
//pinned on sq may move along without exposing the king.
struct CheckInfo {
    int checkers_count = 0;
    uint64_t check_mask = ~0ULL;
    uint64_t pinned = 0ULL;
    uint64_t pin_ray[64];
};

static CheckInfo computeCheckInfo(const Board& board, int color) {
    CheckInfo info;

    int king_square = __builtin_ctzll(board.bitboards[KING][color]);
    int enemy_color = (color == WHITE) ? BLACK : WHITE;
    uint64_t king_bb = 1ULL << king_square;
    uint64_t friendly_pieces = (color == WHITE) ? board.white_pieces : board.black_pieces;

    //Knight/pawn checks can only be resolved by capturing the checker, never by blocking.
    uint64_t knight_checkers = knightAttacks(king_square) & board.bitboards[KNIGHT][enemy_color];
    if(knight_checkers) {
        info.checkers_count++;
        info.check_mask = knight_checkers;
    }

    uint64_t enemy_pawns = board.bitboards[PAWN][enemy_color];
    uint64_t pawn_checkers = (enemy_color == WHITE)
        ? (((king_bb >> 7) & enemy_pawns & ~file_A) | ((king_bb >> 9) & enemy_pawns & ~file_H))
        : (((king_bb << 9) & enemy_pawns & ~file_A) | ((king_bb << 7) & enemy_pawns & ~file_H));
    if(pawn_checkers) {
        info.checkers_count++;
        info.check_mask = pawn_checkers;
    }

    //Sliding checkers and pins: walk each ray outward from the king. The first
    //occupied square is either an enemy slider (check) or a friendly piece (possibly
    //pinned, if a matching enemy slider is the very next occupied square on the ray).
    static constexpr int offsets[8] = {7, -9, 9, -7, 8, -8, 1, -1};
    static constexpr uint64_t edge_files[8] = {file_A, file_A, file_H, file_H, 0ULL, 0ULL, file_H, file_A};

    for(int d = 0; d < 8; ++d) {
        int offset = offsets[d];
        uint64_t edge_file = edge_files[d];
        int slider_type = (d < 4) ? BISHOP : ROOK;

        uint64_t ray = 0ULL;
        int blocker_square = -1;
        int temp = king_square;

        while(true) {
            if((1ULL << temp) & edge_file) break;

            temp += offset;
            if(temp < 0 || temp >= 64) break;

            uint64_t mask = (1ULL << temp);
            ray |= mask;

            if(!(mask & board.all_pieces)) continue;

            bool enemy_slider = (mask & board.bitboards[slider_type][enemy_color]) || (mask & board.bitboards[QUEEN][enemy_color]);

            if(blocker_square == -1) {
                if(mask & friendly_pieces) {
                    blocker_square = temp;
                    continue;
                }

                if(enemy_slider) {
                    info.checkers_count++;
                    info.check_mask = ray;
                }

                break;
            }

            if(enemy_slider) {
                info.pinned |= (1ULL << blocker_square);
                info.pin_ray[blocker_square] = ray;
            }

            break;
        }
    }

    return info;
}

static void filterLegalMoves(Board& board, const std::vector<Move>& pseudoLegalMoves, std::vector<Move>& out) {
    out.clear();
    int mover = board.active_color;
    uint64_t king_bb = board.bitboards[KING][mover];
    CheckInfo info = computeCheckInfo(board, mover);

    for(const auto& move : pseudoLegalMoves) {
        uint64_t start_bb = (1ULL << move.start);
        bool is_king_move = (start_bb & king_bb) != 0;
        bool is_en_passant = (move.flags & EN_PASSANT) != 0;

        if(is_king_move || is_en_passant) {
            //King moves (incl. castling) and en passant have edge cases the mask
            //shortcuts below don't model - a king retreating along the very ray
            //it's checked on, and the rare horizontal discovered check when both
            //en passant pawns vanish at once - so they're verified via make/unmake.
            UndoState state = board.makeMove(move);
            if(!isInCheck(board, mover)) out.push_back(move);
            board.unmakeMove(state);
            continue;
        }

        if(info.checkers_count >= 2) continue; //double check: only the king can move

        uint64_t dest_bb = (1ULL << move.end);

        if(info.checkers_count == 1 && !(dest_bb & info.check_mask)) continue; //must capture/block the checker

        if((info.pinned & start_bb) && !(dest_bb & info.pin_ray[move.start])) continue; //pinned piece must stay on the pin ray

        out.push_back(move);
    }
}

void generateLegalMoves(Board& board, std::vector<Move>& out) {
    out.clear();
    static std::vector<Move> pseudoLegalMoves;
    pseudoLegalMoves.clear();
    generatePseudoLegalMoves(board, pseudoLegalMoves);

    filterLegalMoves(board, pseudoLegalMoves, out);    
}

void generateLegalCaptures(Board& board, std::vector<Move>& out) {
    static std::vector<Move> pseudoLegalCaptures;
    pseudoLegalCaptures.clear();
    generatePseudoLegalCaptures(board, pseudoLegalCaptures);
    filterLegalMoves(board, pseudoLegalCaptures, out);
}

static void generateSlidingMoves(const Board& board, std::vector<Move>& moves,
uint64_t pieces, const int* offsets,
const uint64_t* edge_files, int dir_count, bool captures_only) {
    int color = board.active_color;
    uint64_t friendly_pieces = (color == WHITE) ? board.white_pieces : board.black_pieces;
    uint64_t enemy_pieces = (color == WHITE) ? board.black_pieces : board.white_pieces;

    while(pieces) {
        int start = __builtin_ctzll(pieces);

        for(int d = 0; d < dir_count; ++d) {
            int offset = offsets[d];
            uint64_t edge_file = edge_files[d];
            int temp = start;

            while(true) {
                if((1ULL << temp) & edge_file) break;
                temp += offset;
                if(temp < 0 || temp >= 64) break;

                uint64_t mask = (1ULL << temp);
                Move move{start, temp, -1, 0};

                if(mask & friendly_pieces) break;
                if(mask & enemy_pieces) {
                    move.flags |= CAPTURE;
                    moves.push_back(move);
                    break;
                }

                //Empty square: the ray keeps walking regardless (a capture may still be
                //further out), but the quiet move itself is only recorded when wanted.
                if(!captures_only) moves.push_back(move);
            }
        }

        pieces &= (pieces - 1);
    }
}