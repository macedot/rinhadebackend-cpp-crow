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
    std::optional<std::string> nascimento = std::nullopt;
    std::optional<std::string> stack      = std::nullopt;

    std::string fields() const
    {
        std::string res = std::string{ "id,apelido,nome" };
        if (nascimento)
            res += ",nascimento";
        if (stack)
            res += ",stack";
        return res;
    }

    std::string values() const
    {
        std::string res = fmt::format("'{}','{}','{}'", id, apelido, nome);
        if (nascimento)
            res += fmt::format(",'{}'", nascimento.value());
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
                                : HTTP::to_uint(HTTPStatus::NotFound);
}

int get_pessoa(const std::string& id, Pessoa& pessoa)
{
    auto              instance  = pq_conn_pool::instance();
    auto              dbconn    = instance->burrow();
    const std::string selectsql = fmt::format(
      "SELECT id,apelido,nome,nascimento,stack"
      " FROM pessoas"
      " WHERE ID='{}'"
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

//int get_users(std::list<Pessoa>& lst_user)
//{
//    auto        instance  = pq_conn_pool::instance();
//    auto        dbconn    = instance->burrow();
//    std::string selectsql = "select * from players";
//    int         nrow      = -1;
//    try {
//        // Execute SQL query on PostgreSQL
//        pqxx::nontransaction work(*dbconn);
//        pqxx::result         res(work.exec(selectsql));
//        // Getting the id of a newly inserted row (user data)
//        for (pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
//            player user;
//            user.id       = c[0].as<int>();
//            user.username = c[1].as<std::string>();
//            user.password = c[2].as<std::string>();
//            user.userdata = c[3].as<std::string>();
//            lst_user.push_back(user);
//        }
//        nrow = res.size();
//    }
//    catch (const std::exception& ex) {
//        nrow = -1;
//        std::cerr << "Select failed: " << ex.what() << std::endl;
//    }
//    instance->unburrow(dbconn);
//    return (nrow > 0) ? HTTP::to_uint(HTTPStatus::OK) : HTTP::to_uint(HTTPStatus::NotFound);
//}

//int update_user(const int id, const std::string username, const std::string password, const std::string userdata)
//{
//    const std::string updatesql = "Update players set username = '" + username + "', password = '" + password + "', userdata = '" + userdata + "' WHERE id = " + std::to_string(id) + ";";
//    // 200: successful update 404: not found
//    return (executeSQL(updatesql)) ? HTTP::to_uint(HTTPStatus::OK) : HTTP::to_uint(HTTPStatus::NotFound);
//}

//int delete_user(const int id)
//{
//    std::string deletesql = "DELETE from players where ID = " + std::to_string(id);
//    // 200: successful update 404: not found
//    return (executeSQL(deletesql)) ? HTTP::to_uint(HTTPStatus::OK) : HTTP::to_uint(HTTPStatus::NotFound);
//}

int main(void)
{
    constexpr auto sql      = R"(
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
    )";
    bool           is_ready = executeSQL(sql);

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([is_ready]() {
        //if (is_ready)
        //    return "Hello world!";
        //else
        //    return "Connection to PostgreSQL database failed.";
        return "Hello world!";
    });

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

    //auto join = [](crow::json::wvalue& elems) -> std::string {
    //    std::string res;

    //    if (elems.size() > 0) {
    //        res += elems[0].as;
    //    }

    //    return res;
    //};

    CROW_ROUTE(app, "/pessoas").methods("POST"_method)([](const crow::request& req) {
        if (req.method == "POST"_method) {
            auto msg = crow::json::load(req.body);
            if (!msg)
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));

            // Bad request : one of JSON data was not in the correct format
            if (!msg.has("apelido") || msg["apelido"].t() != crow::json::type::String)
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));

            if (!msg.has("nome") || msg["nome"].t() != crow::json::type::String)
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));

            if (msg.has("nascimento") && msg["nascimento"].t() != crow::json::type::String &&
                msg["nascimento"].t() != crow::json::type::Null)
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));

            if (msg.has("stack") && msg["stack"].t() != crow::json::type::List &&
                msg["stack"].t() != crow::json::type::Null)
                return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));

            std::optional<std::string> nascimento;
            if (msg.has("nascimento") && msg["nascimento"].t() != crow::json::type::Null) {
                nascimento = msg["nascimento"].s();
            }

            std::optional<std::string> stack;
            //if (msg.has("stack")) {
            //    nascimento = msg["stack"].s();
            //}

            Pessoa pessoa = { get_uuid(), msg["apelido"].s(), msg["nome"].s(), nascimento, stack };

            const int response = add_pessoa(pessoa);
            if (response != 201) // Not found
                return crow::response(response);

            crow::json::wvalue result = msg;
            result["id"]              = pessoa.id;
            auto res                  = crow::response(response, result); // Created

            res.set_header("Location", "http://localhost:9999/pessoas/" + pessoa.id);
            return res;
        }

        //else if (req.method == "GET"_method) {
        //    std::list<Pessoa> lst_users;
        //    int               response = get_users(lst_users);
        //    if (response != 200) // Not found
        //        return crow::response(response);
        //    crow::json::wvalue result;
        //    unsigned int       index = 0;
        //    for (auto iter = lst_users.begin(); iter != lst_users.end(); iter++, index++) {
        //        result[index]["id"]       = iter->id;
        //        result[index]["username"] = iter->username;
        //        result[index]["password"] = iter->password;
        //        result[index]["userdata"] = iter->userdata;
        //    }
        //    return crow::response(response, result);
        //}

        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
    });

    CROW_ROUTE(app, "/pessoas/<string>")
      .methods("GET"_method)([=](const crow::request& req, const std::string& id) {
          //if (req.method == "DELETE"_method) {
          //    return crow::response(delete_user(id));
          //}

          //if (req.method == "PUT"_method) {
          //    auto msg = crow::json::load(req.body);
          //    // Bad request : requested data is not JSON
          //    if (!msg)
          //        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
          //    // Bad request : one of JSON data was not in the correct format
          //    if (!msg.has("username") || msg["username"].t() != crow::json::type::String)
          //        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
          //    if (!msg.has("password") || msg["password"].t() != crow::json::type::String)
          //        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
          //    if (!msg.has("userdata") || msg["userdata"].t() != crow::json::type::String)
          //        return crow::response(HTTP::to_uint(HTTPStatus::BadRequest));
          //    // Update a user data in PostgreSQ
          //    return crow::response(update_user(id, msg["username"].s(), msg["password"].s(), msg["userdata"].s()));
          //}

          if (req.method == "GET"_method)
          {
              Pessoa pessoa;
              int    response = get_pessoa(id, pessoa);
              if (response != 200) // Not found
                  return crow::response(response);

              crow::json::wvalue result;
              result["id"]      = pessoa.id;
              result["apelido"] = pessoa.apelido;
              result["nome"]    = pessoa.nome;
              if (pessoa.nascimento)
                  result["nascimento"] = pessoa.nascimento.value();
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