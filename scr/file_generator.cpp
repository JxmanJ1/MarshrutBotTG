#include "file_generator.hpp"
#include <OpenXLSX.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>

namespace router_bot {

using namespace OpenXLSX;

std::string FileGenerator::generateXlsx(const std::vector<Address>& ordered_addresses,
                                         const std::string& output_path) {
    try {
        XLDocument doc;
        // OpenXLSX::XLDocument::create accepts a single path argument; remove extra bool
        doc.create(output_path);

        XLWorksheet wks = doc.workbook().worksheet("Sheet1");
        wks.setName("Маршрут");

        // Заголовки таблиці - точно три колонки, як зазначено у вимогах
        wks.cell("A1").value() = "№";
        wks.cell("B1").value() = "Назва точки";
        wks.cell("C1").value() = "Адреса";

        // Заповнюємо рядки СУВОРО в порядку оптимізованого обходу маршруту
        int row = 2;
        for (size_t i = 0; i < ordered_addresses.size(); ++i) {
            const auto& addr = ordered_addresses[i];
            wks.cell(row, 1).value() = static_cast<int>(i + 1);
            wks.cell(row, 2).value() = addr.name.empty() ? std::string("-") : addr.name;
            wks.cell(row, 3).value() = addr.full_address;
            ++row;
        }

        // Ширина колонок підібрана під типовий вміст (назва компанії / повна адреса)
        wks.column(1).setWidth(6);
        wks.column(2).setWidth(30);
        wks.column(3).setWidth(55);

        doc.save();
        doc.close();
        return output_path;
    } catch (const std::exception& e) {
        std::cerr << "[FileGenerator] Помилка генерації XLSX: " << e.what() << std::endl;
        return "";
    }
}

std::string FileGenerator::escapeHtml(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '&':  result += "&amp;";  break;
            case '<':  result += "&lt;";   break;
            case '>':  result += "&gt;";   break;
            case '"':  result += "&quot;"; break;
            default:   result += c;
        }
    }
    return result;
}

std::string FileGenerator::buildHtmlTable(const std::vector<Address>& ordered_addresses) {
    std::ostringstream html;

    html << "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
         << "<style>"
         << "body { font-family: 'DejaVu Sans', 'Liberation Sans', Arial, sans-serif; }"
         << "h3 { margin-bottom: 12px; }"
         << "table { border-collapse: collapse; width: 100%; }"
         << "th, td { border: 1px solid #333333; padding: 6px 10px; "
         << "font-size: 12px; text-align: left; vertical-align: top; }"
         << "th { background-color: #e8e8e8; font-weight: bold; }"
         << "td.num { text-align: center; width: 5%; }"
         << "</style></head><body>"
         << "<h3>Оптимізований коловий маршрут</h3>"
         << "<table>"
         << "<tr><th>№</th><th>Назва точки</th><th>Адреса</th></tr>";

    for (size_t i = 0; i < ordered_addresses.size(); ++i) {
        const auto& addr = ordered_addresses[i];
        std::string name = addr.name.empty() ? "-" : addr.name;
        html << "<tr>"
             << "<td class='num'>" << (i + 1) << "</td>"
             << "<td>" << escapeHtml(name) << "</td>"
             << "<td>" << escapeHtml(addr.full_address) << "</td>"
             << "</tr>";
    }

    html << "</table></body></html>";
    return html.str();
}

namespace {

bool isCommandAvailable(const std::string& command) {
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv) {
        return false;
    }

    std::string pathValue = pathEnv;
    const char pathSeparator =
#ifdef _WIN32
        ';';
#else
        ':';
#endif

    std::vector<std::string> extensions = {""};
#ifdef _WIN32
    const char* pathext = std::getenv("PATHEXT");
    if (pathext) {
        std::string extList = pathext;
        size_t start = 0;
        while (start < extList.size()) {
            size_t end = extList.find(';', start);
            if (end == std::string::npos) end = extList.size();
            std::string ext = extList.substr(start, end - start);
            if (!ext.empty()) {
                extensions.push_back(ext);
            }
            start = end + 1;
        }
    } else {
        extensions = {".exe", ".bat", ".cmd", ".com"};
    }
#endif

    size_t start = 0;
    while (start < pathValue.size()) {
        size_t end = pathValue.find(pathSeparator, start);
        if (end == std::string::npos) end = pathValue.size();
        std::filesystem::path dir = pathValue.substr(start, end - start);
        for (const auto& ext : extensions) {
            std::filesystem::path filePath = dir / (command + ext);
            if (std::filesystem::exists(filePath)) {
                return true;
            }
        }
        start = end + 1;
    }
    return false;
}

} // namespace

std::string FileGenerator::generatePdf(const std::vector<Address>& ordered_addresses,
                                        const std::string& output_path) {
    if (!isCommandAvailable("wkhtmltopdf")) {
        std::cerr << "[FileGenerator] wkhtmltopdf не знайдено в PATH. Встановіть wkhtmltopdf." << std::endl;
        return "";
    }

    const std::filesystem::path htmlPath = std::filesystem::path(output_path).replace_extension(".tmp.html");
    {
        std::ofstream html_file(htmlPath, std::ios::binary);
        if (!html_file) {
            std::cerr << "[FileGenerator] Не вдалось створити тимчасовий HTML файл" << std::endl;
            return "";
        }
        std::string html_content = buildHtmlTable(ordered_addresses);
        html_file.write(html_content.data(), static_cast<std::streamsize>(html_content.size()));
    }

    std::ostringstream cmd;
    cmd << "wkhtmltopdf --encoding utf-8 --quiet "
        << '"' << htmlPath.string() << '"' << ' ' << '"' << output_path << '"';

    int ret = std::system(cmd.str().c_str());

    std::filesystem::remove(htmlPath);

    if (ret != 0) {
        std::cerr << "[FileGenerator] wkhtmltopdf завершився з кодом помилки: " << ret << std::endl;
        return "";
    }

    return output_path;
}


}
