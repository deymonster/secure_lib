#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <curl/curl.h>
#include "vin_utils.hpp"
#include "security.hpp"
#include "ssh_manager.hpp"
#include "obfuscation.hpp"

// Certificates
const char* CA_PEM = R"(-----BEGIN CERTIFICATE-----
MIIDDzCCAfegAwIBAgIUJ9KcrGaf3rUB0vO4ZbSxvOznSbYwDQYJKoZIhvcNAQEL
BQAwFzEVMBMGA1UEAwwMVEJveCBSb290IENBMB4XDTI1MTIxODExNTkzMVoXDTM1
MTIxNjExNTkzMVowFzEVMBMGA1UEAwwMVEJveCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvJomoBUrGjPkdlvWBg3ZMic+/X3l+ptZ5aR0
93SGgkyIT7i+bHJCBD3CfG9AcGC9ZM94VhqCVjn4ZDNc1Avvwp+09ixFuXDjrN3r
J5yHgbtggxV8d4aYVPNifDqwFQaf3yRW/eMAP0KjBO+lchqb9pTM9oHFpTnFmWUQ
UlpJ7G5cq91yJuE2h60cANO0yAbIt9w+Lyr5/Qd8yK8vqHCunjU85mnbkB2/tB2X
UFUjkuz8bLOgyQpuwAoVLcemaGOmY94cBqVqLUxXPafxHy6TRFS4misVdmGJDwqb
UN3NJV9MdPT+n+xJwiGPLmCs60Gwfe2PC8LczCAgBZGM+1hnrwIDAQABo1MwUTAd
BgNVHQ4EFgQUdJK4jTjqFwP7kt0hp6JwfFoD7T0wHwYDVR0jBBgwFoAUdJK4jTjq
FwP7kt0hp6JwfFoD7T0wDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOC
AQEAkfHVikRJvsYD0HMbg+4n/9dLewkIxRHfE5JI0YBTJyTOiO/N652zBWziFLd5
bXfSrfnvAJDRssi7D9jyXqKjvs91u7ylOnqShmcavQ7W0pYPaEM+wN086A/i9sN0
gbqriFY+Ui4Mt+Os41TFKQEMdhnlNxwcVOwJb5JX1ZNoT4lEip/iUNBfgAoDLV1a
r1zs5h06rvw0XT+12ktIHaE8338fPV5dlJcC49A5/Uh8R9U99EBs78oRf6C/yv0O
U8lnhYbIY4YEDhQ1HZZ21puvfxlohN9aNnqEZD4Ffd+Ceba1WGE/jfWd1Mw1gVWM
8DKnfk4vgAdX9WadBhChO3CtZg==
-----END CERTIFICATE-----)";

const char* CLIENT_CERT_PEM = R"(-----BEGIN CERTIFICATE-----
MIIC6jCCAdKgAwIBAgIBAjANBgkqhkiG9w0BAQsFADAXMRUwEwYDVQQDDAxUQm94
IFJvb3QgQ0EwHhcNMjUxMjE4MTE1OTMyWhcNMjYxMjE4MTE1OTMyWjAWMRQwEgYD
VQQDDAtUQm94IENsaWVudDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
ANZtR0wYhqZpFyrDXNKx5GifaQtiQGF8BMTttyMujpppyDL4DSjyz2Mb5e09ZH8Z
4Esz5UEZJcGb0HjzJ8BZvFjRGmRKrBYElnrNDsnj36PUTkbDAcCanx3PKj8kyzzJ
GKpu1dgo+MYBhFrI9CGoNWQCyAvFvUl6qcBKnBtvU/AtvGrALcWqiXcjaVsXsW4a
SZ5Z7kJI9RYACVULuvf2h72OM0Ip9bdGYJuz0Y4hsr4SE6fsSAlLWyUzt9QIrUsO
+O7uNX1lz66/kT43+fAnt+xYeNmK+FOJBYd9bJBWHHkJPAD8kzCwHtfRlnANhUFL
dcVCNfJuRwXAogWHXsW8+zcCAwEAAaNCMEAwHQYDVR0OBBYEFDfRmYtN7DcM6DSK
L36oyChgS8exMB8GA1UdIwQYMBaAFHSSuI046hcD+5LdIaeicHxaA+09MA0GCSqG
SIb3DQEBCwUAA4IBAQA2hsdlEhgZnf0YU9jair2Y0i0IY+pN05eQRqgdMLWGFmpc
wlyYvv1XsKqx0MozIIjObXnqjV8S5F87anoup5ziaeCrxDhbbOzJcFFaElf+/6j2
xR1PQQoh3GJPI9e7BVct6KtoMhMQdMmGLuxvEuyoGaGe8d7D5yCW8eIJXJI3e389
vPWJ82OvybXFRe1naRgRE7nRRTtc7OTc0lobgzZ8P2u6AQRe6nxE9B8aVWwVVbtG
mVb2xmJKm80HUI9PGsWWw8c6/3YKbR+dxsTIbG0dDHyWMyhFUCg1rVwePquW3VS5
8ZFQZ71Xjvp8TmUzUDunjASDY/19qreRMRy/U7WD
-----END CERTIFICATE-----)";

const char* CLIENT_KEY_PEM = R"(REMOVED_FOR_SECURITY
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDWbUdMGIamaRcq
w1zSseRon2kLYkBhfATE7bcjLo6aacgy+A0o8s9jG+XtPWR/GeBLM+VBGSXBm9B4
8yfAWbxY0RpkSqwWBJZ6zQ7J49+j1E5GwwHAmp8dzyo/JMs8yRiqbtXYKPjGAYRa
yPQhqDVkAsgLxb1JeqnASpwbb1PwLbxqwC3Fqol3I2lbF7FuGkmeWe5CSPUWAAlV
C7r39oe9jjNCKfW3RmCbs9GOIbK+EhOn7EgJS1slM7fUCK1LDvju7jV9Zc+uv5E+
N/nwJ7fsWHjZivhTiQWHfWyQVhx5CTwA/JMwsB7X0ZZwDYVBS3XFQjXybkcFwKIF
h17FvPs3AgMBAAECggEAZTuFUA86LsFpnxZJse0rXRt5b9bsdzmiVMi30APQbmUn
j6ydJepwb6WBqT31Pq5cPpTbFJ9HPH8P7rI5X63d5n6d+6BnWyPCWWqLI1SSRlna
tLynDKPTIDMoilQYIXP9UaVvDxTU0kJwFp5N57/uqO9JO4mZAmJHVxHCXo9yLO7E
wxu2dWc8scIeJEkln6hqr6DTc6dZNMWxWyk4mLgvZtIxtcpfamRMgqcmCouENI8d
9e6ry/076H+SRXCEPJtLlXo6y7KXQioKV1Y5Y/UN9ZSI728Dkr/hLczaY9zw22m4
21tXjIwuy2KYxltjwD0k7gr8m/eQ87O0jStNC1za/QKBgQD+/M+l2sONNAWiDhlA
FS6MOd4NeXEaHT1TSMaCzJtXraKEa2ZjPPUmZ1fgW9RFK7Da5O0MA56DWnK2aav7
hacD4lZvRztvKMRFA9o9g7Xg/n29Zm95IxZB38MprIU9nkrS/4wCkpsTyhHkluq6
Cs5LEQc3BJOUNsaPeBQQtzH04wKBgQDXRz0IAW0uAVLOm4HTUyuNzOe2M+4s7JUJ
st+4hgg8fmXPbhw63OD/CbbDf3XtSayH8FZOvIuvmoEsh+oLh62UtZGtoVr9sGEF
kLzBmxGeUEPIQPrp+pf0zQnmi/f72PsUrlnh9rX11rg+IiCWgKt7o2GZzRvT1wrq
h0NBn7TEnQKBgQDzwUXXrSunfoWsB4JH6nfXATKu+tsONcl8JmPugh42UPy9TdZR
I+LO1ZgCGIbxoPSuLI4XIBaWRw7GJnqMNZYVdndeZWABwZzuxOIKUDC8Z0xYlOYX
jV0nl/r/ibbN0taAol57zx6EanV0anj1ZIMU67BT/gH+e+aRHB9CY35UNQKBgDao
CI/CxPR8M4jvJGwB3rn2vxGcZ6kSO9iliHVx2h95u+GJRDORprI5xiQmdUtUfDBb
TZ5Z8mEYKhmjPEHHJcPuwVjC8bYdFNTz5WiGNVfravopvUqwa+okMJJPERvo//5F
Md7T42jSJh7oTTvI2UDfv0TsNVd4bnYS93lu0ddlAoGBANshypl4+mRojWIZWANE
E2l/8vMI8+7ophMtBcVqR8VOfPpF8vMXiM4mIuqJTZGyWyGmFOgTij1znY3Rr/wi
ckijMCdiKU+t7w2K735opSBg+Cd6PmKhhGeH+0Ue9zfWkUbTpWciHmYylzoEtQoC
vg/lHVuMnljQaTl3OJkM0K6Q
-----END PRIVATE KEY-----)";

// Encrypted Strings (Key: 0xAA)
// https://
std::vector<unsigned char> url_prefix_enc = {0xC2, 0xDE, 0xDE, 0xDA, 0xD9, 0x90, 0x85, 0x85};
// :8443/api/register_device
std::vector<unsigned char> url_suffix_enc = {0x90, 0x92, 0x9E, 0x9E, 0x99, 0x85, 0xCB, 0xDA, 0xC3, 0x85, 0xD8, 0xCF, 0xCD, 0xC3, 0xD9, 0xDE, 0xCF, 0xD8, 0xF5, 0xCE, 0xCF, 0xDC, 0xC3, 0xC9, 0xCF};
// {"vin": "
std::vector<unsigned char> json_prefix_enc = {0xD1, 0x88, 0xDC, 0xC3, 0xC4, 0x88, 0x90, 0x8A, 0x88};
// "}
std::vector<unsigned char> json_suffix_enc = {0x88, 0xD7};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string sendVinToServer(const std::string& vin, const std::string& host) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // Decrypt strings
    std::string url_prefix = tbox::Obfuscator::decrypt(url_prefix_enc, 0xAA);
    // std::string url_suffix = tbox::Obfuscator::decrypt(url_suffix_enc, 0xAA);
    std::string url_suffix = ":8443/graphql"; // GraphQL Endpoint

    // std::string json_prefix = tbox::Obfuscator::decrypt(json_prefix_enc, 0xAA);
    // std::string json_suffix = tbox::Obfuscator::decrypt(json_suffix_enc, 0xAA);

    // Construct URL: https://<HOST>:8443/graphql
    std::string url = url_prefix + host + url_suffix;
    
    // Construct JSON: {"query": "mutation { registerDevice(vin: \"<VIN>\") { success message config { serverIp proxyPort subscriptionActive } } }"}
    std::string jsonPayload = "{\"query\": \"mutation { registerDevice(vin: \\\"" + vin + "\\\") { success message config { serverIp proxyPort subscriptionActive } } }\"}";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // SSL Configuration (In-Memory)
        struct curl_blob ca_blob;
        ca_blob.data = (void*)CA_PEM;
        ca_blob.len = strlen(CA_PEM);
        ca_blob.flags = CURL_BLOB_COPY;
        curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &ca_blob);

        struct curl_blob cert_blob;
        cert_blob.data = (void*)CLIENT_CERT_PEM;
        cert_blob.len = strlen(CLIENT_CERT_PEM);
        cert_blob.flags = CURL_BLOB_COPY;
        curl_easy_setopt(curl, CURLOPT_SSLCERT_BLOB, &cert_blob);
        curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");

        struct curl_blob key_blob;
        key_blob.data = (void*)CLIENT_KEY_PEM;
        key_blob.len = strlen(CLIENT_KEY_PEM);
        key_blob.flags = CURL_BLOB_COPY;
        curl_easy_setopt(curl, CURLOPT_SSLKEY_BLOB, &key_blob);
        curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "PEM");
        
        // Disable host verification for local test if needed
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); 
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            readBuffer = "ERROR: " + std::string(curl_easy_strerror(res));
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}

// Helper to simulate pre-encrypted data (since we are generating code)
// In production, these should be static const std::vector<unsigned char>
std::vector<unsigned char> encrypt_runtime(const std::string& input, unsigned char key) {
    std::vector<unsigned char> output;
    output.reserve(input.size());
    for (char c : input) {
        output.push_back(static_cast<unsigned char>(c) ^ key);
    }
    return output;
}

// Function to perform T-Box setup
void performTBoxSetup(const std::string& serverIp, const std::string& proxyPort) {
    std::cout << "\n[*] Initiating T-Box Setup Sequence..." << std::endl;

    // SSH Credentials (should be secured/encrypted in production)
    const std::string SSH_HOST = "192.168.225.1";
    const int SSH_PORT = 22;
    const std::string SSH_USER = "root";
    const std::string SSH_PASS = "oelinux123";
    const unsigned char CRYPT_KEY = 0x55;

    tbox::SshManager ssh;
    std::cout << "[*] Connecting to T-Box (" << SSH_HOST << ")..." << std::endl;
    
    if (ssh.connect(SSH_HOST, SSH_PORT, SSH_USER, SSH_PASS)) {
        std::cout << "[+] SSH Connected successfully." << std::endl;

        // Prepare commands
        // Note: In a real scenario, 'cmd_templates' would be replaced by 
        // static const vectors containing pre-encrypted bytes.
        
        // 1. Configure Firewall (Dynamic IP/Port)
        std::string cmd1_raw = "iptables -t nat -A OUTPUT -p tcp -d 81.70.125.38 --dport 8111 -j DNAT --to-destination " + serverIp + ":" + proxyPort;
        
        // 2. Verify Firewall
        std::string cmd2_raw = "iptables -t nat -L OUTPUT -n | grep 8111";
        
        // 3. Verify Server Availability
        std::string cmd3_raw = "curl -v -k https://gw-t5.beantechyun.com:8111";

        // Encrypt (Simulating secure storage)
        auto cmd1_enc = encrypt_runtime(cmd1_raw, CRYPT_KEY);
        auto cmd2_enc = encrypt_runtime(cmd2_raw, CRYPT_KEY);
        auto cmd3_enc = encrypt_runtime(cmd3_raw, CRYPT_KEY);

        // Execute Sequence
        auto execute_encrypted = [&](const std::vector<unsigned char>& enc_cmd, const std::string& label) {
            std::string cmd = tbox::Obfuscator::decrypt(enc_cmd, CRYPT_KEY);
            std::cout << "\n[EXEC] " << label << ":" << std::endl;
            std::cout << "CMD: " << cmd << std::endl;
            std::string result = ssh.executeCommand(cmd);
            std::cout << "OUTPUT:\n" << result << std::endl;
        };

        execute_encrypted(cmd1_enc, "Configure Firewall");
        execute_encrypted(cmd2_enc, "Verify Firewall Rules");
        execute_encrypted(cmd3_enc, "Verify Server Availability");

        ssh.disconnect();
        std::cout << "[*] SSH Disconnected." << std::endl;

    } else {
        std::cout << "[-] SSH Connection Failed! Check network/VPN/USB connection." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== Secure T-Box Library Test (mTLS ONLY) ===" << std::endl;

    std::string server_host = "192.168.1.100"; // Default placeholder
    if (argc > 1) {
        server_host = argv[1];
        std::cout << "[*] Using Server IP: " << server_host << std::endl;
    } else {
        std::cout << "[!] No IP provided. Using default: " << server_host << std::endl;
        std::cout << "Usage: ./secure_test_app <SERVER_IP>" << std::endl;
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

    // 2. Используем хардкодный VIN для теста
    std::string vin = "LGWEF5A61NH517926";
    std::cout << "\n[*] Using Test VIN: " << vin << std::endl;

    // 3. Тест mTLS
    std::cout << "\n[*] Testing mTLS Connection to " << server_host << "..." << std::endl;
    
    std::string response = sendVinToServer(vin, server_host);
    std::cout << "Server Response: " << response << std::endl;

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
        std::cout << "    Server IP: " << serverIp << std::endl;
        std::cout << "    Proxy Port: " << proxyPortStr << std::endl;
        
        // Execute SSH operations
        performTBoxSetup(serverIp, proxyPortStr);
        
    } else {
        std::cout << "[-] Subscription: INACTIVE or Error" << std::endl;
    }

    return 0;
}