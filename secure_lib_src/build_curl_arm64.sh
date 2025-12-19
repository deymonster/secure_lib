#!/bin/bash
set -e

# Configuration
# Adjust NDK path to match your environment (taken from build_android.sh)
export ANDROID_NDK_HOME=${ANDROID_NDK_HOME:-$HOME/Library/Android/sdk/ndk/29.0.14033849}
API_LEVEL=29
# Note: Newer NDKs use different toolchain paths sometimes. Adjust if needed.
TOOLCHAIN=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/darwin-x86_64
TARGET_HOST=aarch64-linux-android

# Check if NDK exists
if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "Error: ANDROID_NDK_HOME not found at $ANDROID_NDK_HOME"
    echo "Please set ANDROID_NDK_HOME environment variable."
    exit 1
fi

export AR=$TOOLCHAIN/bin/llvm-ar
export CC=$TOOLCHAIN/bin/${TARGET_HOST}${API_LEVEL}-clang
export CXX=$TOOLCHAIN/bin/${TARGET_HOST}${API_LEVEL}-clang++
export LD=$TOOLCHAIN/bin/ld
export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
export STRIP=$TOOLCHAIN/bin/llvm-strip

CURL_VER=8.4.0
CURL_DIR=curl-${CURL_VER}
LIBS_DIST=$(pwd)/libs_dist

echo "Building Curl for ARM64..."
echo "NDK: $ANDROID_NDK_HOME"
echo "Install Prefix: $LIBS_DIST"

# Download Curl if not exists
if [ ! -d "$CURL_DIR" ]; then
    if [ ! -f "curl-${CURL_VER}.tar.gz" ]; then
        echo "Downloading Curl ${CURL_VER}..."
        curl -LO https://curl.se/download/curl-${CURL_VER}.tar.gz
    fi
    echo "Extracting Curl..."
    tar xzf curl-${CURL_VER}.tar.gz
fi

cd $CURL_DIR

# Clean previous build
make clean || true

# Configure
# Pointing --with-openssl to libs_dist where we have include/openssl and lib/libssl.a
./configure \
  --host=$TARGET_HOST \
  --prefix=$LIBS_DIST \
  --with-openssl=$LIBS_DIST \
  --disable-shared \
  --enable-static \
  --disable-dict \
  --disable-file \
  --disable-ftp \
  --disable-gopher \
  --disable-imap \
  --disable-ldap \
  --disable-ldaps \
  --disable-pop3 \
  --disable-rtsp \
  --disable-smb \
  --disable-smtp \
  --disable-telnet \
  --disable-tftp \
  --without-libidn2 \
  --without-librtmp \
  --without-brotli \
  --without-zstd \
  --enable-ipv6

# Build
make -j8

# Install
make install

echo "Done. Libcurl installed to $LIBS_DIST"