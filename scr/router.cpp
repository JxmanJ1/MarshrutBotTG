#include "router.hpp"
#include <cmath>
#include <limits>
#include <algorithm>

namespace router_bot {

double RouteOptimizer::haversineKm(double lat1, double lon1, double lat2, double lon2) {
    constexpr double kEarthRadiusKm = 6371.0088;
    const double dLat = (lat2 - lat1) * M_PI / 180.0;
    const double dLon = (lon2 - lon1) * M_PI / 180.0;

    const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                      std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
                      std::sin(dLon / 2) * std::sin(dLon / 2);
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusKm * c;
}

std::vector<int> RouteOptimizer::nearestNeighbor(const std::vector<std::vector<double>>& dist,
                                                   int start_idx) {
    const int n = static_cast<int>(dist.size());
    std::vector<bool> visited(n, false);
    std::vector<int> route;
    route.reserve(n);

    int current = start_idx;
    visited[current] = true;
    route.push_back(current);

    for (int step = 1; step < n; ++step) {
        int best = -1;
        double best_dist = std::numeric_limits<double>::max();
        for (int j = 0; j < n; ++j) {
            if (!visited[j] && dist[current][j] < best_dist) {
                best_dist = dist[current][j];
                best = j;
            }
        }
        visited[best] = true;
        route.push_back(best);
        current = best;
    }

    return route;
}

double RouteOptimizer::totalRouteDistance(const std::vector<int>& route,
                                           const std::vector<std::vector<double>>& dist) {
    double total = 0.0;
    const int n = static_cast<int>(route.size());
    for (int i = 0; i < n; ++i) {
        int from = route[i];
        int to = route[(i + 1) % n];
        total += dist[from][to];
    }
    return total;
}

void RouteOptimizer::twoOptImprove(std::vector<int>& route,
                                    const std::vector<std::vector<double>>& dist) {
    const int n = static_cast<int>(route.size());
    if (n < 4) return; 

    bool improved = true;
    while (improved) {
        improved = false;

        for (int i = 1; i < n - 1; ++i) {
            for (int k = i + 1; k < n; ++k) {
                int a = route[i - 1];
                int b = route[i];
                int c = route[k];
                int d = route[(k + 1) % n];

                if (a == c || b == d) continue;

                double currentLen = dist[a][b] + dist[c][d];
                double newLen = dist[a][c] + dist[b][d];

                if (newLen + 1e-9 < currentLen) {
                    std::reverse(route.begin() + i, route.begin() + k + 1);
                    improved = true;
                }
            }
        }
    }
}

RouteResult RouteOptimizer::buildCircularRoute(const std::vector<Address>& addresses) {
    const int n = static_cast<int>(addresses.size());
    RouteResult result;

    if (n == 0) {
        return result;
    }
    if (n == 1) {
        result.order = {0};
        result.total_distance_km = 0.0;
        return result;
    }

    std::vector<std::vector<double>> dist(n, std::vector<double>(n, 0.0));
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            double d = haversineKm(addresses[i].latitude, addresses[i].longitude,
                                    addresses[j].latitude, addresses[j].longitude);
            dist[i][j] = d;
            dist[j][i] = d;
        }
    }

    std::vector<int> route = nearestNeighbor(dist, /*start_idx=*/0);

    twoOptImprove(route, dist);

    result.order = route;
    result.total_distance_km = totalRouteDistance(route, dist);
    return result;
}

} 
