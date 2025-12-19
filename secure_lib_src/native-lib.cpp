#include <string>
#include "tbox_logic.hpp"

// Linter workaround: Mock JNI types if headers are missing
#if defined(__has_include) && !__has_include(<jni.h>)
    #include <cstdint>
    using jstring = void*;
    using jobject = void*;
    using jboolean = unsigned char;
    #define JNIEXPORT
    #define JNICALL
    
    struct JNIEnvMock {
        const char* GetStringUTFChars(jstring, jboolean*) { return ""; }
        void ReleaseStringUTFChars(jstring, const char*) {}
        jstring NewStringUTF(const char*) { return nullptr; }
    };
    using JNIEnv = JNIEnvMock;
#else
    #include <jni.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL
Java_com_android_car_tbox_secure_SecureTBox_sendVin(
        JNIEnv* env,
        jobject /* this */,
        jstring vin_j,
        jstring host_j) {
    
    if (!env) return nullptr;

    const char* vin_c = env->GetStringUTFChars(vin_j, 0);
    const char* host_c = env->GetStringUTFChars(host_j, 0);
    
    std::string vin(vin_c ? vin_c : "");
    std::string host(host_c ? host_c : "");
    
    if (vin_c) env->ReleaseStringUTFChars(vin_j, vin_c);
    if (host_c) env->ReleaseStringUTFChars(host_j, host_c);

    std::string response = tbox::logic::sendVinToServer(vin, host);
    
    return env->NewStringUTF(response.c_str());
}

JNIEXPORT void JNICALL
Java_com_android_car_tbox_secure_SecureTBox_performSetup(
        JNIEnv* env,
        jobject /* this */,
        jstring serverIp_j,
        jstring proxyPort_j) {
            
    if (!env) return;

    const char* serverIp_c = env->GetStringUTFChars(serverIp_j, 0);
    const char* proxyPort_c = env->GetStringUTFChars(proxyPort_j, 0);
    
    std::string serverIp(serverIp_c ? serverIp_c : "");
    std::string proxyPort(proxyPort_c ? proxyPort_c : "");
    
    if (serverIp_c) env->ReleaseStringUTFChars(serverIp_j, serverIp_c);
    if (proxyPort_c) env->ReleaseStringUTFChars(proxyPort_j, proxyPort_c);

    tbox::logic::performTBoxSetup(serverIp, proxyPort);
}

#ifdef __cplusplus
}
#endif
