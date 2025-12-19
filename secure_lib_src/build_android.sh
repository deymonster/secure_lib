#!/bin/bash

# Путь к NDK (замените, если отличается)
export ANDROID_NDK_HOME=$HOME/Library/Android/sdk/ndk/29.0.14033849
# Если NDK установлен через brew или в другом месте, укажите верный путь.
# Если переменная не задана, попробуем найти.
if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "NDK not found at default location. Please set ANDROID_NDK_HOME."
    exit 1
fi

export TOOLCHAIN=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/darwin-x86_64

# Создаем папку сборки
mkdir -p build
cd build

# Запускаем CMake
# ABI: arm64-v8a (наиболее вероятно для T-Box), если старый - armeabi-v7a
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-24 \
      -DENABLE_SSH=ON \
      ..

# Компилируем
make

echo "Build complete."
echo "To run on device:"
echo "adb push libsecure_tbox.so /data/local/tmp/"
echo "adb push secure_test_app /data/local/tmp/"
echo "adb shell chmod +x /data/local/tmp/secure_test_app"
echo "adb shell \"export LD_LIBRARY_PATH=/data/local/tmp:\$LD_LIBRARY_PATH && /data/local/tmp/secure_test_app\""