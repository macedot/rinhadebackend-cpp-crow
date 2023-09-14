#include <iostream>
#include <chrono>
#include <string>
#include <thread>

#include <fmt/format.h>

#define UUID_SYSTEM_GENERATOR
#include "uuid.h"

#include "pq_conn_pool.h"
#include "app_config.h"
#include "ts_unordered_set.hpp"
#include "ts_queue.hpp"

#include <crow.h>

struct Pessoa {
    std::string id{};
    std::string apelido{};
    std::string nome{};
    std::string nascimento{};
    std::string stack{};
};

ts_unordered_set<std::string> cache_apelido;
ts_queue<Pessoa>              queue_pessoas;

std::string get_uuid()
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

auto add_pessoa(Pessoa& pessoa) -> int
{
    auto              instance  = pq_conn_pool::instance();
    auto              dbconn    = instance->burrow();
    const std::string insertsql = fmt::format(
      "INSERT INTO pessoas (id,apelido,nome,nascimento,stack) "
      "VALUES ('{}','{}','{}','{}','{}');",
      pessoa.id,
      pessoa.apelido,
      pessoa.nome,
      pessoa.nascimento,
      pessoa.stack);

    try {
        // Execute SQL query on PostgreSQL
        pqxx::work   work(*dbconn);
        pqxx::result res(work.exec(insertsql));
        work.commit();
    }
    catch (const std::exception& ex) {
        CROW_LOG_ERROR << __func__ << ": " << ex.what();
        pessoa.id.clear();
    }

    instance->unburrow(dbconn);
    return (!pessoa.id.empty()) ? HTTP::to_uint(HTTPStatus::Created)
                                : HTTP::to_uint(HTTPStatus::BadRequest);
}

auto map_result_pessoa(const pqxx::result::const_iterator& c) -> Pessoa
{
    Pessoa pessoa;
    pessoa.id         = c[0].as<std::string>();
    pessoa.apelido    = c[1].as<std::string>();
    pessoa.nome       = c[2].as<std::string>();
    pessoa.nascimento = c[3].as<std::string>();
    pessoa.stack      = c[4].as<std::string>();
    return pessoa;
}

auto get_pessoa(const std::string& id, Pessoa& pessoa) -> int
{
    auto              instance  = pq_conn_pool::instance();
    auto              dbconn    = instance->burrow();
    const std::string selectsql = fmt::format(
      "SELECT id,apelido,nome,nascimento,stack"
      " FROM pessoas"
      " WHERE id='{}'"
      " LIMIT 1",
      id);
    int nrow = -1;
    try {
        pqxx::nontransaction work(*dbconn);
        pqxx::result         res(work.exec(selectsql));
        for (const auto& c : res) {
            pessoa = map_result_pessoa(c);
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

auto get_pessoas(crow::json::wvalue& pessoas, const std::string& query) -> int
{
    auto              instance  = pq_conn_pool::instance();
    auto              dbconn    = instance->burrow();
    const std::string selectsql = fmt::format(
      "SELECT id,apelido,nome,nascimento,stack"
      " FROM pessoas"
      " WHERE searchable LIKE '%{}%'"
      " LIMIT 50",
      str_tolower(query));
    int nrow = -1;
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
                pessoas[nrow]["stack"] = split(stack);
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

auto contagem_pessoas(int64_t& total) -> int64_t
{
    auto instance = pq_conn_pool::instance();
    auto dbconn   = instance->burrow();
    try {
        pqxx::nontransaction work(*dbconn);
        pqxx::result         res(work.exec("SELECT COUNT(*) FROM pessoas;"));
        for (const auto& c : res) {
            total = c[0].as<int64_t>();
        }
    }
    catch (const std::exception& ex) {
        CROW_LOG_ERROR << __func__ << ": " << ex.what();
        total = -99;
    }
    instance->unburrow(dbconn);
    return (total >= 0) ? HTTP::to_uint(HTTPStatus::OK) : HTTP::to_uint(HTTPStatus::NotFound);
}

std::string GetEnvStr(const std::string& variable_name, const std::string& default_value)
{
    const char* value = std::getenv(variable_name.data());
    return value ? value : default_value;
}

int GetEnvUInt16(const std::string& variable_name, int default_value)
{
    const char* value = std::getenv(variable_name.data());
    return (value ? std::stoi(value) : default_value);
}

int main(void)
{
    const auto SERVER_PORT = GetEnvUInt16("SERVER_PORT", 3000);
    const auto SERVER_CONCURRENCY =
      GetEnvUInt16("SERVER_CONCURRENCY", std::thread::hardware_concurrency());
    const auto DATABASE_URL =
      GetEnvStr("DATABASE_URL", "postgresql://postgres:postgres@localhost:5432/postgres");

    auto instance = pq_conn_pool::init(DATABASE_URL, SERVER_CONCURRENCY);
    if (!instance) {
        std::cerr << "FATAL Unable to connect to db!" << std::endl;
        return -1;
    }

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() { return "Hello world!"; });

    CROW_ROUTE(app, "/contagem-pessoas")
    ([]() {
        int64_t   total    = 0;
        const int response = contagem_pessoas(total);
        if (response != 200) {
            return crow::response(response);
        }
        crow::json::wvalue result;
        result["total"] = total;
        return crow::response(response, result);
    });

    CROW_ROUTE(app, "/pessoas").methods("POST"_method, "GET"_method)([](const crow::request& req) {
        if (req.method == "POST"_method) {
            auto msg = crow::json::load(req.body);
            if (!msg) {
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
            }

            auto valida_param_str = [msg](const char* param) -> uint32_t {
                if (!msg.has(param) || msg[param].t() == crow::json::type::Null) {
                    return HTTP::to_uint(HTTPStatus::UnprocessableEntity);
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

            std::string stack{};
            if (msg.has("stack")) {
                if (msg["stack"].t() == crow::json::type::List) {
                    if (!join(msg["stack"], stack)) {
                        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
                    }
                }
                else if (msg["stack"].t() != crow::json::type::Null) {
                    return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
                }
            }

            Pessoa pessoa = {
                get_uuid(), msg["apelido"].s(), msg["nome"].s(), msg["nascimento"].s(), stack
            };

            if (cache_apelido.exists(pessoa.apelido)) {
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
            }

            //queue_pessoas.push(pessoa);
            const auto response = add_pessoa(pessoa);
            if (response != 201) {
                return crow::response(response);
            }

            cache_apelido.emplace(pessoa.apelido);

            auto res = crow::response(HTTP::to_uint(HTTPStatus::Created));
            res.set_header("Location", "http://localhost:9999/pessoas/" + pessoa.id);
            return res;
        }

        if (req.method == "GET"_method) {
            auto query = req.url_params.get("t");
            if (!query) {
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
            }

            crow::json::wvalue result;
            const auto         response = get_pessoas(result, query);
            if (response != 200) {
                return crow::response(response);
            }

            return crow::response(response, result);
        }

        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    });

    CROW_ROUTE(app, "/pessoas/<string>")
      .methods("GET"_method)([=](const crow::request& req, const std::string& id) {
          if (req.method != "GET"_method) {
              return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
          }
          if (id.empty()) {
              return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
          }

          Pessoa pessoa;
          int    response = get_pessoa(id, pessoa);
          if (response != 200) {
              return crow::response(response);
          }

          crow::json::wvalue result;
          result["id"]         = pessoa.id;
          result["apelido"]    = pessoa.apelido;
          result["nome"]       = pessoa.nome;
          result["nascimento"] = pessoa.nascimento;
          if (!pessoa.stack.empty()) {
              result["stack"] = split(pessoa.stack);
          }
          return crow::response(response, result);
      });

    app.loglevel(crow::LogLevel::Critical);

    // std::jthread proc_queue_pessoas([](std::stop_token stoken) {
    //     while (!(stoken.stop_requested())) {
    //         while (!queue_pessoas.empty()) {
    //             auto pessoa = queue_pessoas.pop();
    //             add_pessoa(pessoa);
    //         }
    //         std::this_thread::sleep_for(std::chrono::seconds(1));
    //     }
    // });

    try {
        std::cout << fmt::format(
          "API: START; SERVER_PORT={}; SERVER_CONCURRENCY={};\n", SERVER_PORT, SERVER_CONCURRENCY);
        app.port(SERVER_PORT).concurrency(SERVER_CONCURRENCY).run();
        std::cout << "API: STOP;\n";
    }
    catch (const std::exception& e) {
        std::cerr << "std::exception:" << e.what() << std::endl;
    }

    // proc_queue_pessoas.request_stop();
    // proc_queue_pessoas.join();

    pq_conn_pool::instance()->release_pool();
    std::cout << "API: DONE;\n";
}