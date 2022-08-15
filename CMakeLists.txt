cmake_minimum_required (VERSION 3.3)

project(freelibcxx CXX)
option(FREELIBCXX_TEST "enable testing for freelibcxx" OFF)

set(TEST_FLAGS "")
set (TEST_LIBS "")
if (FREELIBCXX_TEST)
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
        set(TEST_FLAGS "${TEST_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        set(TEST_LIBS "${TEST_LIBS} -fprofile-instr-generate -fcoverage-mapping")

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
endif()

file(GLOB_RECURSE HPPS include/*.hpp)
file(GLOB_RECURSE SRCS src/*.cc)

add_library(freelibcxx STATIC ${HPPS} ${SRCS})
target_include_directories(freelibcxx PUBLIC include)

set_target_properties(freelibcxx PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(freelibcxx PROPERTIES COMPILE_FLAGS 
    "-Wunreachable-code -pipe -std=c++20 -Wno-pedantic \
    -fno-rtti -fno-asynchronous-unwind-tables -fno-common \
    -fno-plt -fno-exceptions -fno-pic -fpie -Wall \
    -ffreestanding -fno-stack-protector -fno-builtin ${TEST_FLAGS}")

function(add_test_execute target files)
    add_executable(freelibcxx_test_${target} ${files} "test/common.cc")
    target_link_libraries(freelibcxx_test_${target} PRIVATE freelibcxx Catch2::Catch2WithMain)
    target_link_libraries(freelibcxx_test_${target} PRIVATE ${ARGV3})

    target_include_directories(freelibcxx_test_${target} PUBLIC test/)
    set_target_properties(freelibcxx_test_${target} PROPERTIES COMPILE_FLAGS "${ARGV2} -std=c++20")

endfunction(add_test_execute)

if (FREELIBCXX_TEST)
    add_test_execute(vector "test/vector.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(string "test/string.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(list "test/list.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(slist "test/slist.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(circular_buffer "test/circular_buffer.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(trunk_buffer "test/trunk_buffer.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(skip_list "test/skip_list.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(random "test/random.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(bit_set "test/bit_set.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(hashmap "test/hashmap.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(formatter "test/formatter.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(time "test/time.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    add_test_execute(buddy "test/buddy.cc"  ${TEST_FLAGS} ${TEST_LIBS})
    set(ALLSRC "test/vector.cc" "test/string.cc" "test/list.cc"
        "test/slist.cc" "test/circular_buffer.cc" "test/trunk_buffer.cc" 
        "test/skip_list.cc" "test/random.cc" "test/bit_set.cc" "test/hashmap.cc"
        "test/formatter.cc" "test/time.cc" "test/buddy.cc")
    
    MESSAGE("flags: ${TEST_FLAGS}")
    add_test_execute(all_in_one "${ALLSRC}" ${TEST_FLAGS} ${TEST_LIBS})
    catch_discover_tests(freelibcxx_test_all_in_one)

    
    set(TARGET_NAME "freelibcxx_test_all_in_one")

    if (CMAKE_COMPILER_IS_GNUCXX)
        add_custom_target(${TARGET_NAME}-lcov-preprocessing
            COMMAND $<TARGET_FILE:${TARGET_NAME}>
            COMMAND lcov -c -d ./CMakeFiles/${TARGET_NAME}.dir -o ${TARGET_NAME}-init.info
            DEPENDS ${TARGET_NAME})
        
        add_custom_target(${TARGET_NAME}-lcov
            COMMAND lcov --remove ${TARGET_NAME}-init.info 
                '*/usr/include/*' '*/usr/lib/*' '*/usr/lib64/*' '*/usr/local/include/*' '*/usr/local/lib/*' '*/usr/local/lib64/*' '*/3rd/Catch2/*'
                 -o ${TARGET_NAME}-final.info
            COMMAND genhtml -o ${TARGET_NAME}_report --legend --title "lcov"  --prefix=./ ${TARGET_NAME}-final.info
            DEPENDS ${TARGET_NAME}-lcov-preprocessing)

    else()
        # llvm-cov
        add_custom_target(${TARGET_NAME}-ccov-preprocessing
            COMMAND LLVM_PROFILE_FILE=${TARGET_NAME}.profraw $<TARGET_FILE:${TARGET_NAME}>
            COMMAND llvm-profdata merge -sparse ${TARGET_NAME}.profraw -o ${TARGET_NAME}.profdata
            DEPENDS ${TARGET_NAME})

        add_custom_target(${TARGET_NAME}-ccov
            COMMAND llvm-cov show $<TARGET_FILE:${TARGET_NAME}> -instr-profile=${TARGET_NAME}.profdata -show-line-counts-or-regions 
                -output-dir=${TARGET_NAME}_report_llvm --ignore-filename-regex='3rd.*' -format="html"
            DEPENDS ${TARGET_NAME}-ccov-preprocessing)
    endif()
endif(FREELIBCXX_TEST)
