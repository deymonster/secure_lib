#include <jni.h>
#include <string>
#include <iostream>
#include "tbox_logic.hpp"
#include "tbox_callback.hpp"

// Глобальная ссылка на JVM, нужна чтобы получить JNIEnv в фоновом потоке
JavaVM* g_jvm = nullptr;
jobject g_listenerObject = nullptr; // Ссылка на Java объект слушателя

// Класс-мост: получает вызовы от C++ logic и перенаправляет их в Java
class AndroidCallback : public tbox::ITBoxCallback {
public:
    void onServerStatusChanged(bool isConnected) override {
        JNIEnv* env = getEnv();
        if (!env || !g_listenerObject) return;

        // Ищем метод: void onServerStatusChanged(boolean)
        jclass cls = env->GetObjectClass(g_listenerObject);
        jmethodID mid = env->GetMethodID(cls, "onServerStatusChanged", "(Z)V");
        if (mid) {
            env->CallVoidMethod(g_listenerObject, mid, (jboolean)isConnected);
        }
    }

    void onTboxStatusChanged(bool isConnected) override {
        JNIEnv* env = getEnv();
        if (!env || !g_listenerObject) return;

        // Ищем метод: void onTboxStatusChanged(boolean)
        jclass cls = env->GetObjectClass(g_listenerObject);
        jmethodID mid = env->GetMethodID(cls, "onTboxStatusChanged", "(Z)V");
        if (mid) {
            env->CallVoidMethod(g_listenerObject, mid, (jboolean)isConnected);
        }
    }

    void onConfigReceived(const std::string& vin, bool subActive) override {
        JNIEnv* env = getEnv();
        if (!env || !g_listenerObject) return;

        // Ищем метод: void onConfigReceived(String, boolean)
        jclass cls = env->GetObjectClass(g_listenerObject);
        jmethodID mid = env->GetMethodID(cls, "onConfigReceived", "(Ljava/lang/String;Z)V");
        if (mid) {
            jstring jVin = env->NewStringUTF(vin.c_str());
            env->CallVoidMethod(g_listenerObject, mid, jVin, (jboolean)subActive);
            env->DeleteLocalRef(jVin);
        }
    }

    void onLog(const std::string& message) override {
        JNIEnv* env = getEnv();
        if (!env || !g_listenerObject) return;

        // Ищем метод: void onLog(String)
        jclass cls = env->GetObjectClass(g_listenerObject);
        jmethodID mid = env->GetMethodID(cls, "onLog", "(Ljava/lang/String;)V");
        if (mid) {
            jstring jMsg = env->NewStringUTF(message.c_str());
            env->CallVoidMethod(g_listenerObject, mid, jMsg);
            env->DeleteLocalRef(jMsg);
        }
    }

private:
    JNIEnv* getEnv() {
        JNIEnv* env;
        int status = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            if (g_jvm->AttachCurrentThread(&env, nullptr) != 0) {
                return nullptr;
            }
        }
        return env;
    }
};

// Глобальный экземпляр колбэка
static AndroidCallback g_androidCallback;

extern "C" {

// Инициализация при загрузке библиотеки
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

// Start Service: теперь принимает объект Listener из Java
// Signature: (Lcom/tbox/secure/TBoxListener;)V
JNIEXPORT void JNICALL
Java_com_tbox_secure_NativeLib_startTBox(JNIEnv* env, jobject /* this */, jobject listener) {
    
    // Создаем глобальную ссылку на слушателя, чтобы GC его не удалил
    if (g_listenerObject) {
        env->DeleteGlobalRef(g_listenerObject);
    }
    g_listenerObject = env->NewGlobalRef(listener);

    tbox::logic::startTBoxService(&g_androidCallback);
}

JNIEXPORT void JNICALL
Java_com_tbox_secure_NativeLib_stopTBox(JNIEnv* env, jobject /* this */) {
    tbox::logic::stopTBoxService();
    
    if (g_listenerObject) {
        env->DeleteGlobalRef(g_listenerObject);
        g_listenerObject = nullptr;
    }
}

JNIEXPORT void JNICALL
Java_com_tbox_secure_NativeLib_getConfig(JNIEnv* env, jobject /* this */) {
    tbox::logic::executeGetConfig();
}

JNIEXPORT void JNICALL
Java_com_tbox_secure_NativeLib_applyConfig(JNIEnv* env, jobject /* this */) {
    tbox::logic::executeApplyConfig();
}

JNIEXPORT void JNICALL
Java_com_tbox_secure_NativeLib_checkTBoxStatus(JNIEnv* env, jobject /* this */) {
    tbox::logic::executeCheckTBoxStatus();
}

JNIEXPORT jstring JNICALL
Java_com_tbox_secure_NativeLib_getVin(JNIEnv* env, jobject /* this */) {
    std::string vin = tbox::logic::executeGetVin();
    return env->NewStringUTF(vin.c_str());
}

} // extern "C"