cmake_minimum_required (VERSION 3.3)

project(freelibcxx CXX)
option(FREELIBCXX_TEST "enable testing for freelibcxx" OFF)

file(GLOB_RECURSE HPPS include/*.hpp)
file(GLOB_RECURSE SRCS src/*.cc)

add_library(freelibcxx STATIC ${HPPS} ${SRCS})
target_include_directories(freelibcxx PUBLIC include)

set_target_properties(freelibcxx PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(freelibcxx PROPERTIES COMPILE_FLAGS "-Wunreachable-code -pipe -std=c++20 -Wno-pedantic \
    -fno-rtti -fno-asynchronous-unwind-tables -fno-common \
    -fno-plt -fno-exceptions -fno-pic -fpie -Wall \
    -ffreestanding -fno-stack-protector -fno-builtin")

function(add_test_execute target files)
    add_executable(freelibcxx_test_${target} ${files} "test/common.cc")
    target_link_libraries(freelibcxx_test_${target} PRIVATE freelibcxx Catch2::Catch2WithMain)
    target_link_libraries(freelibcxx_test_${target} PRIVATE ${ARGV3})

    target_include_directories(freelibcxx_test_${target} PUBLIC test/)
    set_target_properties(freelibcxx_test_${target} PROPERTIES COMPILE_FLAGS "${ARGV2} -std=c++20")

endfunction(add_test_execute)

if (FREELIBCXX_TEST)
    set(TEST_FLAGS "")
    set (TEST_LIBS "")
    add_subdirectory(3rd/Catch2)
    include(CTest)
    include(Catch)
    enable_testing()
    set (GNUC OFF)

    if("${CMAKE_C_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
        message("Building with llvm Code Coverage Tools")

        find_program(LLVM_COV_PATH llvm-cov)
        # Warning/Error messages
        if(NOT LLVM_COV_PATH)
            message(FATAL_ERROR "llvm-cov not found! Aborting.")
        endif()

        # set Flags
        set(TEST_FLAGS "${TEST_FLAGS} -fprofile-instr-generate -fcoverge-mapping")

    elseif(CMAKE_COMPILER_IS_GNUCXX)
        message("Building with lcov Code Coverage Tools")
        set(GNUC ON)

        # Warning/Error messages
        if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
            message(WARNING "Code coverage results with an optimized (non-Debug) build may be misleading")
        endif()

        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)

        if(NOT LCOV_PATH)
            message(FATAL_ERROR "lcov not found! Aborting...")
        endif()
        if(NOT GENHTML_PATH)
            message(FATAL_ERROR "genhtml not found! Aborting...")
        endif()

        set(TEST_FLAGS "${TEST_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(TEST_LIBS "${TEST_LIBS} -lgcov")
    else()
        message(FATAL_ERROR "Code coverage requires Clang or GCC. Aborting.")
    endif()

    if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
        set(TEST_FLAGS "-fsanitize=address")
        set(TEST_LIBS "-lasan")
    else()
        set(TEST_FLAGS "${TEST_FLAGS} -fsanitize=address")
        set(TEST_LIBS "${TEST_LIBS} -lasan")
        string(STRIP ${TEST_FLAGS} TEST_FLAGS)
        string(STRIP ${TEST_LIBS} TEST_LIBS)
    endif()

    add_test_execute(vector "test/vector.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(string "test/string.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(list "test/list.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(slist "test/slist.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(circular_buffer "test/circular_buffer.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(trunk_buffer "test/trunk_buffer.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(skip_list "test/skip_list.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(random "test/random.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(bit_set "test/bit_set.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    set(ALLSRC "test/vector.cc" "test/string.cc" "test/list.cc"
        "test/slist.cc" "test/circular_buffer.cc" "test/trunk_buffer.cc" 
        "test/skip_list.cc" "test/random.cc" "test/bit_set.cc")
    
    MESSAGE("flags: ${TEST_FLAGS}")
    add_test_execute(all_in_one "${ALLSRC}" ${TEST_FLAGS} ${TEST_LIBS})
    catch_discover_tests(freelibcxx_test_all_in_one)


endif(FREELIBCXX_TEST)