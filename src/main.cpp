#include <crow.h>

#include "pq_conn_pool.h"

#include "handlers/get-contagem-pessoas.hpp"
#include "handlers/get-pessoas-by-id.hpp"
#include "handlers/get-pessoas-like-term.hpp"
#include "handlers/post-pessoas.hpp"

auto getEnvVar(const char* var) -> char*
{
    if (auto value = std::getenv(var)) {
        return value;
    }
    throw std::runtime_error(fmt::format("Missing environment variable: {}", var));
}

int main(void)
{
    const auto DATABASE_URL       = getEnvVar("DATABASE_URL");
    const auto SERVER_PORT        = std::atoi(getEnvVar("SERVER_PORT"));
    const auto SERVER_CONCURRENCY = std::atoi(getEnvVar("SERVER_CONCURRENCY"));

    auto instance = pq_conn_pool::init(DATABASE_URL, SERVER_CONCURRENCY);
    if (!instance) {
        std::cerr << "FATAL Unable to connect to db!" << std::endl;
        return -1;
    }

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]() { return "Hello world!"; });

    CROW_ROUTE(app, "/contagem-pessoas")([&]() { return getCongatemPessoas(); });

    CROW_ROUTE(app, "/pessoas").methods("GET"_method, "POST"_method)([&](const crow::request& req) {
        if (req.method == "POST"_method) {
            return postPessoas(req);
        }
        if (req.method == "GET"_method) {
            return getPessoasLikeTerm(req);
        }
        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    });

    CROW_ROUTE(app, "/pessoas/<string>")
      .methods("GET"_method)([&](const crow::request& req, const std::string& id) {
          if (req.method != "GET"_method) {
              return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
          }
          return getPessoaById(req, id);
      });

    app.loglevel(crow::LogLevel::Critical);

    const auto SERVER =
      fmt::format("SERVER(PORT={};CONCURRENCY={})", SERVER_PORT, SERVER_CONCURRENCY);

    try {
        std::cout << SERVER << ": START\n";
        app.port(SERVER_PORT).concurrency(SERVER_CONCURRENCY).run();
        std::cout << SERVER << ": STOP\n";
    }
    catch (const std::exception& e) {
        std::cerr << SERVER << ": Exception: " << e.what() << std::endl;
    }

    pq_conn_pool::instance()->release_pool();

    std::cout << SERVER << ": DONE\n";
}
