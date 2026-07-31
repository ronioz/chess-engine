#pragma once

/*
Minimal UCI (Universal Chess Interface) loop, reading commands from stdin and
writing responses to stdout. Supports just enough of the protocol to be driven
by an automated game runner: uci, isready, ucinewgame, position (startpos/fen,
with moves), go depth N, quit. No time management (wtime/btime/movetime) - the
search has none, per the roadmap's single-threaded/deterministic scope, so
callers drive it with "go depth N" rather than a clock.
*/
void runUCI();
