#include <tgbot/tgbot.h>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <string>

#include "bot_handlers.hpp"

namespace {

void signalHandler(int) {
    std::cout << "\nЗупинка бота..." << std::endl;
    std::exit(0);
}

}

int main() {

    const char* tokenEnv = std::getenv("TELEGRAM_BOT_TOKEN");
    if (!tokenEnv) {
        std::cerr << "Помилка: не встановлено змінну середовища TELEGRAM_BOT_TOKEN" << std::endl;
        return 1;
    }
    const std::string token = tokenEnv;

    TgBot::Bot bot(token);
    std::signal(SIGINT, signalHandler);

    router_bot::BotHandlers handlers(bot);
    handlers.registerHandlers();

    try {
        std::cout << "Бот запущено: @" << bot.getApi().getMe()->username << std::endl;

        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            std::cout << "Очікування оновлень..." << std::endl;
            longPoll.start();
        }
    } catch (const TgBot::TgException& e) {
        std::cerr << "Помилка API: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Помилка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
