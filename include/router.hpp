#pragma once
#include "types.hpp"
#include <vector>

namespace router_bot {


class RouteOptimizer {
public:

    static RouteResult buildCircularRoute(const std::vector<Address>& addresses);

private:

    static double haversineKm(double lat1, double lon1, double lat2, double lon2);


    static std::vector<int> nearestNeighbor(const std::vector<std::vector<double>>& dist,
                                             int start_idx);


    static void twoOptImprove(std::vector<int>& route,
                               const std::vector<std::vector<double>>& dist);

    static double totalRouteDistance(const std::vector<int>& route,
                                      const std::vector<std::vector<double>>& dist);
};

} 
