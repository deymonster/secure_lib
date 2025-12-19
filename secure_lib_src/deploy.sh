#!/bin/bash

# Пушим файлы
echo "[*] Pushing files to device..."
adb push build/libsecure_tbox.so /data/local/tmp/
adb push build/secure_test_app /data/local/tmp/

# Права
adb shell chmod +x /data/local/tmp/secure_test_app

# Аргумент IP (по умолчанию 127.0.0.1)
TARGET_IP=${1:-127.0.0.1}

echo "[*] Running secure_test_app with target IP: $TARGET_IP"
echo "---------------------------------------------------"

# Запуск с LD_LIBRARY_PATH
adb shell "export LD_LIBRARY_PATH=/data/local/tmp:\$LD_LIBRARY_PATH && /data/local/tmp/secure_test_app $TARGET_IP"