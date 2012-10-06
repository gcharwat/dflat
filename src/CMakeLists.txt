cmake_minimum_required(VERSION 2.8)
project(semi)

add_definitions("-Wall -pedantic -ansi -DDISABLE_MULTI_THREADING")
set(CMAKE_CXX_FLAGS_GPROF "-O3 -g -pg")
set(CMAKE_LINKER_FLAGS_GPROF "-pg")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -DDEBUG") # Without -D_DEBUG there will be voodoo in libclasp's pod_vector.h, since then the size of our Clasp::PodVector is different than when we built clasp's debug release

# Unfortunately libclasp does not compile with -pedantic.
# Using -isystem instead of -I; then they are treated as system headers,
# i.e., warnings are suppressed for the clasp headers.
include_directories(SYSTEM ${clasp_dir}/libclasp)
include_directories(${sharp_dir}/include ${gringo_dir}/libgringo)
link_directories(${sharp_dir}/src/.libs)

add_library(gringo STATIC IMPORTED)
set_target_properties(gringo PROPERTIES IMPORTED_LOCATION ${gringo_lib})
add_library(clasp STATIC IMPORTED)
set_target_properties(clasp PROPERTIES IMPORTED_LOCATION ${clasp_lib})

find_library(gmpxx gmpxx)
find_package(Boost COMPONENTS system filesystem timer REQUIRED)

add_executable(sat Algorithm.cpp ClaspAlgorithm.cpp ClaspInputReader.cpp GringoOutputProcessor.cpp sat/Problem.cpp sat/ClaspAlgorithm.cpp sat/CppAlgorithm.cpp sat/CppAlgorithmNormalized.cpp sat/ClaspCallback.cpp sat/GringoOutputProcessor.cpp sat/Tuple.cpp sat/main.cpp)
target_link_libraries(sat libsharp.a gringo clasp gmpxx ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_TIMER_LIBRARY})

add_executable(3col Algorithm.cpp ClaspAlgorithm.cpp ClaspInputReader.cpp GringoOutputProcessor.cpp 3col/Problem.cpp 3col/ClaspAlgorithm.cpp 3col/ClaspCallback.cpp 3col/GringoOutputProcessor.cpp 3col/Tuple.cpp 3col/main.cpp)
target_link_libraries(3col libsharp.a gringo clasp gmpxx ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_TIMER_LIBRARY})