#include "vin_utils.hpp"
#include <vector>
#include <cstring>
#include "obfuscation.hpp"

// Linter workaround for non-Android environments
#if defined(__ANDROID__) || defined(__has_include) && __has_include(<sys/system_properties.h>)
    #include <sys/system_properties.h>
#else
    #define PROP_VALUE_MAX 92
    // Mock function for linter/non-Android build
    static int __system_property_get(const char* key, char* value) {
        return 0;
    }
#endif

namespace tbox {

    std::string VinReader::getSystemProperty(const std::string& key) {
        char value[PROP_VALUE_MAX] = {0};
        int len = __system_property_get(key.c_str(), value);
        if (len > 0) {
            return std::string(value);
        }
        return "";
    }

    std::string VinReader::getVin() {
        // Property: persist.beantechs.vehicle.vin
        // Encrypted with 0xAA
        static const unsigned char prop_enc_arr[] = {
            218, 207, 216, 217, 195, 217, 222, 132, 
            200, 207, 203, 196, 222, 207, 201, 194, 217, 132, 
            220, 207, 194, 195, 201, 198, 207, 132, 
            220, 195, 196
        };
        std::vector<unsigned char> prop_enc(prop_enc_arr, prop_enc_arr + sizeof(prop_enc_arr));

        std::string prop_name = tbox::Obfuscator::decrypt(prop_enc, 0xAA);
        
        std::string val = getSystemProperty(prop_name);
        if (!val.empty() && val != "unknown") {
            return val;
        }
        
        return "UNKNOWN_VIN";
    }
}