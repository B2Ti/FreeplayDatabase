cmake_minimum_required(VERSION 3.25)

project(FreeplayDatabase)

message(STATUS "Checking build type.")
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}.")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wpedantic")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native")
endif()

if (CMAKE_C_COMPILER_ID MATCHES "MSVC")
    add_compile_options("/experimental:c11atomics")
endif()

find_package(ZLIB REQUIRED)

# write the zlib path to a file so the reader script can link with it later
file(WRITE "${CMAKE_BINARY_DIR}/zlib_path.txt" "${ZLIB_LIBRARY}")

file(GLOB SOURCE_FILES "src/*.c")

include_directories("${PROJECT_SOURCE_DIR}/include")

include_directories("${ZLIB_INCLUDE_DIR}")

add_library(SOURCE OBJECT ${SOURCE_FILES})

find_library(MATH_LIBRARY m)
if (MATH_LIBRARY)
    target_link_libraries(SOURCE ZLIB::ZLIB ${MATH_LIBRARY})
else()
    target_link_libraries(SOURCE ZLIB::ZLIB)
endif()

add_executable(database "database/main.c")
add_executable(printing "printing/main.c")
add_executable(statistics "statistics/main.c")

target_link_libraries(printing PUBLIC SOURCE)
target_link_libraries(database PUBLIC SOURCE)
target_link_libraries(statistics PUBLIC SOURCE)

add_library(readerCDLL SHARED "reader/reader.c")


if (WIN32)
    set_target_properties(readerCDLL PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

set_target_properties(readerCDLL PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    OUTPUT_NAME "reader"
)

target_link_libraries(readerCDLL PRIVATE SOURCE)

add_executable(test_reader "reader/readFromDatabase.c")
target_link_libraries(test_reader PUBLIC SOURCE)
target_link_libraries(test_reader PUBLIC readerCDLL)

set(READER_SRC "${CMAKE_SOURCE_DIR}/reader/readWrapper.py")
set(READER_DST "$<TARGET_FILE_DIR:readerCDLL>/readWrapper.py")
set(STATISTICS_SRC "${CMAKE_SOURCE_DIR}/reader/seedStatistics.py")
set(STATISTICS_DST "$<TARGET_FILE_DIR:readerCDLL>/seedStatistics.py")
set(ZLIB_PATH_LOCATION "${CMAKE_BINARY_DIR}/zlib_path.txt")
set(ZLIB_PATH_OUTPUT "$<TARGET_FILE_DIR:readerCDLL>/zlib_path.txt")

add_custom_command(
    TARGET database POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${READER_SRC} ${READER_DST}
    COMMENT "Copying readWrapper.py to runtime directory"
)

add_custom_command(
    TARGET statistics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${STATISTICS_SRC} ${STATISTICS_DST}
    COMMENT "Copying readWrapper.py to runtime directory"
)

add_custom_command(
    TARGET readerCDLL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${ZLIB_PATH_LOCATION} ${ZLIB_PATH_OUTPUT}
    COMMENT "Copying readWrapper.py to runtime directory"
)
