#include <board.hpp>
#include <movegen.hpp>
#include <fen_conversion.hpp>
#include <cassert>
#include <cstdio>
#include <set>

bool hasMove(const std::vector<Move>& moves, int start, int end, int promotion, int flags) {
    for(const Move& m : moves) {
        if(m.start == start && m.end == end && m.promotion == promotion && m.flags == flags) return true;
    }
    return false;
}

std::set<int> moveEnds(const std::vector<Move>& moves) {
    std::set<int> ends;
    for(const Move& m : moves) ends.insert(m.end);
    return ends;
}

//[GROUND-TRUTH GEOMETRY, independent of movegen.cpp's own bitboard logic]

void expectedKingSquares(int sq, std::set<int>& out) {
    int r = sq / 8, f = sq % 8;
    for(int dr = -1; dr <= 1; ++dr) {
        for(int df = -1; df <= 1; ++df) {
            if(dr == 0 && df == 0) continue;
            int nr = r + dr, nf = f + df;
            if(nr >= 0 && nr < 8 && nf >= 0 && nf < 8) out.insert(nr * 8 + nf);
        }
    }
}

void expectedKnightSquares(int sq, std::set<int>& out) {
    int r = sq / 8, f = sq % 8;
    int drs[] = {1,1,-1,-1,2,2,-2,-2};
    int dfs[] = {2,-2,2,-2,1,-1,1,-1};
    for(int i = 0; i < 8; ++i) {
        int nr = r + drs[i], nf = f + dfs[i];
        if(nr >= 0 && nr < 8 && nf >= 0 && nf < 8) out.insert(nr * 8 + nf);
    }
}

void expectedBishopSquares(int sq, std::set<int>& out) {
    int r = sq / 8, f = sq % 8;
    int drs[] = {1,1,-1,-1};
    int dfs[] = {1,-1,1,-1};
    for(int d = 0; d < 4; ++d) {
        int nr = r + drs[d], nf = f + dfs[d];
        while(nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
            out.insert(nr * 8 + nf);
            nr += drs[d];
            nf += dfs[d];
        }
    }
}

void expectedRookSquares(int sq, std::set<int>& out) {
    int r = sq / 8, f = sq % 8;
    int drs[] = {1,-1,0,0};
    int dfs[] = {0,0,1,-1};
    for(int d = 0; d < 4; ++d) {
        int nr = r + drs[d], nf = f + dfs[d];
        while(nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
            out.insert(nr * 8 + nf);
            nr += drs[d];
            nf += dfs[d];
        }
    }
}

void expectedQueenSquares(int sq, std::set<int>& out) {
    expectedBishopSquares(sq, out);
    expectedRookSquares(sq, out);
}

//[KING / KNIGHT ATTACK TABLES]

void test_king_attacks() {
    for(int sq = 0; sq < 64; ++sq) {
        std::set<int> expected;
        expectedKingSquares(sq, expected);

        uint64_t got = kingAttacks(sq);
        std::set<int> gotSet;
        for(int i = 0; i < 64; ++i) if(got & (1ULL << i)) gotSet.insert(i);

        assert(gotSet == expected);
    }
}

void test_knight_attacks() {
    for(int sq = 0; sq < 64; ++sq) {
        std::set<int> expected;
        expectedKnightSquares(sq, expected);

        uint64_t got = knightAttacks(sq);
        std::set<int> gotSet;
        for(int i = 0; i < 64; ++i) if(got & (1ULL << i)) gotSet.insert(i);

        assert(gotSet == expected);
    }
}

//[PAWN MOVES]

void test_pawn_start_position() {
    Board board;
    from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    auto moves = generatePawnMoves(board);
    assert(moves.size() == 16); // 8 single pushes + 8 double pushes

    int singlePushCount = 0, doublePushCount = 0;
    for(const Move& m : moves) {
        if(m.flags & DOUBLE_PAWN_PUSH) doublePushCount++;
        else singlePushCount++;
    }
    assert(singlePushCount == 8);
    assert(doublePushCount == 8);
}

void test_pawn_promotion_push_and_capture() {
    Board board;
    from_FEN(board, "1r2k3/P7/8/8/8/8/8/K7 w - - 0 1");

    auto moves = generatePawnMoves(board);
    assert(moves.size() == 8);

    int a7 = 6 * 8 + 0; // a7
    int a8 = 7 * 8 + 0; // a8
    int b8 = 7 * 8 + 1; // b8

    assert(hasMove(moves, a7, a8, KNIGHT, 0));
    assert(hasMove(moves, a7, a8, BISHOP, 0));
    assert(hasMove(moves, a7, a8, ROOK, 0));
    assert(hasMove(moves, a7, a8, QUEEN, 0));
    assert(hasMove(moves, a7, b8, KNIGHT, CAPTURE));
    assert(hasMove(moves, a7, b8, BISHOP, CAPTURE));
    assert(hasMove(moves, a7, b8, ROOK, CAPTURE));
    assert(hasMove(moves, a7, b8, QUEEN, CAPTURE));
}

void test_pawn_en_passant() {
    Board board;
    from_FEN(board, "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");

    auto moves = generatePawnMoves(board);
    assert(moves.size() == 2);

    int e5 = 4 * 8 + 4; // e5
    int e6 = 5 * 8 + 4; // e6
    int d6 = 5 * 8 + 3; // d6

    assert(hasMove(moves, e5, e6, -1, 0));
    assert(hasMove(moves, e5, d6, -1, CAPTURE | EN_PASSANT));
}

void test_pawn_blocked_push() {
    Board board;
    from_FEN(board, "4k3/8/8/8/8/4n3/4P3/4K3 w - - 0 1");

    auto moves = generatePawnMoves(board);
    assert(moves.empty());
}

void test_pawn_file_edge_no_wraparound() {
    Board board;
    from_FEN(board, "4k3/8/8/8/8/n7/7P/4K3 w - - 0 1");

    auto moves = generatePawnMoves(board);
    assert(moves.size() == 2);

    int h2 = 1 * 8 + 7; // h2
    int h3 = 2 * 8 + 7; // h3
    int h4 = 3 * 8 + 7; // h4
    int a3 = 2 * 8 + 0; // a3

    assert(hasMove(moves, h2, h3, -1, 0));
    assert(hasMove(moves, h2, h4, -1, DOUBLE_PAWN_PUSH));
    for(const Move& m : moves) assert(m.end != a3);
}

void test_black_pawn_start_position() {
    Board board;
    from_FEN(board, "4k3/pppppppp/8/8/8/8/8/4K3 b - - 0 1");

    auto moves = generatePawnMoves(board);
    assert(moves.size() == 16);
}

void test_black_pawn_promotion_and_capture() {
    Board board;
    from_FEN(board, "4k3/8/8/8/8/8/p7/1R2K3 b - - 0 1");

    auto moves = generatePawnMoves(board);
    assert(moves.size() == 8);

    int a2 = 1 * 8 + 0; // a2
    int a1 = 0 * 8 + 0; // a1
    int b1 = 0 * 8 + 1; // b1

    assert(hasMove(moves, a2, a1, QUEEN, 0));
    assert(hasMove(moves, a2, b1, QUEEN, CAPTURE));
}

void test_black_pawn_en_passant() {
    Board board;
    from_FEN(board, "4k3/8/8/8/3Pp3/8/8/4K3 b - d3 0 1");

    auto moves = generatePawnMoves(board);
    assert(moves.size() == 2);

    int e4 = 3 * 8 + 4; // e4
    int e3 = 2 * 8 + 4; // e3
    int d3 = 2 * 8 + 3; // d3

    assert(hasMove(moves, e4, e3, -1, 0));
    assert(hasMove(moves, e4, d3, -1, CAPTURE | EN_PASSANT));
}

//[KNIGHT MOVES]

void test_knight_moves_occupancy() {
    Board board;
    from_FEN(board, "4k3/8/2p5/5P2/3N4/8/8/4K3 w - - 0 1");

    auto moves = generateKnightMoves(board);
    assert(moves.size() == 7); // 8 geometric targets minus the own-piece square f5

    int d4 = 3 * 8 + 3; // d4
    int f5 = 4 * 8 + 5; // f5 (own pawn, must not be a destination)
    int c6 = 5 * 8 + 2; // c6 (enemy pawn, must be a flagged capture)

    for(const Move& m : moves) assert(m.end != f5);
    assert(hasMove(moves, d4, c6, -1, CAPTURE));
}

//[BISHOP MOVES]

void test_bishop_moves_empty_board() {
    for(int sq = 0; sq < 64; ++sq) {
        Board board;
        board.clear();
        board.bitboards[BISHOP][WHITE] = (1ULL << sq);
        board.white_pieces = board.bitboards[BISHOP][WHITE];
        board.all_pieces = board.white_pieces;
        board.active_color = WHITE;

        std::set<int> expected;
        expectedBishopSquares(sq, expected);

        assert(moveEnds(generateBishopMoves(board)) == expected);
    }
}

void test_bishop_blockers_and_capture() {
    Board board;
    from_FEN(board, "4k3/8/1p3p2/8/3B4/8/8/4K3 w - - 0 1");

    auto moves = generateBishopMoves(board);
    assert(moves.size() == 10);

    int d4 = 3 * 8 + 3; // d4
    int b6 = 5 * 8 + 1; // b6 (capture, ray must stop here)
    int f6 = 5 * 8 + 5; // f6 (capture, ray must stop here)
    int a7 = 6 * 8 + 0; // a7 (beyond b6, must not appear)
    int g7 = 6 * 8 + 6; // g7 (beyond f6, must not appear)

    assert(hasMove(moves, d4, b6, -1, CAPTURE));
    assert(hasMove(moves, d4, f6, -1, CAPTURE));
    for(const Move& m : moves) {
        assert(m.end != a7);
        assert(m.end != g7);
    }
}

void test_bishop_corner_no_wraparound() {
    Board board;
    from_FEN(board, "4k3/8/8/8/8/8/8/B3K3 w - - 0 1");

    auto moves = generateBishopMoves(board);
    assert(moves.size() == 7); // only the b2-h8 diagonal is open

    int h1 = 0 * 8 + 7; // h1: the wraparound bug used to produce this as a false target
    for(const Move& m : moves) assert(m.end != h1);
}

//[ROOK MOVES]

void test_rook_moves_empty_board() {
    for(int sq = 0; sq < 64; ++sq) {
        Board board;
        board.clear();
        board.bitboards[ROOK][WHITE] = (1ULL << sq);
        board.white_pieces = board.bitboards[ROOK][WHITE];
        board.all_pieces = board.white_pieces;
        board.active_color = WHITE;

        std::set<int> expected;
        expectedRookSquares(sq, expected);

        assert(moveEnds(generateRookMoves(board)) == expected);
    }
}

void test_rook_file_a_vertical_movement() {
    Board board;
    from_FEN(board, "4k3/8/8/8/R7/8/8/4K3 w - - 0 1");

    auto moves = generateRookMoves(board);
    assert(moves.size() == 14); // 7 vertical (a-file) + 7 horizontal (rank 4)
}

void test_rook_left_no_wraparound() {
    Board board;
    from_FEN(board, "4k3/8/8/8/3R4/8/8/4K3 w - - 0 1");

    auto moves = generateRookMoves(board);
    assert(moves.size() == 14);

    int h3 = 2 * 8 + 7; // h3: the wraparound bug used to produce this as a false target
    for(const Move& m : moves) assert(m.end != h3);
}

void test_rook_blockers_and_capture() {
    Board board;
    from_FEN(board, "4k3/8/3P4/8/1p1R4/8/8/4K3 w - - 0 1");

    auto moves = generateRookMoves(board);
    assert(moves.size() == 10);

    int d4 = 3 * 8 + 3; // d4
    int b4 = 3 * 8 + 1; // b4 (capture)
    int d6 = 5 * 8 + 3; // d6 (own piece, must not appear)

    assert(hasMove(moves, d4, b4, -1, CAPTURE));
    for(const Move& m : moves) assert(m.end != d6);
}

//[QUEEN MOVES]

void test_queen_moves_empty_board() {
    for(int sq = 0; sq < 64; ++sq) {
        Board board;
        board.clear();
        board.bitboards[QUEEN][WHITE] = (1ULL << sq);
        board.white_pieces = board.bitboards[QUEEN][WHITE];
        board.all_pieces = board.white_pieces;
        board.active_color = WHITE;

        std::set<int> expected;
        expectedQueenSquares(sq, expected);

        assert(moveEnds(generateQueenMoves(board)) == expected);
    }
}

void test_queen_mixed_blockers_and_capture() {
    Board board;
    from_FEN(board, "4k3/8/3P1p2/8/1r1Q4/8/8/4K3 w - - 0 1");

    auto moves = generateQueenMoves(board);
    assert(moves.size() == 21);

    int d4 = 3 * 8 + 3; // d4
    int b4 = 3 * 8 + 1; // b4 (horizontal capture)
    int f6 = 5 * 8 + 5; // f6 (diagonal capture)

    assert(hasMove(moves, d4, b4, -1, CAPTURE));
    assert(hasMove(moves, d4, f6, -1, CAPTURE));
}

int main() {
    test_king_attacks();
    test_knight_attacks();

    test_pawn_start_position();
    test_pawn_promotion_push_and_capture();
    test_pawn_en_passant();
    test_pawn_blocked_push();
    test_pawn_file_edge_no_wraparound();
    test_black_pawn_start_position();
    test_black_pawn_promotion_and_capture();
    test_black_pawn_en_passant();

    test_knight_moves_occupancy();

    test_bishop_moves_empty_board();
    test_bishop_blockers_and_capture();
    test_bishop_corner_no_wraparound();

    test_rook_moves_empty_board();
    test_rook_file_a_vertical_movement();
    test_rook_left_no_wraparound();
    test_rook_blockers_and_capture();

    test_queen_moves_empty_board();
    test_queen_mixed_blockers_and_capture();

    printf("test_movegen: all tests passed\n");
    return 0;
}
