cmake_minimum_required (VERSION 3.22)
project (sep)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INSTALL_PREFIX $ENV{HOME})
find_package(PkgConfig REQUIRED)
add_executable(sep main.cpp)
pkg_check_modules(NOTIFY libnotify)
include_directories(sep ${NOTIFY_INCLUDE_DIRS})
target_link_libraries(sep ${NOTIFY_LIBRARIES} )

install(TARGETS sep RUNTIME DESTINATION bin)

