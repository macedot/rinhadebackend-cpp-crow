#pragma once

#include <cstdint>
#include <string>

#include <fmt/format.h>

#include <crow.h>

#include "pq_conn_pool.h"
#include "http_status.h"
#include "utils.hpp"

auto addPessoaDB(const std::string& id,
                 const std::string& apelido,
                 const std::string& nome,
                 const std::string& nascimento,
                 const std::string& stack) -> int
{
    auto              instance  = pq_conn_pool::instance();
    auto              dbconn    = instance->burrow();
    bool              success   = false;
    const std::string insertsql = fmt::format(
      "INSERT INTO pessoas (id,apelido,nome,nascimento,stack) "
      "VALUES ('{}','{}','{}','{}','{}');",
      id,
      apelido,
      nome,
      nascimento,
      stack);

    try {
        pqxx::work   work(*dbconn);
        pqxx::result res(work.exec(insertsql));
        work.commit();
        success = true;
    }
    catch (const std::exception& ex) {
        CROW_LOG_CRITICAL << __func__ << ": " << ex.what();
    }

    instance->unburrow(dbconn);
    //FIXME: Crow dont have UnprocessableEntity return yet;
    return (success) ? HTTP::to_uint(HTTPStatus::Created) : HTTP::to_uint(HTTPStatus::BadRequest);
}

auto postPessoas(const crow::request& req) -> crow::response
{
    auto msg = crow::json::load(req.body);
    if (!msg) {
        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    }

    auto valida_param_str = [msg](const char* param) -> uint32_t {
        if (!msg.has(param) || msg[param].t() == crow::json::type::Null) {
            //return HTTP::to_uint(HTTPStatus::UnprocessableEntity);
            return HTTP::to_uint(HTTPStatus::BadRequest);
        }
        if (msg[param].t() != crow::json::type::String) {
            return HTTP::to_uint(HTTPStatus::BadRequest);
        }
        return 0;
    };

    if (auto res = valida_param_str("apelido"); res > 0) {
        return crow::response(res);
    }

    if (auto res = valida_param_str("nome"); res > 0) {
        return crow::response(res);
    }

    if (auto res = valida_param_str("nascimento"); res > 0) {
        return crow::response(res);
    }

    const auto& apelido = msg["apelido"].s();
    if (apelido.size() > 32) {
        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    }

    const auto& nome = msg["nome"].s();
    if (nome.size() > 100) {
        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    }

    const auto& nascimento = msg["nascimento"].s();
    if (nascimento.size() > 10) {
        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    }

    std::string stack{};
    if (msg.has("stack")) {
        if (msg["stack"].t() == crow::json::type::List) {
            if (!utils::join(msg["stack"], stack)) {
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
            }
        }
        else if (msg["stack"].t() != crow::json::type::Null) {
            return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
        }
    }

    const auto id       = utils::get_uuid();
    const auto response = addPessoaDB(id, apelido, nome, nascimento, stack);
    if (response != 201) {
        return crow::response(response);
    }

    auto res = crow::response(HTTP::to_uint(HTTPStatus::Created));
    res.set_header("Location", "http://localhost:9999/pessoas/" + id);
    return res;
}
