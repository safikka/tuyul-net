#include <gtest/gtest.h>
#include <tuyul/tcp_server.hpp>
#include <thread>
#include <chrono>

/**
 * @brief Test fixture for localized testing of the TcpServer architecture.
 */
class TcpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialization routines if needed before each test execution
    }

    void TearDown() override {
        // Cleanup routines if needed after each test execution
    }
};

/**
 * @brief Verifies that the TcpServer class can be successfully instantiated.
 */
TEST_F(TcpServerTest, LifecycleObjectCreation) {
    // Port 9999 is picked randomly to ensure isolation from common standard system ports
    tuyul::TcpServer server(9999);
    
    // Explicitly verify that the object exists and hasn't crashed during allocation phase
    SUCCEED();
}

/**
 * @brief Asserts that the server can initialize its socket loop and shut down safely via RAII.
 */
TEST_F(TcpServerTest, StandardLifecycleBootAndShutdown) {
    // Use an alternative custom testing port footprint
    tuyul::TcpServer server(10001);
    
    bool logger_triggered = false;
    
    // Attach a local validation logger callback to trace operational milestones
    server.set_logger([&logger_triggered](std::string_view level, std::string_view message) {
        if (level == "INFO") {
            logger_triggered = true;
        }
    });

    // Execute the blocking event-loop inside a dedicated asynchronous background thread
    std::thread server_thread([&server]() {
        tuyul::ErrorCode status = server.start();
        EXPECT_EQ(status, tuyul::ErrorCode::SUCCESS);
    });

    // Give the Linux kernel a brief execution window to fully bind and spawn the epoll descriptors
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Forcefully request the server loop to cease operation and dismantle its descriptors
    server.stop();

    // Re-join the detached thread context back into the main testing pipeline
    if (server_thread.joinable()) {
        server_thread.join();
    }

    // Ensure the internal engine logged its startup lifecycle sequence safely
    EXPECT_TRUE(logger_triggered);
}
