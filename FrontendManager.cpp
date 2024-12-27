#include "FrontendManager.hpp"


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

json FrontendManager::createAdminKeyboard() {
    return {
        {"inline_keyboard", {
            {
                {{"text", "Добавить книгу"}, {"callback_data", "add_book"}},
                {{"text", "Удалить книгу"}, {"callback_data", "remove_book"}}
            },
            {
                {{"text", "Состав библиотеки"}, {"callback_data", "bookshelf"}}
            },
            {
                {{"text", "Заявки"}, {"callback_data", "application"}}
            }
        }}
    };
}

json FrontendManager::createUserKeyboard() {
    return {
        {"inline_keyboard", {
            {
                {{"text", "Взять книгу"}, {"callback_data", "app_borrow"}},
                {{"text", "Сдать книгу"}, {"callback_data", "app_return"}}
            },
            {
                {{"text", "Мои книги"}, {"callback_data", "mybooks"}}
            }
        }}
    };
}

json FrontendManager::createGuestKeyboard() {
    return {
        {"inline_keyboard", {
            {
                {{"text", "Оставить заявку"}, {"callback_data", "app_reg"}}
            }
        }}
    };
}


void FrontendManager::sendMessage(const std::string& chat_id, const std::string& message, const json& reply_markup) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string url = "https://api.telegram.org/bot" + TOKEN + "/sendMessage";
    std::string payload = "chat_id=" + chat_id + "&text=" + message;
    if (!reply_markup.is_null()) {
        payload += "&reply_markup=" + reply_markup.dump();
    }

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Failed to send message: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
}

void FrontendManager::showBooks(const std::string& chat_id, int page, std::string action) {
    const int booksPerPage = 10;
    int totalBooks = 0;
    
    std::vector<Book*> library;
    
    if(action=="remove_book"){
        
        library=Library.getAllLibraryBooks();
        totalBooks = library.size();
    }else if(action=="app_borrow"){
        library=Library.getAvailableBooks();
        totalBooks = library.size();
    }else if(action=="app_return"){
        library=Library.getBooksToReturn(std::stoi(chat_id));
        totalBooks = library.size();
    }else if(action=="mybooks"){
        library=Library.getAllUserBooks(std::stoi(chat_id));
        totalBooks = library.size();
    }
    int totalPages = (totalBooks + booksPerPage - 1) / booksPerPage; // Общее количество страниц
    
    // Проверка на допустимость страницы
    if(totalBooks>0){
        if (page < 1) page = 1;
        if (page > totalPages) page = totalPages;
        std::string message = "Список книг (Страница " + std::to_string(page) + " из " + std::to_string(totalPages) + "):\n";
        // Создание клавиатуры для книг на текущей странице
        json inline_keyboard = json::array();
        for (int i = (page - 1) * booksPerPage; i < (((page * booksPerPage) < (totalBooks)) ? (page * booksPerPage) : (totalBooks)); i++) {
            // Создание кнопки для каждой книги
            json button = {
                {"text", library[i]->getAuthor() + " - " + library[i]->getTitle()},
                {"callback_data", action + "_selected_book_" + std::to_string(library[i]->getId())}
            };
            // Добавление кнопки в новый ряд
            json row = json::array();
            row.push_back(button);
            inline_keyboard.push_back(row);
        }
        
        // Если есть книги, добавляем их в клавиатуру
        if (!inline_keyboard.empty()) {
            json reply_markup = {
                {"inline_keyboard", inline_keyboard}
            };
            
            // Добавление кнопок для навигации между страницами
            json navigation_row = json::array();
            if (page > 1) {
                navigation_row.push_back({
                    {"text", "Назад"},
                    {"callback_data", action + "_books_page_" + std::to_string(page - 1)}
                });
            }
            if (page < totalPages) {
                navigation_row.push_back({
                    {"text", "Вперед"},
                    {"callback_data", action + "_books_page_" + std::to_string(page + 1)}
                });
            }
            // Добавление навигационного ряда только если он не пуст
            if (!navigation_row.empty()) {
                reply_markup["inline_keyboard"].push_back(navigation_row);
            }
            
            sendMessage(chat_id, message, reply_markup);
        }
        else {
            // Если нет книг на странице, отправляем сообщение об этом
            sendMessage(chat_id, "На этой странице нет книг.");
        }
    }else{
        sendMessage(chat_id, "Нахуй ты пришел, я тебя не звал, книг нема");
    }
}


    void FrontendManager::showMainMenu(const std::string& chat_id) {
        UserRole Role=UserRole::Non;
        
        if(Library.userExists(std::stoi(chat_id))){
           Role = Library.getUserById(std::stoi(chat_id))->getRole();
        }
        if (Role == UserRole::Admin) {
            sendMessage(chat_id, "Добро пожаловать, админ!", createAdminKeyboard());
        }
        else if (Role == UserRole::Reader) {
            sendMessage(chat_id, "Добро пожаловать, читатель!", createUserKeyboard());
        }
        else {
            sendMessage(chat_id, "Добро пожаловать, Гость! Вас пока нет в базе, оставьте заявку на регистрацию", createGuestKeyboard());
        }
    }


    void FrontendManager::handleApplications(const std::string& chat_id) {
        
        if (Library.getRequestQueueSize()>0) {
            std::string text;
            text=Library.getFirstRequestDetails();
            sendMessage(chat_id, "Заявка " +text);

json reply_markup = {
                {"inline_keyboard", {
                    {
                        {{"text", "Подтвердить"}, {"callback_data", "confirm_request"}},
                        {{"text", "Отклонить"}, {"callback_data", "reject_request"}}
                    }
                }}
            };
            sendMessage(chat_id, "Выберите действие:", reply_markup);
        }
        else {
            sendMessage(chat_id, "Заявок нет.");
        }
    }


    void FrontendManager::handleCallbackQuery(const json& callbackQuery) {
        if (!callbackQuery.contains("message") || !callbackQuery.contains("data")) {
            std::cerr << "Invalid callback query format!" << std::endl;
            return;
        }

        std::string chat_id = std::to_string(static_cast<long long>(callbackQuery["message"]["chat"]["id"]));
        std::string callback_data = callbackQuery["data"];

        std::cout << "Received callback data: " << callback_data << std::endl;

if (callback_data == "add_book") {
            sendMessage(chat_id, "Введите название книги и автора через ';' (например, 'Книга1;Автор1').");
        }
        else if (callback_data == "remove_book") {
            sendMessage(chat_id, "Выберите книгу для удаления:");
            showBooks(chat_id, 1, "remove_book");
        }
        else if (callback_data == "bookshelf") {
            sendMessage(chat_id, "Состав библиотеки.");
        }
          else if (callback_data == "app_borrow") {
              sendMessage(chat_id, "Выберите книгу для заявки взятия:");
              showBooks(chat_id, 1, "app_borrow");
          }
          else if (callback_data == "app_return") {
              showBooks(chat_id, 1, "app_return");
              sendMessage(chat_id, "Выберите книгу для заявки на возвращение:");
          }
          else if (callback_data == "mybooks") {
              sendMessage(chat_id, "Мои книги:");
              showBooks(chat_id, 1, "mybooks");
          }
          else if (callback_data == "app_reg") {
              sendMessage(chat_id, "Введите ваше имя:");
              registrationStates[chat_id] = RegistrationState{ .step = 1 };
          }
          else if (callback_data == "application") {
              handleApplications(chat_id);
          }
          else if (callback_data == "confirm_request") {
              Library.confirmRequest();
              sendMessage(chat_id, "Заявка подтверждена");
              handleApplications(chat_id);
          }
          else if (callback_data == "reject_request") {
              Library.denyRequest();
              sendMessage(chat_id, "Заявка отклонена");
              handleApplications(chat_id);
          }
          else if (callback_data.starts_with("remove_book_books_page_")) {
              int page = std::stoi(callback_data.substr(23)); // Извлечение номера страницы
              showBooks(chat_id, page, "remove_book"); // Показать книги на новой странице
          }
          else if (callback_data.starts_with("remove_book_selected_book_")) {
              int ID = std::stoi(callback_data.substr(26)); // Извлечение номера страницы
              sendMessage(chat_id, ("Выбрана книга." + std::to_string(ID)));
          }
          else if (callback_data.starts_with("app_borrow_books_page_")) {
              int page = std::stoi(callback_data.substr(22)); // Извлечение номера страницы
              showBooks(chat_id, page, "app_borrow"); // Показать книги на новой странице
          }
          else if (callback_data.starts_with("app_borrow_selected_book_")) {
              int ID = std::stoi(callback_data.substr(25)); // Извлечение номера страницы
              sendMessage(chat_id, ("Выбрана книга " + std::to_string(ID)));
              Library.borrowBookById(std::stoi(chat_id),ID);
          }
          else if (callback_data.starts_with("app_return_books_page_")) {
              int page = std::stoi(callback_data.substr(22)); // Извлечение номера страницы
              showBooks(chat_id, page, "app_return"); // Показать книги на новой странице
          }
          else if (callback_data.starts_with("app_return_selected_book_")) {
              int ID = std::stoi(callback_data.substr(25)); // Извлечение номера страницы
              sendMessage(chat_id, ("Выбрана книга." + std::to_string(ID)));
              Library.returnBookById(ID);
          }
          else {
              std::cerr << "Unknown callback data: " << callback_data << std::endl; // Логирование неизвестных данных
          }
      }

void FrontendManager::handleRegistration(const std::string& chat_id, const std::string& text) {
    auto it = registrationStates.find(chat_id);

if (it != registrationStates.end()) {
        RegistrationState& state = it->second;
        
switch (state.step) {
        case 1:
            state.name = text;
            sendMessage(chat_id, "Введите вашу фамилию:");
            state.step++;
            break;
        case 2:
            state.surname = text;
            sendMessage(chat_id, "Введите ваш никнейм:");
            state.step++;
            break;
        case 3:
            state.username = text;
            sendMessage(chat_id, "Введите вашу почту:");
            state.step++;
            break;
        case 4:
            state.email = text;
            Library.pushUserRequest(std::stoi(chat_id), state.name, state.surname, state.username, state.email);
            sendMessage(chat_id, "Спасибо за регистрацию! Заявка отправлена.");
            registrationStates.erase(chat_id);
            break;
        default:
            sendMessage(chat_id, "Что-то пошло не так. Попробуйте снова.");
            registrationStates.erase(chat_id);
            break;
        }
    }
    else if (text == "/start") {
        showMainMenu(chat_id);
    }
    else {
        sendMessage(chat_id, "Неизвестная команда.");
    }
}


void FrontendManager::getUpdates(int& last_update_id) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string url = "https://api.telegram.org/bot" + TOKEN + "/getUpdates?offset=" + std::to_string(last_update_id + 1);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }

    std::cout << "API Response: " << readBuffer << std::endl;

    try {
        json response = json::parse(readBuffer);
        if (response["ok"] && !response["result"].empty()) {
            for (const auto& result : response["result"]) {
                last_update_id = result["update_id"];

                if (result.contains("message") && result["message"].contains("text")) {
                    std::string text = result["message"]["text"];
                    std::string chat_id = std::to_string(static_cast<long long>(result["message"]["chat"]["id"]));
                    if (text == "/start") {
                        
                        
                        if (Library.userExists(static_cast<int>(result["message"]["chat"]["id"]))) {
                            UserRole Role=UserRole::Non;
                            Role = Library.getUserById(std::stoi(chat_id))->getRole();
                            
                            if (Role == UserRole::Admin) {
                                sendMessage(chat_id, "Добро пожаловать, админ!", createAdminKeyboard());
                            }
                            else if (Role == UserRole::Reader) {
                                sendMessage(chat_id, "Добро пожаловать, читатель!", createUserKeyboard());
                            }
                        }
                        else {
                            sendMessage(chat_id, "Добро пожаловать, Гость! Вас пока нет в базе, оставьте заявку на регистрацию", createGuestKeyboard());
                        }
                    }
                    else if (registrationStates.find(chat_id) != registrationStates.end()) {
                        handleRegistration(chat_id, text);
                    }
                }
                else if (result.contains("callback_query")) {
                    handleCallbackQuery(result["callback_query"]);
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
}

void FrontendManager::run() {
    int last_update_id = 0;

while (true) {
        getUpdates(last_update_id);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
