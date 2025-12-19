#include "security.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace tbox {

    bool Security::checkDebugger() {
        // 1. Простейшая проверка через ptrace (не сработает, если мы уже под ptrace)
        if (ptrace(PTRACE_TRACEME, 0, 1, 0) == -1) {
            return true; // Нас уже кто-то трассирует
        }
        
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