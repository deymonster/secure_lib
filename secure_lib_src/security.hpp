#pragma once

namespace tbox {
    class Security {
    public:
        // Возвращает true, если обнаружена угроза
        static bool checkDebugger();
        static bool checkFrida();
        static void antiDebugBreak(); // Вызвать краш или выход
    };
}