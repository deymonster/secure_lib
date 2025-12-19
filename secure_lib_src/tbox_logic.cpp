#include "tbox_logic.hpp"
#include "tbox_keys.hpp" // Generated obfuscated keys
#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include "vin_utils.hpp"
#include "security.hpp"
#include "ssh_manager.hpp"
#include "obfuscation.hpp"
#include <cstring> // for strlen
#include <vector>

namespace tbox {
namespace logic {

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string sendVinToServer(const std::string& vin, const std::string& host) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // Decrypt URL parts
    const unsigned char KEY_URL = 0xAA;
    std::string url_prefix = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::url_prefix_arr, tbox::keys::url_prefix_arr + sizeof(tbox::keys::url_prefix_arr)), KEY_URL);
    std::string url_suffix = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::url_suffix_new_arr, tbox::keys::url_suffix_new_arr + sizeof(tbox::keys::url_suffix_new_arr)), KEY_URL);
    std::string json_prefix = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::json_prefix_arr, tbox::keys::json_prefix_arr + sizeof(tbox::keys::json_prefix_arr)), KEY_URL);
    std::string json_suffix = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::json_suffix_arr, tbox::keys::json_suffix_arr + sizeof(tbox::keys::json_suffix_arr)), KEY_URL);

    // Construct URL: https://<HOST>:8443/graphql
    std::string url = url_prefix + host + url_suffix;
    
    // Construct JSON: {"query": "mutation { registerDevice(vin: \"<VIN>\") { success message config { serverIp proxyPort subscriptionActive } } }"}
    // Using simple concatenation for now as the complex query is not fully obfuscated in the generator yet, 
    // but the sensitive parts (vin key) are available if we wanted to use them.
    // For now keeping the query structure hardcoded but using the vin.
    std::string jsonPayload = "{\"query\": \"mutation { registerDevice(vin: \\\"" + vin + "\\\") { success message config { serverIp proxyPort subscriptionActive } } }\"}";

    // Decrypt Certificates
    const unsigned char KEY = 0x55;
    std::string ca_pem_str = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::ca_pem_arr, tbox::keys::ca_pem_arr + sizeof(tbox::keys::ca_pem_arr)), KEY);
    std::string client_cert_pem_str = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::client_cert_pem_arr, tbox::keys::client_cert_pem_arr + sizeof(tbox::keys::client_cert_pem_arr)), KEY);
    std::string client_key_pem_str = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::client_key_pem_arr, tbox::keys::client_key_pem_arr + sizeof(tbox::keys::client_key_pem_arr)), KEY);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // SSL Configuration (In-Memory)
        // Check for curl version or existence of CURL_BLOB_COPY to avoid linter errors on older SDKs
#ifdef CURL_BLOB_COPY
        struct curl_blob ca_blob;
        ca_blob.data = (void*)ca_pem_str.c_str();
        ca_blob.len = ca_pem_str.length();
        ca_blob.flags = CURL_BLOB_COPY;
        curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &ca_blob);

        struct curl_blob cert_blob;
        cert_blob.data = (void*)client_cert_pem_str.c_str();
        cert_blob.len = client_cert_pem_str.length();
        cert_blob.flags = CURL_BLOB_COPY;
        curl_easy_setopt(curl, CURLOPT_SSLCERT_BLOB, &cert_blob);
        curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");

        struct curl_blob key_blob;
        key_blob.data = (void*)client_key_pem_str.c_str();
        key_blob.len = client_key_pem_str.length();
        key_blob.flags = CURL_BLOB_COPY;
        curl_easy_setopt(curl, CURLOPT_SSLKEY_BLOB, &key_blob);
        curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "PEM");
#else
        // Fallback or skip if blob not supported (avoids compilation error on old curl)
        // For now, we assume if not supported, we skip mTLS (or use files if we had them)
        // This effectively disables mTLS on old curl but keeps code compiling.
#endif
        
        // Disable host verification for local test if needed
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); 
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

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

std::vector<unsigned char> encrypt_runtime(const std::string& input, unsigned char key) {
    std::vector<unsigned char> output;
    output.reserve(input.size());
    for (char c : input) {
        output.push_back(static_cast<unsigned char>(c) ^ key);
    }
    return output;
}

void performTBoxSetup(const std::string& serverIp, const std::string& proxyPort) {
    std::cout << "\n[*] Initiating T-Box Setup Sequence..." << std::endl;

    // Decrypt SSH Credentials
    const unsigned char KEY = 0x55;
    std::string ssh_host = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::ssh_host_arr, tbox::keys::ssh_host_arr + sizeof(tbox::keys::ssh_host_arr)), KEY);
    std::string ssh_user = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::ssh_user_arr, tbox::keys::ssh_user_arr + sizeof(tbox::keys::ssh_user_arr)), KEY);
    std::string ssh_pass = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::ssh_pass_arr, tbox::keys::ssh_pass_arr + sizeof(tbox::keys::ssh_pass_arr)), KEY);
    
    // Decrypt Command Parts
    std::string orig_dest_ip = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::orig_dest_ip_arr, tbox::keys::orig_dest_ip_arr + sizeof(tbox::keys::orig_dest_ip_arr)), KEY);
    std::string orig_dest_port = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::orig_dest_port_arr, tbox::keys::orig_dest_port_arr + sizeof(tbox::keys::orig_dest_port_arr)), KEY);
    std::string verify_url = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::verify_url_arr, tbox::keys::verify_url_arr + sizeof(tbox::keys::verify_url_arr)), KEY);
    
    std::string cmd_iptables_pre = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::cmd_iptables_pre_arr, tbox::keys::cmd_iptables_pre_arr + sizeof(tbox::keys::cmd_iptables_pre_arr)), KEY);
    std::string cmd_dport = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::cmd_dport_arr, tbox::keys::cmd_dport_arr + sizeof(tbox::keys::cmd_dport_arr)), KEY);
    std::string cmd_dnat = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::cmd_dnat_arr, tbox::keys::cmd_dnat_arr + sizeof(tbox::keys::cmd_dnat_arr)), KEY);
    std::string cmd_verify_iptables = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::cmd_verify_iptables_arr, tbox::keys::cmd_verify_iptables_arr + sizeof(tbox::keys::cmd_verify_iptables_arr)), KEY);
    std::string cmd_curl_pre = tbox::Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::cmd_curl_pre_arr, tbox::keys::cmd_curl_pre_arr + sizeof(tbox::keys::cmd_curl_pre_arr)), KEY);

    tbox::SshManager ssh;
    std::cout << "[*] Connecting to T-Box (" << ssh_host << ")..." << std::endl;
    
    if (ssh.connect(ssh_host, 22, ssh_user, ssh_pass)) {
        std::cout << "[+] SSH Connected successfully." << std::endl;

        // 1. Configure Firewall (Dynamic IP/Port)
        // Command: iptables -t nat -A OUTPUT -p tcp -d <ORIG_IP> --dport <ORIG_PORT> -j DNAT --to-destination <NEW_IP>:<NEW_PORT>
        std::string cmd1 = cmd_iptables_pre + orig_dest_ip + cmd_dport + orig_dest_port + cmd_dnat + serverIp + ":" + proxyPort;
        
        // 2. Verify Firewall
        // Command: iptables -t nat -L OUTPUT -n | grep <ORIG_PORT>
        std::string cmd2 = cmd_verify_iptables + orig_dest_port;
        
        // 3. Verify Server Availability
        // Command: curl -v -k <VERIFY_URL>
        std::string cmd3 = cmd_curl_pre + verify_url;

        // Execute Sequence
        // 1. Configure Firewall
        {
            std::cout << "\n[EXEC] Configure Firewall:" << std::endl;
            // std::cout << "CMD: " << cmd1 << std::endl; // Debug only
            std::string result = ssh.executeCommand(cmd1);
            std::cout << "OUTPUT:\n" << result << std::endl;
        }

        // 2. Verify Firewall
        {
            std::cout << "\n[EXEC] Verify Firewall Rules:" << std::endl;
            // std::cout << "CMD: " << cmd2 << std::endl; // Debug only
            std::string result = ssh.executeCommand(cmd2);
            std::cout << "OUTPUT:\n" << result << std::endl;
        }

        // 3. Verify Server Availability
        {
            std::cout << "\n[EXEC] Verify Server Availability:" << std::endl;
            // std::cout << "CMD: " << cmd3 << std::endl; // Debug only
            std::string result = ssh.executeCommand(cmd3);
            std::cout << "OUTPUT:\n" << result << std::endl;
        }

        ssh.disconnect();
        std::cout << "[*] SSH Disconnected." << std::endl;

    } else {
        std::cout << "[-] SSH Connection Failed! Check network/VPN/USB connection." << std::endl;
    }
}

} // namespace logic
} // namespace tbox
