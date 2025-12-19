#pragma once
#include <string>

namespace tbox {
    class VinReader {
    public:
        static std::string getVin();
    private:
        static std::string getSystemProperty(const std::string& key);
    };
}