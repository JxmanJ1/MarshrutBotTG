#pragma once
#include "types.hpp"
#include <vector>
#include <string>
#include <optional>
#include <utility>

namespace router_bot {

class Geocoder {
public:
    explicit Geocoder(std::string user_agent =
                           "RouteOptimizerBot/1.0 (contact: your-email@example.com)");


    std::optional<std::pair<double, double>> geocodeOne(const std::string& query);


    void geocodeAll(std::vector<Address>& addresses);

private:
    std::string user_agent_;
    static constexpr const char* kNominatimUrl = "https://nominatim.openstreetmap.org/search";

    std::string httpGet(const std::string& url);
};

}
