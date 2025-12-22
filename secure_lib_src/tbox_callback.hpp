#pragma once
#include <string>

namespace tbox {

class ITBoxCallback {
public:
    virtual ~ITBoxCallback() = default;

    // Вызывается при изменении статуса соединения с сервером
    // isConnected: true (зеленая иконка), false (красная)
    virtual void onServerStatusChanged(bool isConnected) = 0;

    // Вызывается при изменении статуса доступности T-Box (172.16.2.97)
    virtual void onTboxStatusChanged(bool isConnected) = 0;

    // Вызывается при успешном получении конфига
    // vin: VIN номер
    // subActive: статус подписки
    virtual void onConfigReceived(const std::string& vin, bool subActive) = 0;

    // Для логирования/отладки в UI
    virtual void onLog(const std::string& message) = 0;
};

} // namespace tbox