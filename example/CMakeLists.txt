add_executable(rectpack2D-example)

target_sources(
    rectpack2D-example
    PUBLIC
        main.cpp
)

target_link_libraries(
    rectpack2D-example
    PUBLIC
        rectpack2D::rectpack2D
)

# Set C++ standard to 17
set_property(
    TARGET rectpack2D-example
    PROPERTY
    CXX_STANDARD 17
)

set(RESULT_EXE_WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Enable LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT output)

if(IPO_SUPPORTED)
    set_target_properties(
        rectpack2D-example
        PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()

if(MSVC)
    # Set executable working directory to current folder if debugging in vs
    set_target_properties(
        rectpack2D-example
        PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY ${RESULT_EXE_WORKING_DIR}
    )

    # Set executable as a startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT rectpack2D-example)

    target_compile_options(
        rectpack2D-example
        PUBLIC
            /permissive-
    )
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(
        rectpack2D-example
        PUBLIC
            -Wall
            -Werror
            -Wextra
            -ftemplate-backtrace-limit=0
    )
endif()
