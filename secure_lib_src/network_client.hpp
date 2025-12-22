#pragma once

#include <string>
#include <vector>

namespace tbox {

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    // Отправка POST запроса с mTLS
    std::string postSecure(const std::string& url, const std::string& jsonPayload);
    
    // Проверка доступности хоста (Heartbeat)
    bool checkConnectivity(const std::string& host);

    // Проверка доступности IP/Port (Simple Connect)
    bool checkReachability(const std::string& ip, int port);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    void setupCertificates(void* curl_handle);
};

} // namespace tbox