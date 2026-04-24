cmake_minimum_required(VERSION 3.16)
project(ungula_bsp_waveshare_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

set(LIB_BSP_SRC ${CMAKE_CURRENT_LIST_DIR}/../src)

# We only test pure, ESP-IDF-free code on the host (the pin-mask builders
# in each board's detail:: namespace). No board.cpp sources are compiled
# here — those pull in ESP32_IO_Expander and TimeControl. Hardware smoke
# checks live in the host project that flashes the real board.
include_directories(
    ${LIB_BSP_SRC}
)

add_executable(test_ungula_bsp_waveshare
    test_lcd7_pin_mask.cpp
    test_lcd43_pin_mask.cpp
)
target_link_libraries(test_ungula_bsp_waveshare gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(test_ungula_bsp_waveshare)
