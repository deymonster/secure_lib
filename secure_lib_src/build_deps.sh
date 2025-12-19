#!/bin/bash
set -e

# Настройки (используем тот же NDK)
export ANDROID_NDK_HOME=$HOME/Library/Android/sdk/ndk/29.0.14033849
# В новых NDK (r19+) toolchain объединен
export TOOLCHAIN_ROOT=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/darwin-x86_64
export PATH=$TOOLCHAIN_ROOT/bin:$PATH

export ANDROID_API=24
export ARCH=arm64-v8a

# Папка для установки библиотек
INSTALL_DIR=$(pwd)/libs_dist
mkdir -p $INSTALL_DIR

# 1. Скачиваем и собираем OpenSSL
if [ ! -d "openssl" ]; then
    echo "Cloning OpenSSL..."
    git clone --depth 1 -b OpenSSL_1_1_1w https://github.com/openssl/openssl.git
fi

cd openssl
echo "Building OpenSSL for $ARCH..."

# Настраиваем переменные окружения для кросс-компиляции (OpenSSL требует их)
export CC=$TOOLCHAIN_ROOT/bin/aarch64-linux-android${ANDROID_API}-clang
export CXX=$TOOLCHAIN_ROOT/bin/aarch64-linux-android${ANDROID_API}-clang++
export AR=$TOOLCHAIN_ROOT/bin/llvm-ar
export RANLIB=$TOOLCHAIN_ROOT/bin/llvm-ranlib
export LD=$TOOLCHAIN_ROOT/bin/ld

# Конфигурация для Android arm64
# Используем generic 'android-arm64' и передаем флаги через параметры или переменные
./Configure android-arm64 -D__ANDROID_API__=$ANDROID_API --prefix=$INSTALL_DIR --openssldir=$INSTALL_DIR/ssl no-shared

make -j4
make install_sw
cd ..

# 2. Скачиваем и собираем libssh
if [ ! -d "libssh" ]; then
    echo "Cloning libssh (full history to ensure tags are found)..."
    git clone https://github.com/libssh/libssh-mirror.git libssh
fi

# Переключаемся на нужную версию
cd libssh
echo "Fetching tags and checking out libssh-0.9.8..."
git fetch --tags
git checkout libssh-0.9.8 || git checkout tags/libssh-0.9.8 || echo "Warning: Could not checkout libssh-0.9.8, using current branch"

# Патчим исходники для совместимости с новым NDK и Clang
echo "Patching libssh sources..."
# 1. Исправляем ошибку strict-prototypes в init.c
if [ -f "src/init.c" ]; then
    perl -pi -e 's/bool is_ssh_initialized\(\) \{/bool is_ssh_initialized(void) \{/g' src/init.c
fi
# 2. Удаляем -Werror из всех файлов CMake, чтобы предупреждения не ломали сборку
find . -name "CMakeLists.txt" -o -name "*.cmake" | xargs perl -pi -e 's/-Werror//g'

cd ..

mkdir -p libssh/build
cd libssh/build

echo "Building libssh for $ARCH..."
# Сбрасываем CC/CXX, чтобы CMake сам нашел их через toolchain file
unset CC CXX AR RANLIB LD

# Добавляем CMAKE_POLICY_VERSION_MINIMUM=3.5 для совместимости с новыми версиями CMake
# Добавляем -Wno-error и -Wno-strict-prototypes, чтобы игнорировать ошибки старого кода (init.c)
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      -DCMAKE_C_FLAGS="-Wno-error -Wno-strict-prototypes" \
      -DANDROID_ABI=$ARCH \
      -DANDROID_PLATFORM=android-$ANDROID_API \
      -DOPENSSL_ROOT_DIR=$INSTALL_DIR \
      -DOPENSSL_INCLUDE_DIR=$INSTALL_DIR/include \
      -DOPENSSL_CRYPTO_LIBRARY=$INSTALL_DIR/lib/libcrypto.a \
      -DOPENSSL_SSL_LIBRARY=$INSTALL_DIR/lib/libssl.a \
      -DWITH_EXAMPLES=OFF \
      -DWITH_SERVER=OFF \
      -DWITH_GSSAPI=OFF \
      -DWITH_ZLIB=OFF \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
      ..

make -j4
make install
cd ../..

echo "Done! Libraries installed in $INSTALL_DIR"