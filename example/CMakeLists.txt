add_executable(rectpack2D-example)

message("Build type: ${CMAKE_BUILD_TYPE}")

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

set(RESULT_EXE_WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Enable LTO (only in release, not debug with sanitizers)
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT output)

if(IPO_SUPPORTED AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(
        rectpack2D-example
        PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()

if(MSVC)
    set_target_properties(
        rectpack2D-example
        PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY ${RESULT_EXE_WORKING_DIR}
    )

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT rectpack2D-example)

    target_compile_options(
        rectpack2D-example
        PUBLIC
            /permissive-
    )

    # Add runtime checks only in Debug builds (they conflict with /O2)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        target_compile_options(
            rectpack2D-example
            PUBLIC
                /RTC1
        )
    endif()
else()
    # GCC or Clang setup
    target_compile_options(
        rectpack2D-example
        PUBLIC
            -Wall
            -Werror
            -Wextra
            -Wshadow
            -ftemplate-backtrace-limit=0
    )

    # Enable sanitizers in Debug mode
	if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        message(STATUS "Enabling Address + Undefined sanitizers for Debug build")
		message(STATUS "THIS MIGHT SLOW DOWN THE EXAMPLE!") 
		message(STATUS "If you want to disable sophisticated debugging, specify -DCMAKE_BUILD_TYPE=Release") 

        target_compile_options(
            rectpack2D-example
            PUBLIC
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
                -O1
        )

        target_link_options(
            rectpack2D-example
            PUBLIC
                -fsanitize=address,undefined
        )
    endif()
endif()

add_custom_target(
    run
    COMMAND rectpack2D-example
    DEPENDS rectpack2D-example
    WORKING_DIRECTORY ${RESULT_EXE_WORKING_DIR}
    COMMENT "Running rectpack2D-example..."
    VERBATIM
)
