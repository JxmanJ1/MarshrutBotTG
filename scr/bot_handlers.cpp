#include "bot_handlers.hpp"
#include "geocoder.hpp"
#include "router.hpp"
#include "file_generator.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <iostream>

namespace router_bot {

BotHandlers::BotHandlers(TgBot::Bot& bot) : bot_(bot) {}

std::string BotHandlers::trim(const std::string& s) {
    const size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<Address> BotHandlers::parseAddressLines(const std::string& rawText) {
    std::vector<Address> result;
    std::istringstream stream(rawText);
    std::string line;
    int idx = 0;

    while (std::getline(stream, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        Address addr;
        addr.original_index = idx++;
        addr.raw_text = trimmed;

const size_t semicolonPos = trimmed.find(';');
if (semicolonPos != std::string::npos) {

    addr.name = trim(trimmed.substr(0, semicolonPos));
    addr.full_address = trim(trimmed.substr(semicolonPos + 1));
} else {
    addr.full_address = trimmed;
}

        if (addr.full_address.empty()) {
            addr.full_address = trimmed;
        }

        result.push_back(addr);
    }

    return result;
}

std::string BotHandlers::downloadDocumentText(const std::string& fileId) {
    try {
        TgBot::File::Ptr file = bot_.getApi().getFile(fileId);
        if (!file || file->filePath.empty()) {
            return "";
        }

        const std::string downloadedPath = bot_.getApi().downloadFile(file->filePath);
        if (downloadedPath.empty()) {
            return "";
        }

        std::ifstream input(downloadedPath, std::ios::binary);
        if (!input) {
            return downloadedPath;
        }

        std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
        std::remove(downloadedPath.c_str());
        return content;
    } catch (const std::exception& e) {
        std::cerr << "[BotHandlers] Помилка завантаження файлу: " << e.what() << std::endl;
        return "";
    }
}

void BotHandlers::onStart(const TgBot::Message::Ptr& message) {
    bot_.getApi().sendMessage(message->chat->id,
        "Привіт! Надішли мені список адрес (кожна адреса з нового рядка) "
        "одним повідомленням, або текстовим .txt файлом.\n\n"
        "Перша адреса в списку стане стартовою точкою маршруту. "
        "Я геокодую всі адреси, побудую оптимальний коловий маршрут "
        "(старт -> усі точки -> повернення до старту) і надішлю звіт "
        "у форматах Excel (.xlsx) та PDF.");
}

void BotHandlers::onTextMessage(const TgBot::Message::Ptr& message) {
    if (!message->text.empty() && message->text[0] != '/') {
        processAddressList(message->chat->id, message->text);
    }
}

void BotHandlers::onDocumentMessage(const TgBot::Message::Ptr& message) {
    if (!message->document) return;

    bot_.getApi().sendMessage(message->chat->id, "Отримав файл, обробляю...");

    const std::string content = downloadDocumentText(message->document->fileId);
    if (content.empty()) {
        bot_.getApi().sendMessage(message->chat->id,
            "Не вдалось прочитати файл. Переконайтесь, що це звичайний "
            "текстовий .txt файл у кодуванні UTF-8.");
        return;
    }

    processAddressList(message->chat->id, content);
}

void BotHandlers::processAddressList(std::int64_t chatId, const std::string& rawText) {
    std::vector<Address> addresses = parseAddressLines(rawText);

    if (addresses.size() < 2) {
        bot_.getApi().sendMessage(chatId, "Потрібно щонайменше 2 адреси, щоб побудувати маршрут.");
        return;
    }

    bot_.getApi().sendMessage(chatId,
        "Розпізнано " + std::to_string(addresses.size()) +
        " адрес(и). Починаю геокодування, це може зайняти трохи часу...");

    Geocoder geocoder;
    geocoder.geocodeAll(addresses);

    std::vector<Address> geocoded;
    std::vector<std::string> failed;
    for (const auto& addr : addresses) {
        if (addr.geocoded) {
            geocoded.push_back(addr);
        } else {
            failed.push_back(addr.full_address);
        }
    }

    if (!failed.empty()) {
        std::string msg = "Не вдалось визначити координати для:\n";
        for (const auto& f : failed) msg += "- " + f + "\n";
        msg += "\nЦі адреси не увійдуть у маршрут.";
        bot_.getApi().sendMessage(chatId, msg);
    }

    if (geocoded.size() < 2) {
        bot_.getApi().sendMessage(chatId, "Замало успішно геокодованих адрес для побудови маршруту.");
        return;
    }

    bot_.getApi().sendMessage(chatId, "Будую оптимальний коловий маршрут...");

    const RouteResult route = RouteOptimizer::buildCircularRoute(geocoded);

    std::vector<Address> ordered;
    ordered.reserve(route.order.size());
    for (const int idx : route.order) {
        ordered.push_back(geocoded[idx]);
    }

    const std::filesystem::path tempDir = std::filesystem::temp_directory_path();
    const std::string filename = "route_" + std::to_string(chatId) + "_" +
                                 std::to_string(std::time(nullptr));
    const std::filesystem::path basePath = tempDir / filename;
    const std::string xlsxPath = (basePath.string() + ".xlsx");
    const std::string pdfPath = (basePath.string() + ".pdf");

    const std::string xlsxResult = FileGenerator::generateXlsx(ordered, xlsxPath);
    const std::string pdfResult = FileGenerator::generatePdf(ordered, pdfPath);

    char distanceBuf[64];
    std::snprintf(distanceBuf, sizeof(distanceBuf), "%.2f", route.total_distance_km);
    bot_.getApi().sendMessage(chatId,
        "Маршрут готовий! Орієнтовна довжина кола: " + std::string(distanceBuf) + " км.");

    if (!xlsxResult.empty()) {
        TgBot::InputFile::Ptr xlsxFile = TgBot::InputFile::fromFile(
            xlsxResult, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
        bot_.getApi().sendDocument(chatId, xlsxFile);
    } else {
        bot_.getApi().sendMessage(chatId, "Не вдалось згенерувати Excel-файл.");
    }

    if (!pdfResult.empty()) {
        TgBot::InputFile::Ptr pdfFile = TgBot::InputFile::fromFile(pdfResult, "application/pdf");
        bot_.getApi().sendDocument(chatId, pdfFile);
    } else {
        bot_.getApi().sendMessage(chatId, "Не вдалось згенерувати PDF-файл.");
    }

    // Прибираємо тимчасові файли після відправки користувачу
    if (!xlsxResult.empty()) std::remove(xlsxResult.c_str());
    if (!pdfResult.empty()) std::remove(pdfResult.c_str());
}

void BotHandlers::registerHandlers() {
    bot_.getEvents().onCommand("start", [this](const TgBot::Message::Ptr& message) {
        onStart(message);
    });

    bot_.getEvents().onAnyMessage([this](const TgBot::Message::Ptr& message) {
        if (message->document) {
            onDocumentMessage(message);
        } else if (!message->text.empty() && message->text != "/start") {
            onTextMessage(message);
        }
    });
}

} 
