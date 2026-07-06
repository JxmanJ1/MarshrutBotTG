#pragma once
#include <string>
#include <vector>

namespace router_bot {

struct Address {
    int original_index = 0;    
    std::string raw_text;       
    std::string name;          
    std::string full_address;   
    double latitude = 0.0;
    double longitude = 0.0;
    bool geocoded = false;      
};


struct RouteResult {
    std::vector<int> order;        
    double total_distance_km = 0.0; 
};

} 
