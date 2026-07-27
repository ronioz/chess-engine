#include <board.hpp>
#include <fen_conversion.hpp>
#include <cassert>
#include <cstdio>

void test_valid_roundtrip() {
    const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "8/8/8/8/8/8/8/8 w - - 0 1",
        "4k3/8/8/8/8/8/8/4K3 b - - 15 42",
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2",
        "r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1",
    };

    for(const char* fen : fens) {
        Board board;
        bool ok = from_FEN(board, fen);
        assert(ok);

        std::string out = to_FEN(board);
        assert(out == fen);
    }
}

void test_from_FEN_valid_fields() {
    Board board;
    bool ok = from_FEN(board, "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    assert(ok);

    assert(board.active_color == WHITE);
    assert(board.white_can_castle_kingside);
    assert(board.white_can_castle_queenside);
    assert(board.black_can_castle_kingside);
    assert(board.black_can_castle_queenside);
    assert(board.en_passant_square == 43); // d6
    assert(board.halfmove_clock == 0);
    assert(board.fullmove_number == 2);
}

void test_from_FEN_no_castling_rights() {
    Board board;
    bool ok = from_FEN(board, "4k3/8/8/8/8/8/8/4K3 b - - 15 42");
    assert(ok);

    assert(board.active_color == BLACK);
    assert(!board.white_can_castle_kingside);
    assert(!board.white_can_castle_queenside);
    assert(!board.black_can_castle_kingside);
    assert(!board.black_can_castle_queenside);
    assert(board.en_passant_square == -1);
    assert(board.halfmove_clock == 15);
    assert(board.fullmove_number == 42);
}

void test_from_FEN_rejects_malformed_piece_placement() {
    Board board;
    assert(!from_FEN(board, "ppppppppp/8/8/8/8/8/8/8 w - - 0 1"));           // 9 pieces in a rank
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBX w KQkq - 0 1")); // invalid piece letter
}

void test_from_FEN_rejects_bad_active_color() {
    Board board;
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1"));
}

void test_from_FEN_rejects_bad_castling() {
    Board board;
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w ZZZZ - 0 1"));
}

void test_from_FEN_rejects_bad_en_passant() {
    Board board;
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq z9 0 1")); // bad file
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e9 0 1")); // bad rank
}

void test_from_FEN_rejects_non_numeric_clocks() {
    Board board;
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - ab 1"));
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 xy"));
}

void test_from_FEN_rejects_incomplete_fen() {
    Board board;
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"));
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w"));
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq"));
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"));
    assert(!from_FEN(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0"));
}

int main() {
    test_valid_roundtrip();
    test_from_FEN_valid_fields();
    test_from_FEN_no_castling_rights();
    test_from_FEN_rejects_malformed_piece_placement();
    test_from_FEN_rejects_bad_active_color();
    test_from_FEN_rejects_bad_castling();
    test_from_FEN_rejects_bad_en_passant();
    test_from_FEN_rejects_non_numeric_clocks();
    test_from_FEN_rejects_incomplete_fen();

    printf("test_fen: all tests passed\n");
    return 0;
}
