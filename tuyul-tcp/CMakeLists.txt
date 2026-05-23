# ==============================================================================
# tuyul-tcp Component Build Configuration
# ==============================================================================

# Define internal source code implementation files
set(TCP_SOURCES
    src/tcp_server.cpp
    src/tcp_client.cpp
)

# Declare the component as an OBJECT Library for production build sequences
add_library(tuyul-tcp-obj OBJECT ${TCP_SOURCES})

target_include_directories(tuyul-tcp-obj
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(Threads REQUIRED)
target_link_libraries(tuyul-tcp-obj PRIVATE Threads::Threads)

# ==============================================================================
# Dedicated Test Binary Configuration (Compiles isolated from production objects)
# ==============================================================================
if(TARGET gtest_main)
    # Define test suite execution target binary
    # We feed the raw .cpp files directly here so the UNIT_TEST macro 
    # burns ONLY the test environment, leaving the production OBJECT library clean!
    add_executable(tuyul-tcp-test
        ${TCP_SOURCES}
        tests/test_tcp_server.cpp
        tests/test_tcp_client.cpp
    )

    # Inject the UNIT_TEST macro strictly and exclusively to the test executable binary
    target_compile_definitions(tuyul-tcp-test PRIVATE UNIT_TEST)

    target_link_libraries(tuyul-tcp-test 
        PRIVATE 
            gtest_main 
            Threads::Threads
    )
    
    target_include_directories(tuyul-tcp-test 
        PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    add_test(NAME tcp_core_unit_test COMMAND tuyul-tcp-test)
endif()
