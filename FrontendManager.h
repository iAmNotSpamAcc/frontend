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
#pragma execution_character_set("utf-8")

using json = nlohmann::json;


enum class Permission {
    ADMIN,
    USER,
    GUEST
};

struct User {
    int id;
    std::string name;
    std::string surname;
    std::string username;
    std::string email;
    Permission permission;
    std::vector<int> borrowedBooks; // Список ID книг, которые пользователь взял
};

struct RegistrationState {
    std::string name;
    std::string surname;
    std::string username;
    std::string email;
    int step;   // 0 - старт, 1 - имя, 2 - фамилия, 3 - никнейм, 4 - почта
};

struct RequestQueue {
    void confirmRequest();
    void denyRequest();
    int getRequestQueueSize();
    std::string getFirstRequestDetails();
};

struct Book {
    int id;
    std::string title;
    std::string author;
};


class FrontendManager {
public:
    void run();
    FrontendManager();

private:
    std::vector<Book> library;
    const std::string TOKEN = "8147709261:AAGDZuVoDVQOvWw412Ypo3dbrBbGcckLyug"; // Замените на ваш токен бота
    std::vector<User> users;
    std::queue< RequestQueue> queueRequest;
    std::unordered_map<std::string, RegistrationState> registrationStates;
    //size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    bool userExists(const std::vector<User>& users, int id);
    Permission getUserPermission(const std::vector<User>& users, int id);
    User createUser(int id, const std::string& name, const std::string& surname,
        const std::string& username, const std::string& email, Permission permission);
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
};
