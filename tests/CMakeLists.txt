cmake_minimum_required(VERSION 3.22)
project(basicSynth_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Filter sources: no HAL or hardware dependencies
set(FILTER_SOURCES
    ../App/Src/svf.cpp
    ../App/Src/moogLadder.cpp
)

set(ADSR_SOURCES
    ../App/Src/adsr.cpp
)

set(FILTER_INCLUDES
    ../App/Inc
    ../Core/Inc
)

add_executable(test_svf test_svf.cpp ${FILTER_SOURCES})
target_include_directories(test_svf PRIVATE ${FILTER_INCLUDES})

add_executable(test_moog_ladder test_moog_ladder.cpp ${FILTER_SOURCES})
target_include_directories(test_moog_ladder PRIVATE ${FILTER_INCLUDES})

add_executable(test_adsr test_adsr.cpp ${ADSR_SOURCES})
target_include_directories(test_adsr PRIVATE ${FILTER_INCLUDES})

# GCC/Clang need explicit libm; MSVC includes it in the CRT
if (NOT MSVC)
    target_link_libraries(test_svf m)
    target_link_libraries(test_moog_ladder m)
    target_link_libraries(test_adsr m)
endif()

enable_testing()
add_test(NAME svf          COMMAND test_svf)
add_test(NAME moog_ladder  COMMAND test_moog_ladder)
add_test(NAME adsr         COMMAND test_adsr)
