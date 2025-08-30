add_executable(nohang-tr
  main.cpp
  tray.cpp
  config.cpp
  system_probe.cpp
)

# Place the binary in the top-level build directory so it can be
# run as `./build/nohang-tr` after building.
set_target_properties(nohang-tr PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(nohang-tr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(nohang-tr
  Qt6::Widgets
)

install(TARGETS nohang-tr RUNTIME DESTINATION bin)
