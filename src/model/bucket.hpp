#pragma once
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <crow/json.h>
#include "../util/json.hpp"
namespace model {
struct bucket {
  int id;
  std::string name;
  std::string description;
  int user_id;
  std::optional<decltype(model::bucket::id)> super;
  std::string created_at;
  std::string updated_at;

  inline crow::json::wvalue to_json() const {
    return {
        {"id", id},
        {"super", super.value_or(0)},
        {"name", name},
        {"description", description},
        {"user_id", user_id},
        {"created_at", created_at},
        {"updated_at", updated_at},
    };
  }

static inline model::bucket from_json(const crow::json::rvalue &json) {
    return  model::bucket {
        .id = util::json::get_or<int>(json, "id", 0),
        .super = util::json::get_or<std::optional<decltype(model::bucket::id)>>(json, "super", {}),
        .name = util::json::get<std::string>(json, "name"),
        .description = util::json::get<std::string>(json, "description"),
        .user_id = util::json::get<int>(json, "user_id"),
    };
  }

  static inline auto make_table() {
    using namespace sqlite_orm;
    return sqlite_orm::make_table(
        "bucket", make_column("id", &bucket::id, primary_key().autoincrement()),
        make_column("name", &bucket::name),
        make_column("description", &bucket::description),
        make_column("user_id", &bucket::user_id),
        make_column("super", &bucket::super),
        make_column("created_at", &bucket::created_at),
        make_column("updated_at", &bucket::updated_at));
  }
};
} // namespace model
