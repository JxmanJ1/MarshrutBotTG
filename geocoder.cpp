#include "geocoder.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>

namespace router_bot {

namespace {

size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

std::string urlEncode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (!curl) return value;

    char* output = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
    std::string result;
    if (output) {
        result = output;
        curl_free(output);
    }
    curl_easy_cleanup(curl);
    return result;
}

} // namespace

Geocoder::Geocoder(std::string user_agent) : user_agent_(std::move(user_agent)) {}

std::string Geocoder::httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response_buffer;
    if (!curl) {
        std::cerr << "[Geocoder] Не вдалось ініціалізувати curl" << std::endl;
        return response_buffer;
    }

    std::string browser_ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, browser_ua.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[Geocoder] HTTP помилка: " << curl_easy_strerror(res) << std::endl;
        response_buffer.clear();
    }

    curl_easy_cleanup(curl);
    return response_buffer;
}

std::optional<std::pair<double, double>> Geocoder::geocodeOne(const std::string& query) {
    if (query.empty()) return std::nullopt;

    // Используем прямой, гарантированно существующий базовый эндпоинт
    std::string base_url = "https://nominatim.openstreetmap.org/search";

    std::ostringstream url;
    url << base_url
        << "?format=json"
        << "&limit=1"
        << "&addressdetails=0"
        << "&email=rofedo228@gmail.com"
        << "&q=" << urlEncode(query);

    std::string body = httpGet(url.str());
    if (body.empty()) {
        return std::nullopt;
    }

    try {
        auto json = nlohmann::json::parse(body);
        if (!json.is_array() || json.empty()) {
            return std::nullopt;
        }
        double lat = std::stod(json[0]["lat"].get<std::string>());
        double lon = std::stod(json[0]["lon"].get<std::string>());
        return std::make_pair(lat, lon);
    } catch (const std::exception& e) {
        std::cerr << "[Geocoder] Помилка парсингу JSON-відповіді: " << e.what() << std::endl;
        std::cerr << "[Geocoder] Сырой ответ сервера: " << body << std::endl;
        return std::nullopt;
    }
}

void Geocoder::geocodeAll(std::vector<Address>& addresses) {
    for (auto& addr : addresses) {
        std::string combined_query = addr.full_address;
        if (!addr.name.empty()) {
            combined_query = addr.name + ", " + addr.full_address;
        }

        auto coords = geocodeOne(combined_query);
        if (!coords && !addr.name.empty()) {
            coords = geocodeOne(addr.full_address);
        }

        if (coords) {
            addr.latitude = coords->first;
            addr.longitude = coords->second;
            addr.geocoded = true;
        } else {
            addr.geocoded = false;
            std::cerr << "[Geocoder] Не вдалось геокодувати: " << addr.full_address << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    }
}

} // namespace router_bot