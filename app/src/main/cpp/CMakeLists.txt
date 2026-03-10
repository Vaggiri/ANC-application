cmake_minimum_required(VERSION 3.22.1)

project("twsnoisecanceller")

# Find Oboe using Prefab
find_package(oboe REQUIRED CONFIG)

add_library(
        ${CMAKE_PROJECT_NAME}
        SHARED
        native-lib.cpp)

# Link the native library against Oboe and Android's logging library
target_link_libraries(
        ${CMAKE_PROJECT_NAME}
        oboe::oboe
        log)
