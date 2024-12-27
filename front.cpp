#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "FrontendManager.h"
#pragma execution_character_set("utf-8")


int main() {
    FrontendManager frontend;
    frontend.run();

    return 0;
}