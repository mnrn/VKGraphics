cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)

set(PROJECT_NAME ReVK)
project(${PROJECT_NAME})

set(PROJECTS_DIR_NAME Projects)
include_directories(SYSTEM third-party)
include_directories(Core)
include_directories(Common)

# vulkan
find_package(Vulkan REQUIRED)
message(STATUS "@@Vulkan_FOUND: ${Vulkan_FOUND}")
message(STATUS "@@Vulkan_INCLUDE_DIR: ${Vulkan_INCLUDE_DIR}")
message(STATUS "@@Vulkan_LIBRARY: ${Vulkan_LIBRARY}")
include_directories(${Vulkan_INCLUDE_DIR})

# glfw
find_library(GLFW_LIBRARIES
    NAMES
    glfw
    glfw3
    glfw3.3
    HINTS
    ${CMAKE_SOURCE_DIR}/Lib/glfw
    )
message("@@ GLFW_LIBRARIES: ${GLFW_LIBRARIES}")

# imgui
include_directories(SYSTEM third-party/imgui)


# Warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wundef")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat -Wno-c++98-compat-pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-zero-as-null-pointer-constant -Wno-sign-conversion -Wno-disabled-macro-expansion -Wno-undefined-func-template -Wno-deprecated-dynamic-exception-spec -Wno-undef") # for Boost
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-signed-enum-bitfield -Wno-shorten-64-to-32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-prototypes -Wno-global-constructors -Wno-exit-time-destructors -Wno-padded")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-weak-vtables -Wno-covered-switch-default")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-poison-system-directories -Wno-ctad-maybe-unsupported")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-old-style-cast -Wno-double-promotion -Wno-unused-macros")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-documentation")
endif ()

# Function for building
function(build TARGET_NAME)
    # Main
    file(GLOB SOURCE
        *.cc
        Core/VK/*.cc
        Core/VK/Image/*.cc
        Core/VK/Pipeline/*.cc
        Core/VK/Buffer/*.cc
        Core/VK/Sync/*.cc
        ${PROJECTS_DIR_NAME}/${TARGET_NAME}/*.cc
        )
    set(MAIN_CC ${TARGET_NAME}/${TARGET_NAME}/Main.cc)
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECTS_DIR_NAME}/${TARGET_NAME}/Main.cc)
        set(MAIN_CC ${PROJECTS_DIR_NAME}/${TARGET_NAME}/Main.cc)
    endif ()

    add_executable(${TARGET_NAME}
        ${MAIN_CC}
        ${SOURCE}
        )
    target_link_libraries(${TARGET_NAME}
        ${Vulkan_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT}
        ${GLFW_LIBRARIES}
        )
endfunction(build)

# Build All
function(buildAll)
    foreach (TARGET ${TARGETS})
        build(${TARGET})
    endforeach (TARGET)
endfunction(buildAll)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/")
set(TARGETS
    #MinimalVK
    #ListsLayers
    #LoadValidationLayers
    HelloTriangle
    )
buildAll()
