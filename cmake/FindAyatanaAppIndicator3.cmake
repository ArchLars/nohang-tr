# Simple passthrough to pkg-config for consistency if needed by IDEs
find_package(PkgConfig REQUIRED)
pkg_check_modules(AyatanaAppIndicator3 REQUIRED ayatana-appindicator3-0.1)
