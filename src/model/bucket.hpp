#pragma once
#include <sqlite_orm/sqlite_orm.h>
#include <string>


namespace model {
struct Bucket {
  int id;
  std::string name;
  std::string folder;
  std::string description;
  int user_id;

  static inline auto make_table() {
    using namespace sqlite_orm;
    return sqlite_orm::make_table(
        "bucket", make_column("id", &Bucket::id, primary_key().autoincrement()),
        make_column("name", &Bucket::name),
        make_column("folder", &Bucket::folder),
        make_column("description", &Bucket::description),
        make_column("user_id", &Bucket::user_id));
  }
};
} // namespace model