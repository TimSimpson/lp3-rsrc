cmake_minimum_required(VERSION 3.5)
project(test_package)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

# TODO: Find aa way to make the CMake package config bring this in
# automatically.
find_package(fmt REQUIRED)
find_package(Lp3_Main CONFIG REQUIRED)
find_package(Lp3_Sdl CONFIG REQUIRED)
find_package(Lp3_Rsrc CONFIG REQUIRED)
# ~start-doc cmake-example


# Create a simple executable which uses SDL2
add_executable(ZipWorld WIN32 ${CMAKE_CURRENT_SOURCE_DIR}/ZipWorld.cpp)
# Associate lp3_sdl as a dependency. Pulls in SDL2 and SDL2 image libraries
# from BinCrafters. For Emscripten builds, adds appropriate compile and link
# options to include built-in SDL2 support.
target_link_libraries(ZipWorld PUBLIC lp3::lp3_rsrc)
# ~end-doc cmake-example
target_link_libraries(ZipWorld PUBLIC lp3::lp3_sdl lp3::lp3_main)


if (NOT "${CMAKE_SYSTEM_NAME}" MATCHES "Emscripten")
    find_package(sdl2_image 2.0.5 REQUIRED)
    target_link_libraries(ZipWorld
        PUBLIC sdl2_image::sdl2_image
    )
else()
    target_compile_options(ZipWorld PUBLIC
        "SHELL:-s USE_SDL_IMAGE=2"
        "SHELL:-s SDL2_IMAGE_FORMATS=[\"png\",\"bmp\"]"
    )
    set_target_properties(ZipWorld PROPERTIES
        SUFFIX ".html"
        LINK_FLAGS "--emrun"
    )
    target_link_options(ZipWorld PRIVATE
        "SHELL:-s USE_SDL_IMAGE=2"
        "SHELL:-s SDL2_IMAGE_FORMATS=[\"png\",\"bmp\"]"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/media@/"
    )
endif()
