#pragma once
#include "../util/json.hpp"
#include "crow/json.h"
#include "crow/logging.h"
#include <any>
#include <memory>
#include <optional>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <system_error>
namespace model {
struct user {
  int id;
  std::optional<decltype(model::user::id)> super;
  std::string name;
  std::string email;
  std::string password;
  std::string created_at;
  std::string updated_at;

  inline crow::json::wvalue to_json() const {
    return {
        {"id", id},
        {"name", name},
        {"email", email},
    };
  }

  static inline crow::json::wvalue to_json_sample() {
    static auto sample =
        from_json(crow::json::load(from_json_sample().dump())).to_json();
    return sample;
  }

  static inline crow::json::wvalue from_json_sample() {
    static auto wsample = crow::json::wvalue{
        {"name", "string"}, {"email", "string"}, {"password", "string"}};
    static auto sample = crow::json::load(wsample.dump());
    static bool tested = false;

    if (not tested) {
      tested = true;
      try {
        /// test the sample
        static auto _ = model::user::from_json(sample);
      } catch (std::system_error &e) {
        CROW_LOG_CRITICAL << "sample test failed: " << __FUNCTION__;
      }
    }
    return sample;
  }

  static inline model::user from_json(const crow::json::rvalue &json) {
    return {
        .id = util::json::get_or<int>(json, "id", 0),
        .super = util::json::get_or<std::optional<decltype(model::user::id)>>(
            json, "super", {}),
        .name = util::json::get<std::string>(json, "name"),
        .email = util::json::get<std::string>(json, "email"),
        .password = util::json::get<std::string>(json, "password"),
    };
  }

  static inline auto make_table() {
    using namespace sqlite_orm;
    return sqlite_orm::make_table(
        "user", make_column("name", &user::name),
        make_column("email", &user::email, unique()),
        make_column("password", &user::password),
        make_column("id", &user::id, primary_key().autoincrement()),
        make_column("created_at", &user::created_at,
                    default_value(date("now"))),
        make_column("updated_at", &user::updated_at),
        make_column("super", &user::super));
  }
};
} // namespace model
