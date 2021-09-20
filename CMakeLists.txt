cmake_minimum_required(VERSION 3.21)
project(ing)
set(PACKAGE_NAME ${PROJECT_NAME})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


add_library(${PROJECT_NAME} INTERFACE)
add_library(${PACKAGE_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)


macro(ing_add_interface name)
    project(${PACKAGE_NAME}_${name})
    add_library(${PROJECT_NAME} INTERFACE)
    add_library(${PACKAGE_NAME}::${name} ALIAS ${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} INTERFACE ${PACKAGE_NAME}::${PACKAGE_NAME} ${ARGN})
endmacro()

macro(ing_add_library name)
    project(${PACKAGE_NAME}_${name})
    add_library(${PROJECT_NAME} ${ARGN})
    add_library(${PACKAGE_NAME}::${name} ALIAS ${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${PACKAGE_NAME}::${PACKAGE_NAME})
endmacro()


find_package(Boost REQUIRED COMPONENTS
    timer
)

ing_add_interface(source_location Boost::headers)

ing_add_library(uptime src/uptime.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::timer)


if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

