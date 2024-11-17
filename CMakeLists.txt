cmake_minimum_required(VERSION 3.23)
project(card_layout)

set(CMAKE_CXX_STANDARD 17)

find_package(unofficial-libharu CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)


# Add your executable
add_executable(card_layout main.cpp)

target_link_libraries(card_layout PRIVATE unofficial::libharu::hpdf)
target_link_libraries(card_layout PRIVATE PNG::PNG)
target_link_libraries(card_layout PRIVATE ZLIB::ZLIB)
