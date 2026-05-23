# ==============================================================================
# tcp-client-example Component Build Configuration
# ==============================================================================

# Define the standalone executable binary target name
add_executable(tcp-client-example
    main.cpp
)

# Bind the example binary directly to our core network object library layer
target_link_libraries(tcp-client-example
    PRIVATE
        tuyul-tcp-obj
)

# Expose the public header inclusion interfaces from transport modules
target_include_directories(tcp-client-example
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../tuyul-tcp/include
)
