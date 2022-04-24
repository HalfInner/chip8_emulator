cmake_minimum_required(VERSION 3.15)
set(FAKE_CHIP8_PROJECT_NAME FakeChip8)
project(${FAKE_CHIP8_PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(SOURCE
    src/main.cc
    src/FakeChip8.cc
    src/SfmlGui.cc
)

set(HEADERS
    inc/FakeChip8Runner.h
    inc/FakeChip8.h
    inc/SfmlGui.h
)

add_executable(${FAKE_CHIP8_PROJECT_NAME} ${SOURCE} ${HEADERS})

set(SFML_STATIC_LIBRARIES TRUE)
set(BUILD_SHARED_LIBS FALSE)
set(SFML_USE_STATIC_STD_LIBS TRUE)
add_subdirectory(3pp/SFML)

target_include_directories(${FAKE_CHIP8_PROJECT_NAME} 
    PUBLIC inc)
target_link_libraries(${FAKE_CHIP8_PROJECT_NAME} 
    PUBLIC sfml-graphics sfml-system
    )

