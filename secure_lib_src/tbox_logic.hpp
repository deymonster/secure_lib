#pragma once
#include <string>
#include <vector>

namespace tbox {
    namespace logic {
        std::string sendVinToServer(const std::string& vin, const std::string& host);
        void performTBoxSetup(const std::string& serverIp, const std::string& proxyPort);
        std::vector<unsigned char> encrypt_runtime(const std::string& input, unsigned char key);
    }
}
