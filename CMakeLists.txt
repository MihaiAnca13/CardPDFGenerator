cmake_minimum_required(VERSION 3.23)
project(card_layout)

set(CMAKE_CXX_STANDARD 20)

find_package(unofficial-libharu CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(raylib CONFIG REQUIRED)

if (UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)
    find_package(glfw3 REQUIRED)
endif()


# Add your executable
add_executable(
    card_layout
#        main.cpp
        CardPDFGenerator.cpp
        ui.cpp
)

target_link_libraries(card_layout PRIVATE unofficial::libharu::hpdf)
target_link_libraries(card_layout PRIVATE PNG::PNG)
target_link_libraries(card_layout PRIVATE ZLIB::ZLIB)
target_link_libraries(card_layout PRIVATE raylib)

if (UNIX AND NOT APPLE)
    target_link_libraries(card_layout PRIVATE
            OpenGL::GL
            Threads::Threads
            dl
            m
    )
    target_link_libraries(card_layout PRIVATE glfw)
endif()


# copy front_images folder with all images to build directory
file(COPY front_images DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY back.png DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY back_images DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY fonts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})