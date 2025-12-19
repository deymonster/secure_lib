#include "security.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(__linux__) || defined(__ANDROID__)
    #include <sys/ptrace.h>
#endif

namespace tbox {

    bool Security::checkDebugger() {
#if defined(__linux__) || defined(__ANDROID__)
        // 1. Простейшая проверка через ptrace (не сработает, если мы уже под ptrace)
        // Note: PTRACE_TRACEME arg types may vary, using 0, 0, 0 for broad compatibility if needed, 
        // but standard linux is (request, pid, addr, data)
        #ifdef PTRACE_TRACEME
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1) {
            return true; // Нас уже кто-то трассирует
        }
        #endif
        
        // 2. Чтение /proc/self/status и поиск TracerPid
        FILE* fp = fopen("/proc/self/status", "r");
        if (!fp) return false;

        char line[256];
        bool traced = false;
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "TracerPid:", 10) == 0) {
                int pid = atoi(&line[10]);
                if (pid != 0) {
                    traced = true;
                }
                break;
            }
        }
        fclose(fp);
        return traced;
#else
        return false;
#endif
    }

    bool Security::checkFrida() {
        // Сканируем карты памяти на наличие строк "frida"
        FILE* fp = fopen("/proc/self/maps", "r");
        if (!fp) return false;

        char line[512];
        bool found = false;
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "frida") || strstr(line, "gadget")) {
                found = true;
                break;
            }
        }
        fclose(fp);
        return found;
    }

    void Security::antiDebugBreak() {
        // Тихо убиваем процесс
        kill(getpid(), SIGKILL);
    }
}