#pragma once
#include "crow/json.h"
#include <optional>
#include <string>
#include <memory>
#include <sqlite_orm/sqlite_orm.h>
#include "../util/json.hpp"
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


        static inline model::user from_json(const crow::json::rvalue & json){
            return {
                .id = util::json::get_or<int>(json, "id", 0),
                .super = util::json::get_or<std::optional<decltype(model::user::id)>>(json, "super", {}),
                .name = util::json::get<std::string>(json, "name"),
                .email = util::json::get<std::string>(json, "email"),
                .password = util::json::get<std::string>(json, "password"),
            };
        }

        static inline auto make_table() {
            using namespace sqlite_orm;
            return sqlite_orm::make_table("user",
                make_column("name", &user::name),
                make_column("email", &user::email, unique()),
                make_column("password", &user::password),
                make_column("id", &user::id, primary_key().autoincrement()),
                make_column("created_at", &user::created_at, default_value(date("now"))),
                make_column("updated_at", &user::updated_at),
                make_column("super", &user::super)
            );
        }
    };
}
