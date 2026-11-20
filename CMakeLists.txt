cmake_minimum_required(VERSION 3.16)
project(chip8_emulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Núcleo da CPU: C++ puro, compila em qualquer sistema
add_library(chip8_core STATIC emular8hd/Chip8.cpp)
target_include_directories(chip8_core PUBLIC emular8hd)
if(MSVC)
  target_compile_options(chip8_core PRIVATE /W4)
else()
  target_compile_options(chip8_core PRIVATE -Wall -Wextra)
endif()

# Testes do núcleo (rodam no Linux, macOS e Windows)
enable_testing()
add_executable(chip8_tests tests/test_chip8.cpp)
target_link_libraries(chip8_tests PRIVATE chip8_core)
add_test(NAME chip8_tests COMMAND chip8_tests)

# Interface gráfica: só no Windows (Win32 API)
if(WIN32)
  add_executable(chip8_emulator emular8hd/main.cpp)
  target_link_libraries(chip8_emulator PRIVATE chip8_core gdi32)
endif()
