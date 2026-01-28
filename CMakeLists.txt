cmake_minimum_required(VERSION 3.20)
project(sensor_log_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sla
    src/main.cpp 
    src/util.cpp
    src/csv_split.cpp
    src/number_parse.cpp
    src/stats.cpp
    src/time_axis.cpp
    src/csv.cpp
    src/clean_writer.cpp
    src/report_json.cpp
    src/cli.cpp
)

target_include_directories(sla PRIVATE 
    ${CMAKE_SOURCE_DIR}/include/sla
    ${CMAKE_SOURCE_DIR}/src
)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(sla PRIVATE fmt::fmt nlohmann_json::nlohmann_json)
