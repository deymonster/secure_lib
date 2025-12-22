#include "tbox_logic.hpp"
#include "tbox_keys.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <regex>
#include "vin_utils.hpp"
#include "ssh_manager.hpp"
#include "obfuscation.hpp"
#include "network_client.hpp"
#include "XorStr.hpp" // Подключаем шифрование строк

// !!! UNCOMMENT TO ENABLE MOCK MODE FOR TESTING ON PHONE !!!
// #define MOCK_TBOX_LOGIC

namespace tbox {
namespace logic {

static std::atomic<bool> g_running(false);
static std::thread g_serviceThread;
static ITBoxCallback* g_callback = nullptr; // Указатель на callback

// Global Context
static NetworkClient g_netClient;
static SshManager g_sshManager;
static std::string g_vin;
static std::string g_full_url;
static std::string g_connectivity_host;
static std::string g_ssh_host_decrypted;
static std::string g_ssh_user_decrypted;
static std::string g_ssh_pass_decrypted;

struct ServerConfig {
    std::string serverIp;
    std::string proxyPort;
    bool subActive = false;
    bool isValid = false;
};
static ServerConfig g_lastConfig;

// Макрос для удобного логирования через callback
// Если определен STRIP_LOGS, логи удаляются из кода
#ifdef STRIP_LOGS
    #define LOG_CB(msg) ((void)0)
#else
    #define LOG_CB(msg) { std::string _m = (msg); if(g_callback) g_callback->onLog(_m); std::cout << _m << std::endl; }
#endif

std::string extractJsonValue(const std::string& json, const std::string& key) {
    // Регулярки можно не шифровать, они сложны для чтения
    std::string pattern = "\"" + key + "\":\\s*\"?([^\"},]+)\"?";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re) && match.size() > 1) {
        return match.str(1);
    }
    return "";
}

// Парсинг булевых значений (true/false в JSON без кавычек)
bool extractJsonBool(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":\\s*(true|false)";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re) && match.size() > 1) {
        return (match.str(1) == "true");
    }
    return false;
}

void initGlobals() {
    static bool initialized = false;
    if (initialized) return;

    VinReader vinReader;
    g_vin = vinReader.getVin();
    
    // Fallback for VIN
    if (g_vin.empty()) g_vin = XORS("UNKNOWN_VIN");

    // Расшифровка URL префиксов
    const unsigned char KEY_URL = 0xAA;
    std::string url_prefix = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::url_prefix_arr, tbox::keys::url_prefix_arr + sizeof(tbox::keys::url_prefix_arr)), KEY_URL);
    std::string url_suffix = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::url_suffix_new_arr, tbox::keys::url_suffix_new_arr + sizeof(tbox::keys::url_suffix_new_arr)), KEY_URL);
    
    // IP: 8.138.224.189 (XOR 0xAA)
    std::string target_ip = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::target_ip_arr, tbox::keys::target_ip_arr + sizeof(tbox::keys::target_ip_arr)), KEY_URL);
    
    // Но для g_connectivity_host нам НУЖЕН порт, так как checkConnectivity просто добавляет https://
    // и если порта нет, он пойдет на 443.
    std::string host = target_ip + XORS(":8443");
    
    // Для проверки доступности
    g_connectivity_host = host;

    g_full_url = url_prefix + target_ip + url_suffix; 
    
    // SSH данные
    const unsigned char KEY = 0x55;
    g_ssh_host_decrypted = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::ssh_host_arr, tbox::keys::ssh_host_arr + sizeof(tbox::keys::ssh_host_arr)), KEY);
    g_ssh_user_decrypted = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::ssh_user_arr, tbox::keys::ssh_user_arr + sizeof(tbox::keys::ssh_user_arr)), KEY);
    g_ssh_pass_decrypted = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::ssh_pass_arr, tbox::keys::ssh_pass_arr + sizeof(tbox::keys::ssh_pass_arr)), KEY);

    initialized = true;
}

void executeGetConfig() {
    if (g_vin.empty() || g_vin == "UNKNOWN") {
         // Попробуем прочитать еще раз, если не инициализировано
         initGlobals();
    }
    
    LOG_CB(XORS("[TBox] Requesting Config for VIN: ") + g_vin);
    LOG_CB(XORS("[TBox] Target URL: ") + g_full_url);

    // Шифруем JSON Payload
    std::string jsonPayload = XORS("{\"query\": \"mutation { registerDevice(vin: \\\"") + g_vin + XORS("\\\") { success message config { serverIp proxyPort subscriptionActive } } }\"}");
    
    // Выполняем POST запрос
    std::string response = g_netClient.postSecure(g_full_url, jsonPayload);
    
    if (!response.empty()) {
        LOG_CB(XORS("[TBox] Response: ") + response);

        g_lastConfig.proxyPort = extractJsonValue(response, XORS("proxyPort"));
        g_lastConfig.serverIp = extractJsonValue(response, XORS("serverIp"));
        g_lastConfig.subActive = extractJsonBool(response, XORS("subscriptionActive"));
        g_lastConfig.isValid = true;

        LOG_CB(XORS("[TBox] Config Received: IP=") + g_lastConfig.serverIp + XORS(", Port=") + g_lastConfig.proxyPort + XORS(", Active=") + (g_lastConfig.subActive ? "true" : "false"));

        // Уведомляем UI
        if (g_callback) g_callback->onConfigReceived(g_vin, g_lastConfig.subActive);
    } else {
        LOG_CB(XORS("[TBox] Failed to get config from server"));
    }
}

void executeCheckTBoxStatus() {
    initGlobals();
    
    // Key calculated: '1' (0x31) ^ 0x9B = 0xAA
    const unsigned char KEY_TBOX = 0xAA;
    std::string tbox_ip = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::tbox_check_ip_arr, tbox::keys::tbox_check_ip_arr + sizeof(tbox::keys::tbox_check_ip_arr)), KEY_TBOX);
    
    // Check T-Box availability
    // Try port 80 (HTTP) or 22 (SSH)
    bool isTboxReachable;
#ifdef MOCK_TBOX_LOGIC
    LOG_CB(XORS("[MOCK] Real T-Box check skipped. Simulating SUCCESS."));
    isTboxReachable = true;
#else
    isTboxReachable = g_netClient.checkReachability(tbox_ip, 80) || 
                      g_netClient.checkReachability(tbox_ip, 22);
#endif
                           
    if (g_callback) {
        g_callback->onTboxStatusChanged(isTboxReachable);
    }
    LOG_CB(isTboxReachable ? XORS("[TBox] T-Box is reachable") : XORS("[TBox] T-Box is NOT reachable"));
}

void executeApplyConfig() {
    if (!g_lastConfig.isValid) {
        LOG_CB(XORS("[TBox] Cannot apply config: No valid config available. Please 'Get Config' first."));
        return;
    }

    if (!g_lastConfig.subActive) {
        LOG_CB(XORS("[TBox] Cannot apply config: Subscription is NOT ACTIVE."));
        return;
    }

    
    // Key calculated: '1' (0x31) ^ 0x9B = 0xAA
    const unsigned char KEY_TBOX = 0xAA;
    std::string tbox_ip = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::tbox_check_ip_arr, tbox::keys::tbox_check_ip_arr + sizeof(tbox::keys::tbox_check_ip_arr)), KEY_TBOX);
    
    LOG_CB(XORS("[TBox] Checking T-Box availability at ") + tbox_ip + "...");
    
    // Check T-Box availability
    // Try port 80 (HTTP) or 22 (SSH)
    bool isTboxReachable;
#ifdef MOCK_TBOX_LOGIC
    LOG_CB(XORS("[MOCK] Real T-Box check skipped. Simulating SUCCESS."));
    isTboxReachable = true;
#else
    isTboxReachable = g_netClient.checkReachability(tbox_ip, 80) || 
                      g_netClient.checkReachability(tbox_ip, 22);
#endif
                           
    if (g_callback) {
        g_callback->onTboxStatusChanged(isTboxReachable);
    }
    
    if (!isTboxReachable) {
        LOG_CB(XORS("[TBox] T-Box (") + tbox_ip + XORS(") is NOT reachable. Ensure you are connected to the car's WiFi."));
        return;
    }

    LOG_CB(XORS("[TBox] T-Box is reachable. Applying Config via SSH..."));
    
    // Prepare commands - ШИФРУЕМ КОМАНДЫ
    std::string iptables_cmd = XORS("iptables -t nat -A PREROUTING -p tcp --dport 80 -j DNAT --to-destination ") + g_lastConfig.serverIp + ":" + g_lastConfig.proxyPort;
    std::string check_netstat_cmd = XORS("netstat -an | grep ") + g_lastConfig.proxyPort;
    // Проверка соединения через curl (с таймаутом 5 сек, игнорируем SSL ошибки)
    // Мы ожидаем, что запрос на gw-t5 уйдет на наш IP
    std::string check_curl_cmd = XORS("curl -v -k --max-time 5 https://gw-t5.beantechyun.com:8111");

    // SSH Connect
    bool ssh_connected;
#ifdef MOCK_TBOX_LOGIC
    LOG_CB(XORS("[MOCK] Real SSH Connection skipped. Simulating SUCCESS."));
    LOG_CB(XORS("[MOCK] Would connect to: ") + g_ssh_host_decrypted + " User: " + g_ssh_user_decrypted + " Pass: " + g_ssh_pass_decrypted);
    ssh_connected = true;
#else
    ssh_connected = g_sshManager.connect(g_ssh_host_decrypted, 22, g_ssh_user_decrypted, g_ssh_pass_decrypted);
#endif
    
    if (ssh_connected) {
        LOG_CB(XORS("[TBox] SSH Connected Successfully."));
        
        // 1. APPLY IPTABLES
        LOG_CB(XORS("[CMD] Executing: ") + iptables_cmd);
#ifdef MOCK_TBOX_LOGIC
        LOG_CB(XORS("[MOCK] Command execution simulated (success)"));
#else
        g_sshManager.executeCommand(iptables_cmd);
#endif
        
        // 2. CHECK NETSTAT
        LOG_CB(XORS("[CMD] Executing: ") + check_netstat_cmd);
        std::string netstat_output;
#ifdef MOCK_TBOX_LOGIC
        LOG_CB(XORS("[MOCK] Simulating netstat output..."));
        netstat_output = XORS("tcp        0      0 0.0.0.0:") + g_lastConfig.proxyPort + XORS("         0.0.0.0:*               LISTEN");
#else
        netstat_output = g_sshManager.executeCommand(check_netstat_cmd);
#endif
        LOG_CB(XORS("[CMD Output] ") + netstat_output);
        
        if (netstat_output.find(g_lastConfig.proxyPort) != std::string::npos) {
            LOG_CB(XORS("[TBox] Netstat check PASSED: Port redirection seems active."));
        } else {
             LOG_CB(XORS("[TBox] Netstat check WARNING: Port redirection might not be active!"));
        }

        // 3. CHECK CURL (Connectivity Test)
        LOG_CB(XORS("[CMD] Executing: ") + check_curl_cmd);
        std::string curl_output;
#ifdef MOCK_TBOX_LOGIC
        LOG_CB(XORS("[MOCK] Simulating curl output..."));
        curl_output = XORS("*   Trying ") + g_lastConfig.serverIp + XORS("...\n* Connected to gw-t5.beantechyun.com (") + g_lastConfig.serverIp + XORS(") port 8111\n< HTTP/1.1 200 OK\n< Server: CustomTBoxServer");
#else
        curl_output = g_sshManager.executeCommand(check_curl_cmd);
#endif
        LOG_CB(XORS("[CMD Output] ") + curl_output);
        
        // Проверяем, есть ли в ответе наш IP или успешный HTTP код
        if (curl_output.find(g_lastConfig.serverIp) != std::string::npos || curl_output.find("200 OK") != std::string::npos) {
             LOG_CB(XORS("[TBox] Curl check PASSED: Traffic is routed correctly."));
        } else {
             LOG_CB(XORS("[TBox] Curl check FAILED: Traffic might not be routed correctly."));
        }

#ifdef MOCK_TBOX_LOGIC
        LOG_CB(XORS("[MOCK] SSH Disconnect simulated"));
#else
        g_sshManager.disconnect();
#endif
        LOG_CB(XORS("[TBox] Config Applied (SSH Session closed)"));
    } else {
        LOG_CB(XORS("[TBox] SSH Connection Failed!"));
    }
}

void serviceLoop() {
    initGlobals(); // Убедимся, что все инициализировано
    
    LOG_CB(XORS("[TBox] Service Thread Started (Heartbeat Mode)"));
    LOG_CB(XORS("[TBox] Target Host: ") + g_connectivity_host);

    bool lastConnectionStatus = false;

    while (g_running) {
        // Heartbeat каждые 30 секунд
        // Используем checkConnectivity, который просто проверяет TCP/SSL хендшейк (или делает легкий запрос)
        bool isConnected = g_netClient.checkConnectivity(g_connectivity_host);
        
        // Уведомляем UI только если статус изменился
        if (isConnected != lastConnectionStatus) {
            lastConnectionStatus = isConnected;
            if (g_callback) g_callback->onServerStatusChanged(isConnected);
            LOG_CB(isConnected ? XORS("[TBox] Server ONLINE") : XORS("[TBox] Server OFFLINE"));
        }

        // Пауза 30 секунд
        for(int i=0; i<30; i++) {
            if(!g_running) break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void startTBoxService(ITBoxCallback* callback) {
    if (g_running) return;
    g_callback = callback; // Сохраняем callback
    g_running = true;
    g_serviceThread = std::thread(serviceLoop);
    g_serviceThread.detach();
}

void stopTBoxService() {
    g_running = false;
    if (g_serviceThread.joinable()) {
        g_serviceThread.join();
    }
    g_callback = nullptr;
}

// Заглушки
std::string sendVinToServer(const std::string& vin, const std::string& host) { return ""; }
void performTBoxSetup(const std::string& serverIp, const std::string& proxyPort) {}

std::string executeGetVin() {
    return tbox::VinReader::getVin();
}

} // namespace logic
} // namespace tbox