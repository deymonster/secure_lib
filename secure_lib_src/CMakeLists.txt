cmake_minimum_required(VERSION 3.18.0)

project("secure_tbox")

# Enforce C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Security and Obfuscation Flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-fvisibility=hidden) # Hide internal symbols
    add_compile_options(-fvisibility-inlines-hidden)
    add_compile_options(-Os) # Optimize for size (makes code harder to read)
    add_definitions(-DNDEBUG) # Standard macro to disable asserts and debug code
    add_definitions(-DSTRIP_LOGS) # Our custom macro to remove logs
elseif(CMAKE_BUILD_TYPE STREQUAL "Mock")
    message(STATUS "MOCK MODE ENABLED: Simulating T-Box connection")
    add_definitions(-DMOCK_TBOX_LOGIC)
endif()

# Опция для включения/выключения SSH
option(ENABLE_SSH "Enable SSH functionality" ON)

# Определяем пути к зависимостям
set(LIBS_DIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs_dist)
include_directories(${LIBS_DIST_DIR}/include)

# Find JNI (Java Native Interface)
# find_package(JNI REQUIRED)
# if (JNI_FOUND)
#     message (STATUS "JNI found: ${JNI_INCLUDE_DIRS}")
#     include_directories(${JNI_INCLUDE_DIRS})
# endif()

# Основные исходники
set(SOURCE_FILES
    vin_utils.cpp
    security.cpp
    ssh_manager.cpp
    network_client.cpp
    tbox_logic.cpp
    native-lib.cpp
)

add_library(secure_tbox SHARED ${SOURCE_FILES})

find_library(log-lib log)
find_library(z-lib z) # OpenSSL/libssh могут требовать zlib

# Линкуем статические библиотеки
if(ENABLE_SSH)
    add_definitions(-DENABLE_SSH)
    # Порядок важен: libssh -> libssl -> libcrypto
    target_link_libraries(secure_tbox
       ${LIBS_DIST_DIR}/lib/libcurl.a   
        ${LIBS_DIST_DIR}/lib/libssh.a
        ${LIBS_DIST_DIR}/lib/libssl.a
        ${LIBS_DIST_DIR}/lib/libcrypto.a
        ${log-lib}
        ${z-lib}
    )
else()
    target_link_libraries(secure_tbox ${log-lib})
endif()
