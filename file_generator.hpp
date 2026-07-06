#pragma once
#include "types.hpp"
#include <vector>
#include <string>

namespace router_bot {

class FileGenerator {
public:

    static std::string generateXlsx(const std::vector<Address>& ordered_addresses,
                                     const std::string& output_path);

    static std::string generatePdf(const std::vector<Address>& ordered_addresses,
                                    const std::string& output_path);

private:

    static std::string buildHtmlTable(const std::vector<Address>& ordered_addresses);


    static std::string escapeHtml(const std::string& text);
};

} // namespace router_bot
