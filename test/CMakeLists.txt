project(${PACKAGE_NAME}_test)
set(TEST ${PROJECT_NAME})

find_package(Boost REQUIRED COMPONENTS unit_test_framework)

macro(ing_add_test name)
    project(${TEST}_${name})
    add_executable(${PROJECT_NAME} test.cpp test_${name}.cpp ${ARGN})
    target_compile_definitions(${PROJECT_NAME} PUBLIC MODULE=$<UPPER_CASE:${name}>)
    target_link_libraries(${PROJECT_NAME} PUBLIC Boost::unit_test_framework ${PACKAGE_NAME}::${name})
    add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endmacro()


ing_add_test(source_location test_std_source_location.cpp)

