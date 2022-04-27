cmake_minimum_required(VERSION 3.22)
project(direct_storage_vk_example)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(DSVK_DEPENDENCY_DIR ${CMAKE_SOURCE_DIR}/dependencies)
set(DIRECT_STORAGE_INCLUDE_DIR ${DSVK_DEPENDENCY_DIR}/include)
set(DIRECT_STORAGE_LIB_DIR ${DSVK_DEPENDENCY_DIR}/lib)

include_directories(${DIRECT_STORAGE_INCLUDE_DIR} ${Vulkan_INCLUDE_DIR})

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE DSVK_SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.c** ${CMAKE_SOURCE_DIR}/src/*.h**)

add_executable(direct_storage_vk_example ${DSVK_INCLUDE_FILES} ${DSVK_SOURCE_FILES})
target_link_libraries(direct_storage_vk_example ${DIRECT_STORAGE_LIB_DIR}/dstorage.lib
        ${DIRECT_STORAGE_LIB_DIR}/SDL2.lib
        ${DIRECT_STORAGE_LIB_DIR}/SDL2main.lib
        d3d12.lib)