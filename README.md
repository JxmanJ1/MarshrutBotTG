# 🗺️ Telegram Route Bot

A Telegram bot that takes a list of addresses, geocodes them using OpenStreetMap Nominatim, builds an optimized circular route (TSP nearest-neighbor), and sends the result as both an Excel spreadsheet and a PDF report.

## Features

- 📍 **Address geocoding** via [Nominatim](https://nominatim.openstreetmap.org/) (OpenStreetMap)
- 🔁 **Circular route optimization** — starts and ends at the first address
- 📊 **Excel report** (`.xlsx`) with the ordered list of stops
- 📄 **PDF report** generated via `wkhtmltopdf`
- 📎 Accepts addresses as a **plain text message** or a `.txt` file upload
- 🏷️ Optional **point names** — separate name from address with a semicolon: `Warehouse; Zodchikh St 52, Kyiv`

## Tech Stack

| Component | Library |
|---|---|
| Telegram Bot API | [tgbot-cpp](https://github.com/reo7sp/tgbot-cpp) |
| HTTP requests | [libcurl](https://curl.se/libcurl/) |
| JSON parsing | [nlohmann/json](https://github.com/nlohmann/json) |
| Excel generation | [OpenXLSX](https://github.com/troldal/OpenXLSX) |
| PDF generation | [wkhtmltopdf](https://wkhtmltopdf.org/) |
| Build system | CMake + Ninja |
| Compiler | GCC (MSYS2 UCRT64) |

## Requirements

- Windows with [MSYS2](https://www.msys2.org/) (UCRT64 environment)
- [wkhtmltopdf](https://wkhtmltopdf.org/downloads.html) installed and available in `PATH`
- The following MSYS2 packages:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-curl \
          mingw-w64-ucrt-x86_64-nlohmann-json \
          mingw-w64-ucrt-x86_64-boost \
          git
```

- **tgbot-cpp** built and installed manually into the MSYS2 prefix:

```bash
git clone https://github.com/reo7sp/tgbot-cpp.git
cd tgbot-cpp && mkdir build && cd build
cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=/ucrt64
ninja && ninja install
```

## Building

```powershell
git clone https://github.com/your-username/telegram-route-bot.git
cd telegram-route-bot
mkdir build && cd build
cmake .. -G Ninja
ninja
```

## Running

Set your bot token as an environment variable and run:

```powershell
$env:TELEGRAM_BOT_TOKEN = "your_token_here"
.\build\route_bot.exe
```

To get a token, talk to [@BotFather](https://t.me/BotFather) on Telegram and create a new bot with `/newbot`.

## Usage

Send the bot a list of addresses — one per line:

```
Warehouse; Kyiv
NAME; ADRESS
```

The format is: `Point Name; Full Address` — the name part is optional. If no semicolon is present, the entire line is treated as an address.

The bot will:
1. Geocode all addresses
2. Build an optimized circular route starting from the first address
3. Send back an `.xlsx` and `.pdf` report with the ordered stops and total distance

## Project Structure

```
telegram-route-bot/
├── src/
│   ├── main.cpp            # Entry point, bot initialization
│   ├── bot_handlers.cpp    # Telegram message handlers, address parsing
│   ├── geocoder.cpp        # Nominatim geocoding via libcurl
│   ├── router.cpp          # TSP route optimization
│   └── file_generator.cpp  # Excel and PDF report generation
├── include/
│   ├── bot_handlers.hpp
│   ├── geocoder.hpp
│   ├── router.hpp
│   └── file_generator.hpp
└── CMakeLists.txt
```

## Notes

- Nominatim enforces a rate limit of 1 request per second — the bot adds a 1.2s delay between geocoding requests automatically.
- The bot token must never be hardcoded in source files. Always use the `TELEGRAM_BOT_TOKEN` environment variable.
- Addresses that cannot be geocoded are reported back to the user and excluded from the route.

## License

MIT
