cmake_minimum_required(VERSION 3.21)

find_package(Java QUIET REQUIRED COMPONENTS Development)

set(Source ${CMAKE_ARGV4})
cmake_path(ABSOLUTE_PATH Source BASE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} NORMALIZE)
cmake_path(GET Source STEM Class)

set(ARGN [[]])
foreach(i RANGE 5 ${CMAKE_ARGC})
	list(APPEND ARGN ${CMAKE_ARGV${i}})
endforeach()

execute_process(COMMAND ${Java_JAVAC_EXECUTABLE} -d . ${Source} COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${Java_JAVA_EXECUTABLE} -Djava.library.path=. ${Class} ${ARGN} COMMAND_ERROR_IS_FATAL ANY)
