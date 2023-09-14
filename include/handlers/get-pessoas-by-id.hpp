#pragma once

#include <cstdint>
#include <string>

#include <fmt/format.h>

#include <crow.h>

#include "pq_conn_pool.h"
#include "http_status.h"
#include "utils.hpp"

auto selectPessoaById(crow::json::wvalue& pessoa, const std::string& id) -> int
{
    auto              instance  = pq_conn_pool::instance();
    auto              dbconn    = instance->burrow();
    int               nrow      = -1;
    const std::string selectsql = fmt::format(
      "SELECT id,apelido,nome,nascimento,stack"
      " FROM pessoas"
      " WHERE id='{}'"
      " LIMIT 1",
      id);

    try {
        pqxx::nontransaction work(*dbconn);
        pqxx::result         res(work.exec(selectsql));
        std::string          stack;
        for (const auto& c : res) {
            pessoa["id"]         = c[0].as<std::string>();
            pessoa["apelido"]    = c[1].as<std::string>();
            pessoa["nome"]       = c[2].as<std::string>();
            pessoa["nascimento"] = c[3].as<std::string>();
            stack                = c[4].as<std::string>();
            if (!stack.empty()) {
                pessoa["stack"] = utils::split(stack);
            }
        }
        nrow = res.size();
    }
    catch (const std::exception& ex) {
        CROW_LOG_ERROR << __func__ << ": " << ex.what();
        nrow = -1;
    }

    instance->unburrow(dbconn);
    return (nrow > 0) ? HTTP::to_uint(HTTPStatus::OK) : HTTP::to_uint(HTTPStatus::NotFound);
}

auto getPessoaById(const crow::request& req, const std::string& id) -> crow::response
{
    if (req.method != "GET"_method) {
        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    }
    if (id.empty()) {
        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    }

    crow::json::wvalue result;
    int                response = selectPessoaById(result, id);
    if (response != 200) {
        return crow::response(response);
    }

    return crow::response(response, result);
}
