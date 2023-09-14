#pragma once

#include <cstdint>
#include <string>

#include <fmt/format.h>

#include <crow.h>

#include "pq_conn_pool.h"
#include "http_status.h"
#include "utils.hpp"

auto selectPessoasLikeTerm(crow::json::wvalue& pessoas, const std::string& query) -> int
{
    auto              instance  = pq_conn_pool::instance();
    auto              dbconn    = instance->burrow();
    int               nrow      = -1;
    const std::string selectsql = fmt::format(
      "SELECT id,apelido,nome,nascimento,stack"
      " FROM pessoas"
      " WHERE searchable LIKE '%{}%'"
      " LIMIT 50",
      utils::str_tolower(query));

    try {
        pqxx::nontransaction work(*dbconn);
        pqxx::result         res(work.exec(selectsql));
        std::string          stack;
        for (const auto& c : res) {
            ++nrow;
            pessoas[nrow]["id"]         = c[0].as<std::string>();
            pessoas[nrow]["apelido"]    = c[1].as<std::string>();
            pessoas[nrow]["nome"]       = c[2].as<std::string>();
            pessoas[nrow]["nascimento"] = c[3].as<std::string>();
            stack                       = c[4].as<std::string>();
            if (!stack.empty()) {
                pessoas[nrow]["stack"] = utils::split(stack);
            }
        }
    }
    catch (const std::exception& ex) {
        CROW_LOG_ERROR << __func__ << ": " << ex.what();
        nrow = -1;
    }

    instance->unburrow(dbconn);
    return (nrow >= 0) ? HTTP::to_uint(HTTPStatus::OK) : HTTP::to_uint(HTTPStatus::NotFound);
}

auto getPessoasLikeTerm(const crow::request& req) -> crow::response
{
    auto term = req.url_params.get("t");
    if (!term) {
        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    }

    crow::json::wvalue result;
    const auto         response = selectPessoasLikeTerm(result, term);
    if (response != 200) {
        return crow::response(response);
    }

    return crow::response(response, result);
}
