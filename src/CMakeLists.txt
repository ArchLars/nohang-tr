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

if(AyatanaAppIndicator3_FOUND)
  target_link_libraries(nohang-tr ${AyatanaAppIndicator3_LIBRARIES})
  target_compile_options(nohang-tr PRIVATE ${AyatanaAppIndicator3_CFLAGS_OTHER})
  target_include_directories(nohang-tr PRIVATE ${AyatanaAppIndicator3_INCLUDE_DIRS})
endif()

install(TARGETS nohang-tr RUNTIME DESTINATION bin)
