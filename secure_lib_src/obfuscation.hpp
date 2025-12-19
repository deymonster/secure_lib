#pragma once

#include <string>
#include <vector>

namespace tbox {

    class Obfuscator {
    public:
        // Простой XOR дешифратор
        // data: массив зашифрованных байт
        // key: ключ шифрования (байт)
        static std::string decrypt(const std::vector<unsigned char>& data, unsigned char key) {
            std::string result;
            result.reserve(data.size());
            for (unsigned char c : data) {
                result.push_back(c ^ key);
            }
            return result;
        }
    };

}