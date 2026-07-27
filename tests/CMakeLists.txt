add_executable(test_board test_board.cpp ${CMAKE_SOURCE_DIR}/src/board.cpp)
target_include_directories(test_board PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME board_tests COMMAND test_board)

add_executable(test_fen test_fen.cpp ${CMAKE_SOURCE_DIR}/src/board.cpp ${CMAKE_SOURCE_DIR}/src/fen_conversion.cpp)
target_include_directories(test_fen PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME fen_tests COMMAND test_fen)
