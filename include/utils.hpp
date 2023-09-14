#pragma once

#include <iostream>
#include <chrono>
#include <string>

#define UUID_SYSTEM_GENERATOR
#include "uuid.h"

#include "pq_conn_pool.h"
#include "http_status.h"

#include <crow.h>

namespace utils {

auto get_uuid() -> std::string
{
    return uuids::to_string(uuids::uuid_system_generator{}());
}

auto join(const crow::json::rvalue& elems, std::string& stack) -> bool
{
    if (elems.size() > 0) {
        if (elems[0].t() != crow::json::type::String) {
            return false;
        }
        stack += elems[0].s();
        if (elems.size() > 1) {
            for (size_t i = 1; i < elems.size(); ++i) {
                if (elems[i].t() != crow::json::type::String) {
                    return false;
                }
                stack += ",";
                stack += elems[i].s();
            }
        }
    }
    return true;
};

auto split(const std::string& src) -> std::vector<std::string>
{
    std::vector<std::string> stack;
    std::stringstream        s_stream(src);
    std::string              substr;
    while (s_stream.good()) {
        getline(s_stream, substr, ',');
        stack.push_back(substr);
    }
    return stack;
}

std::string str_tolower(std::string s)
{
    std::transform(s.begin(),
                   s.end(),
                   s.begin(),
                   // static_cast<int(*)(int)>(std::tolower)         // wrong
                   // [](int c){ return std::tolower(c); }           // wrong
                   // [](char c){ return std::tolower(c); }          // wrong
                   [](unsigned char c) { return std::tolower(c); } // correct
    );
    return s;
}

}; //namespace utils;