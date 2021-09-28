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

macro(ing_java_test name)
    project(${TEST}_${name})
    if(TARGET ${PACKAGE_NAME}::jmain)

    add_library(${PROJECT_NAME} SHARED test.cpp test_${name}.cpp)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MODULE=$<UPPER_CASE:${name}>)
    target_link_libraries(${PROJECT_NAME} PRIVATE Boost::unit_test_framework ${PACKAGE_NAME}::jmain)
    if(TARGET ${PACKAGE_NAME}::${name})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PACKAGE_NAME}::${name})
    endif()

    add_test(NAME ${PROJECT_NAME} COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/java.cmake --
        JniMain.java ${PROJECT_NAME} ${ARGN}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )

    endif()
endmacro()


ing_add_test(source_location test_std_source_location.cpp)

ing_add_test(uptime)

ing_add_test(threading)

ing_java_test(jmain)

ing_add_test(logging)
