find_package(Catch2 3 QUIET)
if (Catch2_FOUND)
  add_executable(unit-test
      test_config.cpp
      test_system_probe.cpp
      test_tray.cpp
      test_config_path.cpp
      ../src/system_probe.cpp
      ../src/tray.cpp
      ../src/config.cpp)
  target_include_directories(unit-test PRIVATE ../src)
  target_link_libraries(unit-test
      Catch2::Catch2WithMain
      Qt6::Widgets)
  set_target_properties(unit-test PROPERTIES AUTOMOC ON)
  add_test(NAME unit COMMAND unit-test)
else()
  message(WARNING "Catch2 not found. Tests will be skipped.")
endif()

