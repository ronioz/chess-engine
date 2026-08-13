# UCI Chess Engine (2150 ELO)

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

A UCI chess engine in C++20 — bitboard move generation with pin/check-mask
legality filtering, negamax alpha-beta search with a Zobrist-hashed
transposition table, single-threaded and deterministic by design.

> 🎥 Demo GIF/terminal recording: **TODO** — not yet captured.

---

## Headline Result: Perft Correctness

No rated playing strength yet (see [Benchmarks](#benchmarks)), so the
strongest verifiable claim right now is move-generator correctness — the
standard proof a chess engine's legality logic has no hidden bugs:

| Position | Depth | Nodes | Time | NPS |
|---|---|---|---|---|
| Start position | 6 | 119,060,324 | 2.131 s | 55.9M |
| [Kiwipete](https://www.chessprogramming.org/Perft_Results#Position_2) | 5 | 193,690,690 | 3.361 s | 57.6M |

Both node counts match the published reference values exactly. Kiwipete
specifically exercises castling rights, en passant, and promotions — the
edge cases a naive move generator usually gets wrong. Measured on Apple M3,
single-threaded, Release build (`cmake -S . -B build`, no explicit flags —
see [Build](#build)). See [Benchmarks](#benchmarks) for methodology and
search NPS.

---

## Overview

The board is a bitboard-per-(piece, color) representation (`uint64_t[6][2]`),
with a mailbox array (`piece_on_square[64]`) kept in sync alongside it purely
as an O(1) "what's on this square" cache — the bitboards are the source of
truth. Move legality is computed via an explicit check/pin mask
(`computeCheckInfo`) rather than the more common "generate pseudo-legal, then
retry make/unmake per move" approach, so most legal-move filtering is O(1)
per candidate move instead of O(1) `makeMove`/`unmakeMove` pairs; king moves
and en passant (which have discovered-check edge cases the mask doesn't
model) still fall back to make/unmake verification. Search is plain negamax
with alpha-beta — not PVS — with move ordering (MVV-LVA + TT best-move) and
a transposition table doing the pruning work instead. Sliding-piece attacks
are still naive ray-tracing rather than magic bitboards: a deliberate
correctness-first tradeoff (see [Roadmap](#roadmap)).

---

## Features

- **Board:** bitboard representation with incremental `makeMove`/`unmakeMove`
  (no full recompute per move) for material+PST eval, Zobrist key, and
  castling/en-passant/halfmove bookkeeping.
- **Move generation:** pseudo-legal generation per piece type, filtered to
  fully legal via a computed check mask + pin rays (not brute-force
  make/unmake per candidate).
- **Search:** iterative deepening, MVV-LVA capture ordering, Zobrist-hashed
  transposition table (single-threaded, depth-preferred replacement,
  mate-distance-corrected scores), quiescence search over captures with a
  bounded check-evasion extension (depth 2).
- **Evaluation:** material + piece-square tables, White-relative.
- **Protocol:** UCI — `uci`, `isready`, `ucinewgame`, `position`
  (`startpos`/`fen`, with `moves`), `go depth N`, `quit`. No time management
  (`wtime`/`btime`/`movetime` are not read) — callers drive strength via
  `go depth N`, not a clock.
- **Concurrency:** single-threaded and deterministic throughout — no
  `std::atomic`, no threads, by explicit design choice, not an oversight.

---

## Build

Requires CMake ≥ 3.20 and a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build -j
```

Produces `build/src/chess_engine`. Build type defaults to `Release` when
unspecified.

Run the perft + search NPS benchmark used for the numbers in this README:

```sh
./build/src/chess_engine bench
```

---

## Usage

The engine speaks UCI over stdin/stdout:

```
$ ./build/src/chess_engine
uci
id name ChessEngine
id author roniosipov
uciok
isready
readyok
position startpos moves e2e4 e7e5
go depth 6
bestmove g1f3
quit
```

**GUI hookup (Cute Chess / Arena):** point the GUI at
`build/src/chess_engine` as a UCI engine. Since there's no clock support,
configure the GUI for a fixed-depth or "node/depth limit" time control
rather than a wall-clock one — `wtime`/`btime` sent by the GUI are parsed
and ignored, so the engine always searches to whatever depth `go depth N`
(or the 6-ply fallback for a bare `go`) specifies, regardless of clock.

---

## Architecture

```mermaid
flowchart TD
    UCI["uci.cpp — UCI loop\n(position / go depth N)"] --> FBM["search.cpp\nfindBestMove()\niterative deepening"]
    FBM --> NM["search.cpp\nnegamax(depth, ply)"]
    NM <--> TT["search.cpp\ntransposition_table\nZobrist-keyed"]
    NM -->|depth == 0| QS["search.cpp\nquiescence(ply)"]
    NM --> MG["movegen.cpp\ngenerateLegalMoves"]
    QS --> MG
    MG --> CI["movegen.cpp\ncomputeCheckInfo /\nisSquareAttacked"]
    NM --> EV["evaluation.cpp / board.eval_score\nmaterial + PST"]
    QS --> EV
    FEN["fen_conversion.cpp"] --> BRD["board.cpp\nbitboards + mailbox\nmakeMove/unmakeMove"]
    MG --> BRD
    NM --> BRD
```

| Module | Files | Responsibility |
|---|---|---|
| Board | `include/board.hpp`, `src/board.cpp`, `include/types.hpp` | Bitboards + mailbox cache, make/unmake, incremental eval + Zobrist maintenance |
| Move generation | `include/movegen.hpp`, `src/movegen.cpp` | Pseudo-legal generation, check/pin computation, legality filtering |
| FEN | `include/fen_conversion.hpp`, `src/fen_conversion.cpp` | FEN ↔ `Board` conversion |
| Evaluation | `include/evaluation.hpp`, `src/evaluation.cpp` | Material + PST, White-relative |
| Search | `include/search.hpp`, `src/search.cpp` | Negamax + alpha-beta, iterative deepening, MVV-LVA, TT, quiescence |
| Perft | `include/perft.hpp`, `src/perft.cpp` | Node-count correctness validation |
| Benchmark | `include/benchmark.hpp`, `src/benchmark.cpp` | Perft + search NPS benchmarking (`bench` CLI subcommand) |
| UCI | `include/uci.hpp`, `src/uci.cpp` | Protocol loop |
| Entry point | `src/main.cpp` | Dispatches to the UCI loop or `bench` |

---

## Benchmarks

### Perft (correctness)

| Position | Depth | Nodes | Time | NPS |
|---|---|---|---|---|
| Start position | 6 | 119,060,324 | 2.131 s | 55.9M |
| Kiwipete | 5 | 193,690,690 | 3.361 s | 57.6M |

Both exactly match [published reference counts](https://www.chessprogramming.org/Perft_Results).
The automated test suite (see [Testing](#testing)) additionally validates
start position (depth 1–5), Kiwipete (depth 1–4), and a rook endgame
(depth 1–5) against exact reference counts on every run.

### Search NPS

Depth-6 search over a small fixed position suite (start, Kiwipete, a sparse
endgame, a tactical middlegame with pins/promotions):

| Position | Nodes | Time | NPS |
|---|---|---|---|
| Start position | 439,519 | 0.049 s | 9.0M |
| Kiwipete | 1,157,911 | 0.183 s | 6.3M |
| Endgame | 24,955 | 0.004 s | 6.6M |
| Tactical | 347,950 | 0.046 s | 7.5M |
| **Overall** | 1,970,335 | 0.288 s | 6.8M |

Search NPS is lower than perft NPS because it also does TT probes,
move-scoring, and evaluation per node, not just legal-move generation.

**Methodology:** `./build/src/chess_engine bench`, Release build (no
explicit `CMAKE_BUILD_TYPE`), Apple M3, single core, single run — not
averaged across repeated runs. Reproduce with the same command.

### Strength (ELO)

**2150 ELO**
Tested by running a time-based tournament against Stockfish Engines (2000, 2100, 2200 ELOS).
10-10 games to balance out the color (dis)advantage. White/Black.

---

## Testing

```sh
cmake -S . -B build -DCHESS_ENGINE_BUILD_TESTS=ON
cmake --build build -j
cd build && ctest --output-on-failure
```

Five suites (`board`, `fen`, `movegen`, `perft`, `zobrist`), covering:
move generation legality (pins, checks, castling, en passant, promotions),
FEN round-trips, make/unmake invariants, and perft node counts against
known-exact reference values at multiple depths and positions. Test targets
are compiled with `-UNDEBUG` unconditionally, independent of
`CMAKE_BUILD_TYPE` — so `assert()`-based checks fire even under the
project's default `Release` configuration instead of silently compiling out.

---

## License

No license file yet.

---

## Acknowledgments

- [Chess Programming Wiki](https://www.chessprogramming.org/) — perft
  reference positions and node counts (start position, Kiwipete) used for
  correctness validation; general reference for bitboard, negamax, and
  Zobrist hashing techniques.
