#include "network_client.hpp"
#include <curl/curl.h>
#include <iostream>
#include "tbox_keys.hpp"
#include "obfuscation.hpp"
#include "XorStr.hpp"

namespace tbox {

NetworkClient::NetworkClient() {
    // Глобальная инициализация должна выполняться один раз, 
    // но curl_global_init безопасен для многократного вызова в новых версиях, 
    // однако лучше делать это в main/JNI_OnLoad. Здесь оставим для надежности.
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

NetworkClient::~NetworkClient() {
    // curl_global_cleanup(); // Лучше не вызывать деструктор глобально, если есть другие потоки
}

size_t NetworkClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void NetworkClient::setupCertificates(void* curl_handle) {
    CURL* curl = (CURL*)curl_handle;
    
    const unsigned char KEY = 0x55;
    // Расшифровка ключей и сертификатов в память
    std::string ca_pem = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::ca_pem_arr, tbox::keys::ca_pem_arr + sizeof(tbox::keys::ca_pem_arr)), KEY);
    std::string client_cert = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::client_cert_pem_arr, tbox::keys::client_cert_pem_arr + sizeof(tbox::keys::client_cert_pem_arr)), KEY);
    std::string client_key = Obfuscator::decrypt(std::vector<unsigned char>(tbox::keys::client_key_pem_arr, tbox::keys::client_key_pem_arr + sizeof(tbox::keys::client_key_pem_arr)), KEY);

#ifdef CURL_BLOB_COPY
    // Используем BLOB API для передачи сертификатов из памяти (требуется libcurl >= 7.77.0)
    struct curl_blob ca_blob;
    ca_blob.data = (void*)ca_pem.c_str();
    ca_blob.len = ca_pem.length();
    ca_blob.flags = CURL_BLOB_COPY;
    curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &ca_blob);

    struct curl_blob cert_blob;
    cert_blob.data = (void*)client_cert.c_str();
    cert_blob.len = client_cert.length();
    cert_blob.flags = CURL_BLOB_COPY;
    curl_easy_setopt(curl, CURLOPT_SSLCERT_BLOB, &cert_blob);
    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");

    struct curl_blob key_blob;
    key_blob.data = (void*)client_key.c_str();
    key_blob.len = client_key.length();
    key_blob.flags = CURL_BLOB_COPY;
    curl_easy_setopt(curl, CURLOPT_SSLKEY_BLOB, &key_blob);
    curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "PEM");
#else
    #pragma message("Warning: CURL_BLOB_COPY not defined. mTLS from memory requires newer libcurl.")
    // Если libcurl старый, здесь нужен фоллбек: либо запись во временные файлы (небезопасно),
    // либо использование CURLOPT_SSLCTX_FUNCTION (сложнее, требует OpenSSL headers).
#endif
    
    // В продакшене включите проверку хоста!
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
}

std::string NetworkClient::postSecure(const std::string& url, const std::string& jsonPayload) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30 секунд таймаут
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, XORS("Content-Type: application/json").c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        setupCertificates(curl);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        CURLcode res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
#ifndef STRIP_LOGS
            std::cerr << "postSecure failed: " << curl_easy_strerror(res) << std::endl;
#endif
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return XORS("CURL_ERROR: ") + curl_easy_strerror(res);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

bool NetworkClient::checkConnectivity(const std::string& host) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    // Используем эндпоинт /heartbeat для полной проверки mTLS
    std::string url = XORS("https://") + host + XORS("/heartbeat"); 
    std::string readBuffer; // Буфер для ответа
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // 5 секунд на проверку
    // curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD вызывает 405 Method Not Allowed на некоторых серверах
    
    // Используем GET и читаем ответ
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    // Настраиваем сертификаты для mTLS
    setupCertificates(curl);

    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_easy_cleanup(curl);

    // Считаем успешным, если курл отработал и сервер вернул 200 OK
    return (res == CURLE_OK && http_code == 200);
}

bool NetworkClient::checkReachability(const std::string& ip, int port) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = XORS("http://") + ip + ":" + std::to_string(port);
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L); // 3 seconds timeout
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_easy_cleanup(curl);
    
    // We only care if we could connect, even if 404 or whatever
    return (res != CURLE_COULDNT_CONNECT && res != CURLE_OPERATION_TIMEDOUT);
}

} // namespace tbox