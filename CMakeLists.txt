cmake_minimum_required(VERSION 3.20)
project(sensor_log_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies (vcpkg)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(FastFloat CONFIG REQUIRED)

add_library(sla_lib
    src/util.cpp
    src/csv_split.cpp
    src/number_parse.cpp
    src/time_axis.cpp
    src/csv.cpp
    src/writer.cpp
    src/report_json.cpp
    src/cli.cpp
    src/calibration.cpp
)

target_include_directories(sla_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(sla_lib PUBLIC
    fmt::fmt
    nlohmann_json::nlohmann_json
    FastFloat::fast_float
)

# Main exe
add_executable(sla src/main.cpp)
target_link_libraries(sla PRIVATE sla_lib)

# Tests (CTest + Catch2)
include(CTest)    # adds the BUILD_TESTING option
enable_testing()

if (BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(unit_tests
        tests/test_util.cpp
        tests/test_csv_split.cpp
        tests/test_number_parse.cpp
        tests/test_welford.cpp
        tests/test_time_axis.cpp
    )

    target_link_libraries(unit_tests PRIVATE
        sla_lib
        Catch2::Catch2WithMain
    )

    include(Catch)
    catch_discover_tests(unit_tests)
endif()