#include <iostream>
#include <string>
#include <optional>

#include <crow.h>

#include <fmt/format.h>

#include "pq_conn_pool.h"
#include "app_config.h"

constexpr auto API_MAX_THREADS = 50;

struct Pessoa {
    std::string                id{};
    std::string                apelido{};
    std::string                nome{};
    std::string                nascimento{};
    std::optional<std::string> stack = std::nullopt;

    std::string insert_fields() const
    {
        std::string res = std::string{ "apelido,nome,nascimento" };
        if (stack) {
            res += ",stack";
        }
        return res;
    }

    std::string insert_values() const
    {
        std::string res = fmt::format("'{}','{}','{}'", apelido, nome, nascimento);
        if (stack) {
            res += fmt::format(",'{}'", stack.value());
        }
        return res;
    }
};

auto join(const crow::json::rvalue& elems, std::optional<std::string>& stack) -> bool
{
    std::string res;
    if (elems.size() > 0) {
        if (elems[0].t() != crow::json::type::String) {
            return false;
        }
        res += elems[0].s();
        if (elems.size() > 1) {
            for (size_t i = 1; i < elems.size(); ++i) {
                if (elems[i].t() != crow::json::type::String) {
                    return false;
                }
                res += ",";
                res += elems[i].s();
            }
        }
    }
    stack = res;
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
      "INSERT INTO pessoas ({}) "
      "VALUES ({}) RETURNING id;",
      pessoa.insert_fields(),
      pessoa.insert_values());
    try {
        // Execute SQL query on PostgreSQL
        pqxx::work   work(*dbconn);
        pqxx::result res(work.exec(insertsql));
        work.commit();
        // Getting the id of a newly inserted row (user data)
        for (const auto& c : res) {
            pessoa.id = c[0].as<std::string>();
        }
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
    if (!c[4].is_null()) {
        pessoa.stack = c[4].as<std::string>();
    }
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

auto get_pessoas(std::list<Pessoa>& pessoas, const std::string& query) -> int
{
    auto              instance  = pq_conn_pool::instance();
    auto              dbconn    = instance->burrow();
    const std::string selectsql = fmt::format(
      "SELECT id,apelido,nome,nascimento,stack"
      " FROM pessoas"
      " WHERE searchable ILIKE '%{}%'"
      " LIMIT 50",
      query);
    int nrow = -1;
    try {
        pqxx::nontransaction work(*dbconn);
        pqxx::result         res(work.exec(selectsql));
        for (const auto& c : res) {
            pessoas.push_back(map_result_pessoa(c));
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

// https://stackoverflow.com/questions/524591/performance-of-creating-a-c-stdstring-from-an-input-iterator/524843#524843
auto readFile2(const std::string& fileName) -> std::string
{
    std::ifstream ifs(fileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    const std::ifstream::pos_type fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<char> bytes(fileSize);
    ifs.read(bytes.data(), fileSize);
    return std::string(bytes.data(), fileSize);
}

int main(void)
{
    auto instance = pq_conn_pool::init(readFile2("db.string"), API_MAX_THREADS);
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

            std::optional<std::string> stack;
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
                "", msg["apelido"].s(), msg["nome"].s(), msg["nascimento"].s(), stack
            };

            const int response = add_pessoa(pessoa);
            if (response != 201) {
                // UnprocessableEntity
                return crow::response(response);
            }

            auto res = crow::response(response); // Created
            res.set_header("Location", "http://localhost:9999/pessoas/" + pessoa.id);
            return res;
        }

        if (req.method == "GET"_method) {
            auto query = req.url_params.get("t");
            if (!query) {
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
            }

            std::list<Pessoa> pessoas;
            const auto        response = get_pessoas(pessoas, query);
            if (response != 200) {
                return crow::response(response);
            }

            crow::json::wvalue result;
            unsigned int       index = 0;
            for (const auto& pessoa : pessoas) {
                result[index]["id"]         = pessoa.id;
                result[index]["apelido"]    = pessoa.apelido;
                result[index]["nome"]       = pessoa.nome;
                result[index]["nascimento"] = pessoa.nascimento;
                if (pessoa.stack) {
                    result[index]["stack"] = split(pessoa.stack.value());
                }
                ++index;
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
          if (pessoa.stack) {
              result["stack"] = split(pessoa.stack.value());
          }
          return crow::response(response, result);
      });

    app.loglevel(crow::LogLevel::Critical);

    try {
        std::cout << "Crow: START\n";
        app.port(3000).concurrency(API_MAX_THREADS).run();
        std::cout << "Crow: STOP\n";
    }
    catch (const std::exception& e) {
        std::cerr << "std::exception:" << e.what() << std::endl;
    }

    pq_conn_pool::instance()->release_pool();
}