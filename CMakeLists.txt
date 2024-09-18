cmake_minimum_required(VERSION 3.14)
project(nine-medics)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH "./vcpkg_installed/x64-linux")

# Include vcpkg toolchain file
set(CMAKE_TOOLCHAIN_FILE "/opt/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

# Find the nlohmann_json package
find_package(nlohmann_json CONFIG REQUIRED)

# Add your executable
add_executable(nine-medics main.cpp)

# Link against nlohmann_json
target_link_libraries(nine-medics PRIVATE nlohmann_json::nlohmann_json)