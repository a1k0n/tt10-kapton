add_library(fm STATIC
    src/adsr.cpp
    src/fm_channel.cpp
    src/tables.cpp
    src/pulse_channel.cpp
)

target_include_directories(fm
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
) 