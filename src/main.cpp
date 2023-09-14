#include <iostream>
#include <chrono>
#include <string>

#include <fmt/format.h>

#define UUID_SYSTEM_GENERATOR
#include "uuid.h"

#include <crow.h>

#include "pq_conn_pool.h"
#include "http_status.h"

#include "handlers/get-contagem-pessoas.hpp"
#include "handlers/get-pessoas-by-id.hpp"
#include "handlers/get-pessoas-like-term.hpp"
#include "handlers/post-pessoas.hpp"

int main(void)
{
    const auto DATABASE_URL       = std::getenv("DATABASE_URL");
    const auto SERVER_PORT        = std::atoi(std::getenv("SERVER_PORT"));
    const auto SERVER_CONCURRENCY = std::atoi(std::getenv("SERVER_CONCURRENCY"));

    auto instance = pq_conn_pool::init(DATABASE_URL, SERVER_CONCURRENCY);
    if (!instance) {
        std::cerr << "FATAL Unable to connect to db!" << std::endl;
        return -1;
    }

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]() { return "Hello world!"; });
    CROW_ROUTE(app, "/contagem-pessoas")(getCongatemPessoas);
    CROW_ROUTE(app, "/pessoas").methods("POST"_method)(postPessoas);
    CROW_ROUTE(app, "/pessoas").methods("GET"_method)(getPessoasLikeTerm);
    CROW_ROUTE(app, "/pessoas/<string>").methods("GET"_method)(getPessoaById);

    app.loglevel(crow::LogLevel::Error);

    try {
        std::cout << fmt::format(
          "API: START; SERVER_PORT={}; SERVER_CONCURRENCY={};\n", SERVER_PORT, SERVER_CONCURRENCY);
        app.port(SERVER_PORT).concurrency(SERVER_CONCURRENCY).run();
        std::cout << "API: STOP;\n";
    }
    catch (const std::exception& e) {
        std::cerr << "std::exception:" << e.what() << std::endl;
    }

    pq_conn_pool::instance()->release_pool();
    std::cout << "API: DONE;\n";
}