#include <board.hpp>
#include <fen_conversion.hpp>
#include <cassert>
#include <cstdio>

// Runs a move through makeMove/unmakeMove and checks two invariants:
// (1) after makeMove, the incrementally-updated key matches a from-scratch recompute
// (2) after unmakeMove, the key is restored exactly to its pre-move value
void checkMoveRoundTrip(Board& board, const Move& move) {
    uint64_t before = board.zobrist_key;

    UndoState state = board.makeMove(move);
    assert(board.zobrist_key == board.initZobristKey());

    board.unmakeMove(state);
    assert(board.zobrist_key == before);
}

void test_setup_matches_recompute() {
    Board board;
    board.setup();

    assert(board.zobrist_key == board.initZobristKey());
}

void test_fen_matches_recompute() {
    const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "r3k2r/8/8/8/8/8/8/R3K2R w Kk - 0 1",
        "8/8/8/8/8/8/8/4K2k w - - 0 1",
    };

    for(const char* fen : fens) {
        Board board;
        bool ok = from_FEN(board, fen);
        assert(ok);
        assert(board.zobrist_key == board.initZobristKey());
    }
}

void test_same_position_different_path_same_key() {
    Board a;
    a.setup();

    Board b;
    from_FEN(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    assert(a.zobrist_key == b.zobrist_key);
}

void test_move_changes_key() {
    Board board;
    board.setup();

    uint64_t before = board.zobrist_key;
    board.makeMove(Move{6, 21, -1, 0}); // g1f3

    assert(board.zobrist_key != before);
}

void test_quiet_move_round_trip() {
    Board board;
    board.setup();

    checkMoveRoundTrip(board, Move{6, 21, -1, 0}); // g1f3
}

void test_capture_round_trip() {
    Board board;
    from_FEN(board, "rnbqkbnr/pppp1ppp/8/4p3/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1");

    checkMoveRoundTrip(board, Move{21, 36, -1, CAPTURE}); // f3xe5
}

void test_castle_white_kingside_round_trip() {
    Board board;
    from_FEN(board, "rnbqk2r/pppp1ppp/5n2/4p3/2B1P1b1/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1");

    checkMoveRoundTrip(board, Move{4, 6, -1, CASTLE}); // e1g1
}

void test_castle_white_queenside_round_trip() {
    Board board;
    from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R3KBNR w KQkq - 0 1");

    checkMoveRoundTrip(board, Move{4, 2, -1, CASTLE}); // e1c1
}

void test_castle_black_kingside_round_trip() {
    Board board;
    from_FEN(board, "rnbqk2r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");

    checkMoveRoundTrip(board, Move{60, 62, -1, CASTLE}); // e8g8
}

void test_castle_black_queenside_round_trip() {
    Board board;
    from_FEN(board, "r3kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");

    checkMoveRoundTrip(board, Move{60, 58, -1, CASTLE}); // e8c8
}

void test_en_passant_round_trip() {
    Board board;
    from_FEN(board, "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 1");

    checkMoveRoundTrip(board, Move{36, 43, -1, EN_PASSANT | CAPTURE}); // e5xd6 e.p.
}

void test_promotion_capture_round_trip() {
    Board board;
    from_FEN(board, "rnbqkbnr/P1pppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1");

    checkMoveRoundTrip(board, Move{48, 56, QUEEN, CAPTURE}); // a7xb8=Q
}

void test_promotion_quiet_round_trip() {
    Board board;
    from_FEN(board, "4k3/4P3/8/8/8/8/8/4K3 w - - 0 1");

    checkMoveRoundTrip(board, Move{52, 60, QUEEN, 0}); // e7e8=Q
}

int main() {
    test_setup_matches_recompute();
    test_fen_matches_recompute();
    test_same_position_different_path_same_key();
    test_move_changes_key();
    test_quiet_move_round_trip();
    test_capture_round_trip();
    test_castle_white_kingside_round_trip();
    test_castle_white_queenside_round_trip();
    test_castle_black_kingside_round_trip();
    test_castle_black_queenside_round_trip();
    test_en_passant_round_trip();
    test_promotion_capture_round_trip();
    test_promotion_quiet_round_trip();

    printf("test_zobrist: all tests passed\n");
    return 0;
}
