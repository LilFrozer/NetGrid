cmake_minimum_required(VERSION 3.10)
project(local_scaner_prj)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_path(BOOST_INCLUDE_DIR
    NAMES boost/beast.hpp
    PATHS /opt/homebrew/include
    /opt/homebrew/opt/boost/include
    /usr/local/include
    /usr/include
    REQUIRED
)
set(FLTK_DIR "/opt/homebrew/lib/cmake/FLTK")
find_package(FLTK REQUIRED)

add_executable(local_scaner_prj
    src/main.cc
    inc/LocalNetworkScaner.h
    src/LocalNetworkScaner.cc
    inc/Window.h
    src/Window.cc
)

target_include_directories(local_scaner_prj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
    ${BOOST_INCLUDE_DIR}
    ${FLTK_INCLUDE_DIRS})

if(APPLE)
# enable_language(OBJCXX)
# set_source_files_properties(Clipboard.mm PROPERTIES
# LANGUAGE OBJCXX
# )
find_package(OpenSSL REQUIRED)
target_link_libraries(local_scaner_prj
    "-framework CoreFoundation"
    "-framework CoreGraphics"
    "-framework ApplicationServices"
    ${FLTK_LIBRARIES}
)
target_link_libraries(local_scaner_prj
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)
target_compile_definitions(local_scaner_prj PRIVATE
    BOOST_BEAST_USE_STD_ERIC=1
    BOOST_ASIO_USE_STD_ERIC=1
)
endif()
