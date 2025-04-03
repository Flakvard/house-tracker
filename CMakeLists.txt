cmake_minimum_required(VERSION 3.30)

# The toolchain file setup should ideally be in a CMakePresets.json file,
# but we'll keep it here for simplicity
set(CMAKE_TOOLCHAIN_FILE "C:/Users/MarniFalkvardJoensen/scoop/apps/vcpkg/current/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(HouseTracker VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add dependencies
find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# find_package(Gumbo CONFIG REQUIRED)
find_package(Drogon CONFIG REQUIRED)

# No C++20 modules
set(CMAKE_CXX_SCAN_FOR_MODULES OFF) 

# HouseTracker executable
add_executable(HouseTracker)

# HouseTracker sources and headers
file(GLOB_RECURSE HouseTracker_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
)

target_sources(HouseTracker PRIVATE ${HouseTracker_SOURCES})

# HouseTracker include dirs
target_include_directories(HouseTracker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Preprocessor defines
target_compile_definitions(HouseTracker PRIVATE
    MY_TEST_DEFINE
)

# Automatically links to these libs
target_link_libraries(HouseTracker PRIVATE
    CURL::libcurl 
    nlohmann_json::nlohmann_json 
    Drogon::Drogon
    gumbo
)


# Add directories with any random loose .dlls or .libs here
target_link_directories(HouseTracker PRIVATE
  
)

# Set configuration properties for HouseTracker
if(MSVC)
    target_compile_options(HouseTracker PRIVATE
        /W3
        /MP # multithreaded build
        /WX # warnings as errors
        /ZI # program database for edit and continue
    )
else()
    target_compile_options(HouseTracker PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

# Set HouseTracker as the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HouseTracker)

# Filters
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${HouseTracker_SOURCES})

# Enable solution folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
