#include <iostream>
#include <string>
#include "tbox_logic.hpp"
#include "security.hpp"
#include "vin_utils.hpp" // Added to test real VIN reading

int main(int argc, char* argv[]) {
    std::cout << "=== Secure T-Box Library Test (Offline/Mock Mode) ===" << std::endl;

    std::string server_host = "192.168.1.100"; // Default placeholder
    if (argc > 1) {
        server_host = argv[1];
        std::cout << "[*] Using Server IP (Target for Tunnel): " << server_host << std::endl;
    } else {
        // Default target IP (gw-t5.beantechyun.com resolved or similar)
        server_host = "81.70.125.38"; 
        std::cout << "[!] No IP provided. Using default target: " << server_host << std::endl;
    }

    // 1. Проверка Security (оставляем для порядка)
    std::cout << "[*] Checking Security..." << std::endl;
    tbox::Security security;
    
    if (security.checkDebugger()) {
        std::cout << "[!] Debugger detected via TracerPid!" << std::endl;
    } 

    if (security.checkFrida()) {
        std::cout << "[!] Frida/Gadget detected in memory maps!" << std::endl;
    }

    // 2. Тест чтения VIN (Реальный)
    std::cout << "\n[*] Reading System VIN..." << std::endl;
    std::string vin = tbox::VinReader::getVin();
    std::cout << "    Detected VIN: " << vin << std::endl;

    // 3. Тест mTLS (REAL - Проверка соединения)
    std::cout << "\n[*] Testing mTLS Connection (REAL)..." << std::endl;
    // std::cout << "[MOCK] Skipping real network call. Simulating SUCCESS response." << std::endl;
    
    // Используем "localhost:8443" если запускаем на эмуляторе с пробросом, 
    // или IP компьютера если на реальном девайсе.
    // Функция sendVinToServer сама добавит префикс https:// и суффикс :8443/graphql (из обфусцированных данных)
    // Но подождите, в tbox_logic.cpp:
    // std::string url = url_prefix + host + url_suffix;
    // url_prefix = "https://"
    // url_suffix = ":8443/graphql"
    // Значит host должен быть просто IP или домен.
    
    std::string response = tbox::logic::sendVinToServer(vin, server_host);
    std::cout << "    Server Response: " << response << std::endl;
    
    /*
    // Создаем фейковый JSON ответ, который ожидает парсер
    // serverIp: куда пробрасывать трафик
    // proxyPort: порт на сервере (обычно 10022 и т.д., но для теста возьмем 8111 или любой)
    std::string mock_target_ip = server_host; 
    std::string mock_proxy_port = "8111";
    
    std::string response = "{"
        "\"data\": {"
            "\"registerDevice\": {"
                "\"success\": true,"
                "\"message\": \"Device registered\","
                "\"config\": {"
                    "\"serverIp\": \"" + mock_target_ip + "\","
                    "\"proxyPort\": " + mock_proxy_port + ","
                    "\"subscriptionActive\": true"
                "}"
            "}"
        "}"
    "}";
    */
    
    std::cout << "Server Response (Simulated): " << response << std::endl;

    // Simple parsing for test utility
    bool subscriptionActive = false;
    // Handle potential spaces in JSON
    size_t subPos = response.find("\"subscriptionActive\"");
    if (subPos != std::string::npos) {
        size_t colon = response.find(":", subPos);
        if (colon != std::string::npos) {
            size_t valStart = response.find_first_not_of(" \t\n\r", colon + 1);
            if (valStart != std::string::npos && response.substr(valStart, 4) == "true") {
                subscriptionActive = true;
            }
        }
    }

    std::string serverIp = server_host;
    std::string proxyPortStr = "10022"; // Default

    size_t ipPos = response.find("\"serverIp\"");
    if (ipPos != std::string::npos) {
        size_t colon = response.find(":", ipPos);
        size_t start = response.find("\"", colon + 1); // Start of value string
        if (start != std::string::npos) {
            size_t end = response.find("\"", start + 1);
            if (end != std::string::npos) {
                serverIp = response.substr(start + 1, end - start - 1);
            }
        }
    }

    size_t portPos = response.find("\"proxyPort\"");
    if (portPos != std::string::npos) {
        size_t colon = response.find(":", portPos);
        size_t start = response.find_first_of("0123456789", colon + 1);
        if (start != std::string::npos) {
            size_t end = response.find_first_not_of("0123456789", start);
            proxyPortStr = response.substr(start, end - start);
        }
    }

    if (subscriptionActive) {
        std::cout << "[+] Subscription: ACTIVE" << std::endl;
        std::cout << "    Server IP (Tunnel Target): " << serverIp << std::endl;
        std::cout << "    Proxy Port: " << proxyPortStr << std::endl;
        
        // Execute SSH operations
        // Это попытается подключиться к 192.168.225.1 (зашито в tbox_logic.cpp)
        // и выполнить команды iptables для проброса на serverIp:proxyPortStr
        tbox::logic::performTBoxSetup(serverIp, proxyPortStr);
        
    } else {
        std::cout << "[-] Subscription: INACTIVE or Error" << std::endl;
    }

    return 0;
}
