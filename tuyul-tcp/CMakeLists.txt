# ==============================================================================
# tuyul-tcp Component Build Configuration
# ==============================================================================

# 1. Define internal source code implementation files
set(TCP_SOURCES
    src/tcp_server.cpp
    src/tcp_client.cpp
)

# 2. Declare the component as an OBJECT Library for optimized internal linking
add_library(tuyul-tcp-obj OBJECT ${TCP_SOURCES})

# 3. Configure public and private include paths for the object target
target_include_directories(tuyul-tcp-obj
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 4. Bind native system threading support required for epoll asynchronous worker loops
find_package(Threads REQUIRED)
target_link_libraries(tuyul-tcp-obj PRIVATE Threads::Threads)

# ==============================================================================
# Dedicated Test Binary Configuration (Compiles only if GoogleTest framework exists)
# ==============================================================================
if(TARGET gtest_main)
    # Define test suite execution target binary
    add_executable(tuyul-tcp-test
        tests/test_tcp_server.cpp
        tests/test_tcp_client.cpp
    )

    # Inject the UNIT_TEST macro into BOTH the test execution and the object library compiler flags
    target_compile_definitions(tuyul-tcp-test PRIVATE UNIT_TEST)
    target_compile_definitions(tuyul-tcp-obj PRIVATE UNIT_TEST)

    # Link execution requirements
    target_link_libraries(tuyul-tcp-test 
        PRIVATE 
            tuyul-tcp-obj 
            gtest_main 
            Threads::Threads
    )
    
    target_include_directories(tuyul-tcp-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    add_test(NAME tcp_core_unit_test COMMAND tuyul-tcp-test)
endif()
