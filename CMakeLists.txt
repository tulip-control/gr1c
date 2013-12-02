cmake_minimum_required (VERSION 2.6)
# SCL; 2 Dec 2013.

project (gr1c)
set (CUDD_ROOT extern/cudd-2.5.0)
add_definitions (-DHAVE_IEEE_754 -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8)
set (USE_READLINE 0)

include_directories (${CUDD_ROOT}/include)
find_package (BISON)
find_package (FLEX)

bison_target (SpecParser gr1c_parse.y ${CMAKE_CURRENT_BINARY_DIR}/y.tab.c COMPILE_FLAGS "-y -d")
flex_target (SpecScanner gr1c_scan.l ${CMAKE_CURRENT_BINARY_DIR}/gr1c_scan.c)
add_flex_bison_dependency (SpecScanner SpecParser)

include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_executable (gr1c main.c interactive.c solve_operators.c solve.c ptree.c automaton.c ${BISON_SpecParser_OUTPUTS} ${FLEX_SpecScanner_OUTPUTS})
install (TARGETS gr1c RUNTIME DESTINATION bin)

find_library (cudd libcudd.a PATHS ${CMAKE_CURRENT_SOURCE_DIR}/${CUDD_ROOT}/cudd NO_DEFAULT_PATH)
find_library (mtr libmtr.a PATHS ${CMAKE_CURRENT_SOURCE_DIR}/${CUDD_ROOT}/mtr NO_DEFAULT_PATH)
find_library (st libst.a PATHS ${CMAKE_CURRENT_SOURCE_DIR}/${CUDD_ROOT}/st NO_DEFAULT_PATH)
find_library (util libutil.a PATHS ${CMAKE_CURRENT_SOURCE_DIR}/${CUDD_ROOT}/util NO_DEFAULT_PATH)
find_library (epd libepd.a PATHS ${CMAKE_CURRENT_SOURCE_DIR}/${CUDD_ROOT}/epd NO_DEFAULT_PATH)
target_link_libraries (gr1c ${cudd} ${mtr} ${st} ${util} ${epd} m)

if (${USE_READLINE})
  message (STATUS "Using GNU Readline")
  add_definitions (-DUSE_READLINE)
  find_library (readline readline)
  target_link_libraries (gr1c ${readline})
else (${USE_READLINE})
  message (STATUS "Not using GNU Readline")
endif (${USE_READLINE})
