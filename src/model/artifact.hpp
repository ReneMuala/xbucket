#pragma once
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <crow/json.h>
#include "../util/json.hpp"
#include "bucket.hpp"

namespace model {
struct artifact {
  int id;
  std::string name;
  std::string filename;
  std::string original_filename;
  decltype(model::bucket::id) bucket_id;
  std::optional<decltype(model::artifact::id)> super;
  std::string created_at;
  std::string updated_at;

  inline crow::json::wvalue to_json() const {
    return {
        {"id", id},
        {"super", super.value_or(0)},
        {"name", name},
        {"filename", filename},
        {"original_filename", original_filename},
        {"bucket_id", bucket_id},
        {"created_at", created_at},
        {"updated_at", updated_at},
    };
  }

static inline model::artifact from_json(const crow::json::rvalue &json) {
    return  model::artifact {
        .id = util::json::get_or<int>(json, "id", 0),
        .super = util::json::get_or<std::optional<decltype(model::artifact::id)>>(json, "super", {}),
        .name = util::json::get<std::string>(json, "name"),
        .filename = util::json::get<std::string>(json, "filename"),
        .original_filename = util::json::get<std::string>(json, "original_filename"),
        .bucket_id = util::json::get<int>(json, "bucket_id"),
    };
  }

  static inline auto make_table() {
    using namespace sqlite_orm;
    return sqlite_orm::make_table(
        "artifact", make_column("id", &artifact::id, primary_key().autoincrement()),
        make_column("name", &artifact::name),
        make_column("filename", &artifact::filename),
        make_column("original_filename", &artifact::original_filename),
        make_column("bucket_id", &artifact::bucket_id),
        make_column("super", &artifact::super),
        make_column("created_at", &artifact::created_at),
        make_column("updated_at", &artifact::updated_at),
        foreign_key(&artifact::bucket_id).references(&model::bucket::id)
    );
  }
};
} // namespace model
