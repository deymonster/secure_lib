cmake_minimum_required(VERSION 3.18.0)

project("secure_tbox")

# Опция для включения/выключения SSH
option(ENABLE_SSH "Enable SSH functionality" ON)

# Определяем пути к зависимостям
set(LIBS_DIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs_dist)
include_directories(${LIBS_DIST_DIR}/include)

# Основные исходники
set(SOURCE_FILES
    vin_utils.cpp
    security.cpp
    ssh_manager.cpp
)

add_library(secure_tbox SHARED ${SOURCE_FILES})

find_library(log-lib log)
find_library(z-lib z) # OpenSSL/libssh могут требовать zlib

# Линкуем статические библиотеки
if(ENABLE_SSH)
    add_definitions(-DENABLE_SSH)
    # Порядок важен: libssh -> libssl -> libcrypto
    target_link_libraries(secure_tbox
        ${LIBS_DIST_DIR}/lib/libssh.a
        ${LIBS_DIST_DIR}/lib/libssl.a
        ${LIBS_DIST_DIR}/lib/libcrypto.a
        ${log-lib}
        ${z-lib}
    )
else()
    target_link_libraries(secure_tbox ${log-lib})
endif()

# Тестовое приложение (консольное) для запуска через ADB
add_executable(secure_test_app test_main.cpp)
target_link_libraries(secure_test_app 
    secure_tbox 
    ${LIBS_DIST_DIR}/lib/libcurl.a
    ${LIBS_DIST_DIR}/lib/libssl.a
    ${LIBS_DIST_DIR}/lib/libcrypto.a
    ${log-lib}
    ${z-lib}
)