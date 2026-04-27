cmake_minimum_required(VERSION 3.27)
project(hyprfence VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC "src/*.cpp")
add_library(hyprfence SHARED ${SRC})

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET
    hyprland
    pixman-1
    libdrm
    wayland-server
    xkbcommon
)

target_link_libraries(hyprfence PRIVATE PkgConfig::deps)
install(TARGETS hyprfence DESTINATION lib)
