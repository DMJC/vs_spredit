cmake_minimum_required(VERSION 3.10)
project(vs_spredit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(CAIROMM REQUIRED cairomm-1.0)

add_executable(vs_spredit vegastrike_animation.cpp)

target_include_directories(vs_spredit PRIVATE
    ${GTKMM_INCLUDE_DIRS}
    ${CAIROMM_INCLUDE_DIRS}
)

target_link_directories(vs_spredit PRIVATE
    ${GTKMM_LIBRARY_DIRS}
    ${CAIROMM_LIBRARY_DIRS}
)

target_link_libraries(vs_spredit PRIVATE
    ${GTKMM_LIBRARIES}
    ${CAIROMM_LIBRARIES}
)

target_compile_options(vs_spredit PRIVATE
    ${GTKMM_CFLAGS_OTHER}
    ${CAIROMM_CFLAGS_OTHER}
)

install(TARGETS vs_spredit
    RUNTIME DESTINATION bin
)
