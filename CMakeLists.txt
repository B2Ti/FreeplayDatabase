cmake_minimum_required(VERSION 3.25)

project(FreeplayDatabase)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(MATH m)

find_package(ZLIB REQUIRED)

file(GLOB SOURCE_FILES "src/*.c")

add_library(SOURCE SHARED ${SOURCE_FILES})
target_link_libraries(SOURCE ZLIB::ZLIB ${MATH})

include_directories("${PROJECT_SOURCE_DIR}/include")

add_executable(database "database.c")
add_executable(printing "printing.c")
add_executable(statistics "statistics.c")

target_link_libraries(printing SOURCE)
target_link_libraries(database SOURCE)
target_link_libraries(statistics SOURCE)

add_library(readerCDLL SHARED "reader/reader.c")

if (WIN32)
    set_target_properties(readerCDLL PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_link_libraries(readerCDLL SOURCE)

configure_file("reader/readWrapper.py" "readWrapper.py" COPYONLY)
