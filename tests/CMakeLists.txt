add_executable(test_board test_board.cpp ${CMAKE_SOURCE_DIR}/src/board.cpp)
target_include_directories(test_board PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME board_tests COMMAND test_board)

add_executable(test_fen test_fen.cpp ${CMAKE_SOURCE_DIR}/src/board.cpp ${CMAKE_SOURCE_DIR}/src/fen_conversion.cpp)
target_include_directories(test_fen PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME fen_tests COMMAND test_fen)

add_executable(test_movegen test_movegen.cpp ${CMAKE_SOURCE_DIR}/src/board.cpp ${CMAKE_SOURCE_DIR}/src/fen_conversion.cpp ${CMAKE_SOURCE_DIR}/src/movegen.cpp)
target_include_directories(test_movegen PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME movegen_tests COMMAND test_movegen)

add_executable(test_perft test_perft.cpp ${CMAKE_SOURCE_DIR}/src/board.cpp ${CMAKE_SOURCE_DIR}/src/perft.cpp ${CMAKE_SOURCE_DIR}/src/movegen.cpp ${CMAKE_SOURCE_DIR}/src/fen_conversion.cpp)
target_include_directories(test_perft PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME perft_tests COMMAND test_perft)
