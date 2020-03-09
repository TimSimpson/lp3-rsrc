# *********************************************************************
# LP3_RSRC
#       Handles very rudimentary, basic stuff all the other libs need.
# *********************************************************************
cmake_minimum_required(VERSION 3.9.0 FATAL_ERROR)
project(Lp3_Rsrc CXX)

include(CMakePackageConfigHelpers)
include(CTest)

include("${CMAKE_BINARY_DIR}/conan_paths.cmake" OPTIONAL
        RESULT_VARIABLE using_conan)

include("${CMAKE_BINARY_DIR}/conan_paths.cmake" OPTIONAL
        RESULT_VARIABLE using_conan)
if(NOT using_conan)
    include("${CMAKE_MODULE_PATH}/conan_paths.cmake" OPTIONAL
            RESULT_VARIABLE using_conan)
endif()
if(using_conan)
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
endif()

get_directory_property(is_child PARENT_DIRECTORY)
option(LP3_RSRC_Build_Tests "Build tests when BUILD_TESTING is enabled."
       $<NOT:${is_child}>)

find_package(ZLIB REQUIRED)
find_package(fmt REQUIRED)
find_package(Lp3_Sdl CONFIG REQUIRED)

add_library(
    lp3_rsrc
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lp3/rsrc.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lp3/rsrc/config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lp3/rsrc/containers.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lp3/rsrc/directory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lp3/rsrc/env_vars.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lp3/rsrc/ZipFile.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lp3/rsrc/zip_utils.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lp3/rsrc/zip/stream.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/directory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ZipFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/zip_utils.cpp)
set_target_properties(lp3_rsrc PROPERTIES OUTPUT_NAME "Lp3_Rsrc")

# Mandate the use of at least C++17 by everything that uses this
target_compile_features(lp3_rsrc PUBLIC cxx_std_17)
# Is this Hell? See : https://stackoverflow.com/questions/60042498/cmake-idiom-for-overcoming-libstdc-filesystem-weirdness#
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(lp3_rsrc "stdc++fs")
endif()

# set_property(TARGET lp3_rsrc PROPERTY CXX_STANDARD 17)

target_include_directories(
    lp3_rsrc
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
    PRIVATE src)

target_link_libraries(lp3_rsrc lp3::lp3_sdl fmt::fmt ZLIB::ZLIB)

# This is built as a "shared library" in sarcastic air quotes. It's only
# made that way to make linking faster, and relies on consumers using the same
# version of the runtime it was built with. IIRC this used to not be that big
# of a problem with VS, but now it is, thus the disabled warnings.
if(BUILD_SHARED_LIBS)
    target_compile_definitions(
        lp3_rsrc
        PUBLIC LP3_RSRC_API_DYNAMIC
        PRIVATE LP3_RSRC_API_CREATE)
    if(MSVC)
        target_compile_options(lp3_rsrc PRIVATE /wd4251 /wd4275)
    endif()
endif()

# if ("${CMAKE_SYSTEM_NAME}" MATCHES "Emscripten")
#     target_link_options(directory_tests PRIVATE
#         "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources@/resources"
#     )
# endif()

add_executable(zip_ls ${CMAKE_CURRENT_SOURCE_DIR}/apps/zip_ls.cpp)
target_link_libraries(zip_ls lp3_rsrc)

add_executable(unzip ${CMAKE_CURRENT_SOURCE_DIR}/apps/unzip.cpp)
target_link_libraries(unzip lp3_rsrc)

add_executable(open_file ${CMAKE_CURRENT_SOURCE_DIR}/apps/open_file.cpp)
target_link_libraries(open_file lp3_rsrc)

# add_executable(open_file_ex ${CMAKE_CURRENT_SOURCE_DIR}/apps/open_file_ex.cpp)
# target_link_libraries(open_file_ex lp3_rsrc)

# *********************************************************************
# Package / Install Stuff
# *********************************************************************

install(DIRECTORY include/ DESTINATION include)

install(
    TARGETS lp3_rsrc
    EXPORT Lp3_Rsrc-targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES
    DESTINATION include)

add_library(lp3::rsrc ALIAS lp3_rsrc)
install(
    EXPORT Lp3_Rsrc-targets
    FILE Lp3_Rsrc-targets.cmake
    NAMESPACE lp3::
    DESTINATION lib/cmake/Lp3_Rsrc)

file(
    WRITE "${PROJECT_BINARY_DIR}/Lp3_RsrcConfig.cmake"
    "
include(CMakeFindDependencyMacro)
if (NOT \"\${CMAKE_SYSTEM_NAME}\" MATCHES \"Emscripten\")
    find_dependency(rsrc)
    find_dependency(rsrc_image)
endif()
include(\"\${CMAKE_CURRENT_LIST_DIR}/Lp3_Rsrc-targets.cmake\")
")

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/Lp3_RsrcConfigVersion.cmake"
    VERSION 1.0.1
    COMPATIBILITY AnyNewerVersion)

install(FILES "${PROJECT_BINARY_DIR}/Lp3_RsrcConfig.cmake"
              "${PROJECT_BINARY_DIR}/Lp3_RsrcConfigVersion.cmake"
        DESTINATION lib/cmake/Lp3_Rsrc)

# *********************************************************************
# Tests and Drivers / Demos
# *********************************************************************
function(define_tests)
    find_package(Catch2 CONFIG REQUIRED)

    function(make_test exe_target)
        if("${CMAKE_SYSTEM_NAME}" MATCHES "Emscripten")
            add_test(NAME "test_${exe_target}"
                     COMMAND node $<TARGET_FILE:${exe_target}>
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
            target_compile_options(${exe_target} PUBLIC "SHELL:-s DISABLE_EXCEPTION_CATCHING=0 -s DEMANGLE_SUPPORT=1")
            target_link_options(${exe_target} PUBLIC "SHELL:-s DISABLE_EXCEPTION_CATCHING=0 -s DEMANGLE_SUPPORT=1")
        else()
            add_test(NAME "test_${exe_target}" COMMAND ${exe_target}
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endif()
    endfunction()

    add_executable(directory_tests
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests/directory_tests.cpp)
    target_link_libraries(directory_tests lp3_rsrc Catch2::Catch2)
    make_test(directory_tests)

    add_executable(unzip_tests
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests/unzip_tests.cpp)
    target_link_libraries(unzip_tests lp3_rsrc Catch2::Catch2)
    make_test(unzip_tests)
endfunction()


if(BUILD_TESTING AND LP3_SDL_Build_Tests)
    # include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tests.cmake)
    define_tests()
endif()

get_directory_property(is_child PARENT_DIRECTORY)
if(NOT is_child)
    set(LP3_RSRC_FULL True)
endif()

if(LP3_RSRC_FULL)
    if(LP3_SETUP_FOLDERS)
        set_target_properties(lp3_rsrc PROPERTIES FOLDER lp3_rsrc)
    endif()
endif()
