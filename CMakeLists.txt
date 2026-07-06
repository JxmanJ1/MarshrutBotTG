cmake_minimum_required(VERSION 3.15)
project(RouterTelegramBot LANGUAGES CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-include cstdint)

include(FetchContent)

# Оставляем только OpenXLSX, так как его нет в пакетах MSYS2, но он собирается без проблем
FetchContent_Declare(openxlsx
    GIT_REPOSITORY https://github.com/troldal/OpenXLSX.git
    GIT_TAG        v0.3.2
)
set(OPENXLSX_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(openxlsx)

# Находим ВСЕ установленные в MSYS2 пакеты
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)


find_package(Boost 1.65.1 REQUIRED)
find_package(TgBot REQUIRED)

add_executable(route_bot
    src/main.cpp
    src/bot_handlers.cpp
    src/geocoder.cpp
    src/router.cpp
    src/file_generator.cpp
)

target_include_directories(route_bot PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(route_bot PRIVATE
    TgBot::TgBot
    OpenXLSX
    CURL::libcurl
    nlohmann_json::nlohmann_json
)