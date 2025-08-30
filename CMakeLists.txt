cmake_minimum_required(VERSION 3.20)
project(nohang-tr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE)
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Ayatana AppIndicator 3 (optional but recommended for GNOME)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AyatanaAppIndicator3 QUIET ayatana-appindicator3-0.1)
if(AyatanaAppIndicator3_FOUND)
  add_compile_definitions(HAVE_AYATANA_APPINDICATOR3=1)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

install(FILES config/nohang-tr.example.toml DESTINATION share/nohang-tr)
