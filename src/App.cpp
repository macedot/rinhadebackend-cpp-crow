#include <iostream>
#include <string>
#include <optional>

#include <crow.h>
#include <fmt/format.h>

#include "pq_conn_pool.h"
#include "db_config.h"
#include "uuid.h"

struct Pessoa {
    std::string                id{};
    std::string                apelido{};
    std::string                nome{};
    std::string                nascimento{};
    std::optional<std::string> stack = std::nullopt;

    std::string fields() const
    {
        std::string res = std::string{ "id,apelido,nome,nascimento" };
        if (stack)
            res += ",stack";
        return res;
    }

    std::string values() const
    {
        std::string res = fmt::format("'{}','{}','{}','{}'", id, apelido, nome, nascimento);
        if (stack)
            res += fmt::format(",'{}'", stack.value());
        return res;
    }
};

std::string get_uuid()
{
    std::string res = uuid::v4::UUID::New().String();
    return res;
}

// execute a transactional SQL
bool executeSQL(const std::string& sql)
{
    bool bsuccess = false;
    auto instance = pq_conn_pool::instance();
    auto dbconn   = instance->burrow();
    try {
        if (!dbconn->is_open()) {
            throw std::runtime_error("Open database failed");
        }

        pqxx::work   work(*dbconn);
        pqxx::result res(work.exec(sql));
        work.commit();

        if (res.affected_rows() > 0)
            bsuccess = true;
    }
    catch (const std::exception& ex) {
        //std::cerr << "executeSQL failed: " << ex.what() << std::endl;
        bsuccess = false;
    }
    instance->unburrow(dbconn);
    return bsuccess;
}

int add_pessoa(Pessoa& pessoa)
{
    auto              instance  = pq_conn_pool::instance();
    auto              dbconn    = instance->burrow();
    const std::string insertsql = fmt::format(
      "INSERT INTO pessoas ({}) "
      "VALUES ({}) RETURNING id;",
      pessoa.fields(),
      pessoa.values());
    try {
        // Execute SQL query on PostgreSQL
        pqxx::work   work(*dbconn);
        pqxx::result res(work.exec(insertsql));
        work.commit();
        // Getting the id of a newly inserted row (user data)
        for (const auto c : res) {
            pessoa.id.clear();
            pessoa.id = c[0].as<std::string>();
        }
    }
    catch (const std::exception& ex) {
        //std::cerr << "Insert failed: " << ex.what() << std::endl;
        pessoa.id.clear();
    }
    instance->unburrow(dbconn);
    return (!pessoa.id.empty()) ? HTTP::to_uint(HTTPStatus::Created)
                                : HTTP::to_uint(HTTPStatus::BadRequest);
}

int get_pessoa(const std::string& id, Pessoa& pessoa)
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
        // Execute SQL query on PostgreSQL
        pqxx::nontransaction work(*dbconn);
        pqxx::result         res(work.exec(selectsql));
        // Getting the id of a newly inserted row (user data)
        for (const auto c : res) {
            pessoa.id      = c[0].as<std::string>();
            pessoa.apelido = c[1].as<std::string>();
            pessoa.nome    = c[2].as<std::string>();
            if (!c[3].is_null()) {
                pessoa.nascimento = c[3].as<std::string>();
            }
            if (!c[4].is_null()) {
                pessoa.stack = c[4].as<std::string>();
            }
        }
        nrow = res.size();
    }
    catch (const std::exception& ex) {
        nrow = -1;
        //std::cerr << "Select failed: " << ex.what() << std::endl;
    }
    instance->unburrow(dbconn);
    return (nrow > 0) ? HTTP::to_uint(HTTPStatus::OK) : HTTP::to_uint(HTTPStatus::NotFound);
}

int64_t contagem_pessoas(int64_t& total)
{
    auto instance = pq_conn_pool::instance();
    auto dbconn   = instance->burrow();
    try {
        pqxx::nontransaction work(*dbconn);
        pqxx::result         res(work.exec("SELECT COUNT(*) FROM pessoas;"));
        for (const auto c : res) {
            total = c[0].as<int64_t>();
        }
        //std::cerr << "Found: " << total << std::endl;
    }
    catch (const std::exception& ex) {
        //std::cerr << "Select failed: " << ex.what() << std::endl;
        total = -99;
    }
    instance->unburrow(dbconn);
    return (total >= 0) ? HTTP::to_uint(HTTPStatus::OK) : HTTP::to_uint(HTTPStatus::NotFound);
}

auto join(const crow::json::rvalue& elems, std::optional<std::string>& stack) -> bool
{
    std::string res;
    if (elems.size() > 0) {
        if (elems[0].t() != crow::json::type::String)
            return false;
        res += elems[0].s();
        if (elems.size() > 1) {
            for (size_t i = 1; i < elems.size(); ++i) {
                if (elems[i].t() != crow::json::type::String)
                    return false;
                res += ",";
                res += elems[i].s();
            }
        }
    }
    stack = res;
    return true;
};

int main(void)
{
    executeSQL(R"(
        CREATE TABLE IF NOT EXISTS public.pessoas (
            id          uuid NOT NULL,
            apelido     text NOT NULL,
            nome        text NOT NULL,
            nascimento  text,
            stack       text
        );

        ALTER TABLE ONLY public.pessoas
            ADD CONSTRAINT pessoas_pkey PRIMARY KEY (id);

        CREATE UNIQUE INDEX pessoas_apelido_index ON public.pessoas
            USING btree (apelido);
    )");

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() { return "Hello world!"; });

    CROW_ROUTE(app, "/contagem-pessoas")
    ([]() {
        int64_t   total    = 0;
        const int response = contagem_pessoas(total);
        if (response != 200) // Not found
            return crow::response(response);
        crow::json::wvalue result;
        result["total"] = total;
        return crow::response(response, result); // Created
    });

    CROW_ROUTE(app, "/pessoas").methods("POST"_method)([](const crow::request& req) {
        if (req.method == "POST"_method) {
            auto msg = crow::json::load(req.body);
            if (!msg)
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));

            auto valida_param_str = [msg](const char* param) -> uint32_t {
                if (!msg.has(param) || msg[param].t() == crow::json::type::Null)
                    return HTTP::to_uint(HTTPStatus::UnprocessableEntity);
                if (msg[param].t() != crow::json::type::String)
                    return HTTP::to_uint(HTTPStatus::BadRequest);
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
                    if (!join(msg["stack"], stack))
                        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
                }
                else if (msg["stack"].t() != crow::json::type::Null) {
                    return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
                }
            }

            Pessoa pessoa = {
                get_uuid(), msg["apelido"].s(), msg["nome"].s(), msg["nascimento"].s(), stack
            };

            const int response = add_pessoa(pessoa);
            if (response != 201) // UnprocessableEntity
                return crow::response(response);

            auto res = crow::response(response); // Created
            res.set_header("Location", "http://localhost:9999/pessoas/" + pessoa.id);
            return res;
        }

        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    });

    CROW_ROUTE(app, "/pessoas/<string>")
      .methods("GET"_method)([=](const crow::request& req, const std::string& id) {
          if (req.method == "GET"_method) {
              if (id.empty()) {
                  return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
              }

              Pessoa pessoa;
              int    response = get_pessoa(id, pessoa);
              if (response != 200) // Not found
                  return crow::response(response);

              crow::json::wvalue result;
              result["id"]         = pessoa.id;
              result["apelido"]    = pessoa.apelido;
              result["nome"]       = pessoa.nome;
              result["nascimento"] = pessoa.nascimento;
              if (pessoa.stack)
                  result["stack"] = pessoa.stack.value();
              return crow::response(response, result);
          }
          return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
      });

    app.loglevel(crow::LogLevel::Critical);

    try {
        //app.port(9999).multithreaded().run();
        app.port(9999).concurrency(API_MAX_THREADS).run();
    }
    catch (const std::exception& e) {
        std::cerr << "std::exception:" << e.what() << std::endl;
    }

    pq_conn_pool::instance()->release_pool();
}