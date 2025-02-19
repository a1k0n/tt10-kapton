cmake_minimum_required(VERSION 3.15)
project(tt10-fm)

# Initialize conan if using the conan toolchain file
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

add_subdirectory(libfm)
add_subdirectory(standalone)
#add_subdirectory(vst) 
