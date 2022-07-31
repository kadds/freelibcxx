cmake_minimum_required (VERSION 3.3)

project(freelibcxx CXX)

file(GLOB_RECURSE HPPS include/*.hpp)
file(GLOB_RECURSE SRCS src/*.cc)

add_library(freelibcxx STATIC ${HPPS} ${SRCS})
target_include_directories(freelibcxx PUBLIC include)

set_target_properties(freelibcxx PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(freelibcxx PROPERTIES COMPILE_FLAGS "-Wunreachable-code -pipe -std=c++20 -Wno-pedantic \
    -fno-rtti -fno-asynchronous-unwind-tables -fno-common \
    -fno-plt -fno-exceptions -fno-pic -fpie -Wall \
    -ffreestanding -fno-stack-protector -fno-builtin")


add_subdirectory(3rd/Catch2)
include(CTest)
include(Catch)

enable_testing()
function(add_test_execute target files)    
    add_executable(freelibcxx_test_${target} ${files} "test/common.cc")
    target_link_libraries(freelibcxx_test_${target} PRIVATE freelibcxx Catch2::Catch2WithMain)
    target_include_directories(freelibcxx_test_${target} PUBLIC test/)
    set_target_properties(freelibcxx_test_${target} PROPERTIES COMPILE_FLAGS "-std=c++20")
    catch_discover_tests(freelibcxx_test_${target})
endfunction(add_test_execute)


add_test_execute(vector "test/vector.cc")
add_test_execute(string "test/string.cc")
add_test_execute(list "test/list.cc")
add_test_execute(slist "test/slist.cc")
add_test_execute(circular_buffer "test/circular_buffer.cc")
add_test_execute(trunk_buffer "test/trunk_buffer.cc")
add_test_execute(skip_list "test/skip_list.cc")
add_test_execute(random "test/random.cc")
add_test_execute(bit_set "test/bit_set.cc")

