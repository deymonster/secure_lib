#include <iostream>
#include <string>
#include "tbox_logic.hpp"
#include "security.hpp"
#include "vin_utils.hpp" // Added to test real VIN reading

#include <thread>
#include <chrono>
#include "tbox_logic.hpp"
#include "tbox_callback.hpp"

// Консольный колбэк для тестов
class ConsoleCallback : public tbox::ITBoxCallback {
public:
    void onServerStatusChanged(bool isConnected) override {
        std::cout << "[TEST UI] Server Status Changed: " << (isConnected ? "ONLINE" : "OFFLINE") << std::endl;
    }

    void onConfigReceived(const std::string& vin, bool subActive) override {
        std::cout << "[TEST UI] Config Received. VIN: " << vin << ", Subscription: " << (subActive ? "ACTIVE" : "EXPIRED") << std::endl;
    }

    void onLog(const std::string& message) override {
        std::cout << "[LOG] " << message << std::endl;
    }
};

int main() {
    std::cout << "Starting Secure TBox Test Environment..." << std::endl;

    ConsoleCallback callback;
    
    // Запускаем сервис
    tbox::logic::startTBoxService(&callback);

    std::cout << "Service is running in background. Waiting for events..." << std::endl;
    std::cout << "You should run the python mock server now if you want to test connectivity." << std::endl;

    // Ждем 60 секунд или пока пользователь не нажмет Ctrl+C
    for(int i=0; i<60; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (i % 10 == 0) std::cout << "Test running... " << i << "s" << std::endl;
    }

    std::cout << "Stopping Service..." << std::endl;
    tbox::logic::stopTBoxService();
    std::cout << "Test Finished." << std::endl;

    return 0;
}