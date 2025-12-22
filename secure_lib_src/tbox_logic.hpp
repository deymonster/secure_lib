#pragma once

#include <string>
#include "tbox_callback.hpp"

namespace tbox {
namespace logic {

    // Запуск сервиса. Принимает callback для уведомлений.
    // Важно: объект callback должен жить до вызова stopTBoxService.
    void startTBoxService(ITBoxCallback* callback);

    void stopTBoxService();

    // Новые функции для ручного управления
    void executeGetConfig();
    void executeApplyConfig();
    void executeCheckTBoxStatus();
    std::string executeGetVin();

    // Вспомогательные (можно оставить или скрыть)
    std::string sendVinToServer(const std::string& vin, const std::string& host);
    void performTBoxSetup(const std::string& serverIp, const std::string& proxyPort);

} // namespace logic
} // namespace tbox