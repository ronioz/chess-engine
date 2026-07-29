add_executable(chess_engine main.cpp board.cpp fen_conversion.cpp movegen.cpp perft.cpp evaluation.cpp search.cpp)
target_include_directories(chess_engine PRIVATE ${CMAKE_SOURCE_DIR}/include)
