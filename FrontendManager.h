#pragma once
#include <iostream>
#include <queue>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "Library.hpp"
#include "ElectronicBook.hpp"
#include "PhysicalBook.hpp"
#include "Book.hpp"
#include "UserManager.hpp"

using json = nlohmann::json;


enum class Permission {
    ADMIN,
    USER,
    GUEST
};


struct RegistrationState {
    std::string name;
    std::string surname;
    std::string username;
    std::string email;
    int step;   // 0 - старт, 1 - имя, 2 - фамилия, 3 - никнейм, 4 - почта
};


class FrontendManager {
public:
    FrontendManager(LibrarySystem& lib) : Library(lib) {}
    void run();

private:
    const std::string TOKEN = ""; // Замените на ваш токен бота
    std::unordered_map<std::string, RegistrationState> registrationStates;
    json createAdminKeyboard();
    json createUserKeyboard();
    json createGuestKeyboard();
    void sendMessage(const std::string& chat_id, const std::string& message, const json& reply_markup = nullptr);
    void showBooks(const std::string& chat_id, int page, std::string action = "");
    void showMainMenu(const std::string& chat_id);
    void handleApplications(const std::string& chat_id);
    void handleCallbackQuery(const json& callbackQuery);
    void handleRegistration(const std::string& chat_id, const std::string& text);
    void getUpdates(int& last_update_id);
    LibrarySystem& Library;
};
