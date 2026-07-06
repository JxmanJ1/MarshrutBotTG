#pragma once
#include <cstdint>
#include <tgbot/tgbot.h>
#include <string>
#include <vector>
#include "types.hpp"

namespace router_bot {


class BotHandlers {
public:
    explicit BotHandlers(TgBot::Bot& bot);


    void registerHandlers();

private:
    TgBot::Bot& bot_;

    void onStart(const TgBot::Message::Ptr& message);
    void onTextMessage(const TgBot::Message::Ptr& message);
    void onDocumentMessage(const TgBot::Message::Ptr& message);

    void processAddressList(std::int64_t chatId, const std::string& rawText);

    std::vector<Address> parseAddressLines(const std::string& rawText);

    std::string downloadDocumentText(const std::string& fileId);

    static std::string trim(const std::string& s);
};

} // namespace router_bot
