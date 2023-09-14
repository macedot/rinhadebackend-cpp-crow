#pragma once

#include <cstdint>
#include <string>

#include <crow.h>

#include "pq_conn_pool.h"
#include "http_status.h"

auto selectCountPessoas(int64_t& count) -> int64_t
{
    auto instance = pq_conn_pool::instance();
    auto dbconn   = instance->burrow();

    try {
        pqxx::nontransaction work(*dbconn);
        pqxx::result         res(work.exec("SELECT COUNT(*) FROM pessoas;"));
        for (const auto& c : res) {
            count = c[0].as<int64_t>();
        }
    }
    catch (const std::exception& ex) {
        CROW_LOG_ERROR << __func__ << ": " << ex.what();
        count = -1;
    }

    instance->unburrow(dbconn);
    return (count >= 0) ? HTTP::to_uint(HTTPStatus::OK) : HTTP::to_uint(HTTPStatus::NotFound);
}

auto getCongatemPessoas() -> crow::response
{
    int64_t count    = 0;
    auto    response = selectCountPessoas(count);

    crow::json::wvalue result;
    result["count"] = count;
    return crow::response(response, result);
}
