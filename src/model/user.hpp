#pragma once
#include "bucket.hpp"
#include <sqlite_orm/sqlite_orm.h>
#include <string>
namespace model {

struct User {
  int id;
  std::string name;
  std::string email;
  std::string password;

  template <class Storage> inline auto get_buckets(Storage storage) {
    using namespace sqlite_orm;
    return storage.template get_all<Bucket>(where(c(&Bucket::user_id) == id));
  }

  static inline auto make_table() {
    using namespace sqlite_orm;
    return sqlite_orm::make_table(
        "user", make_column("id", &User::id, primary_key().autoincrement()),
        make_column("name", &User::name), make_column("email", &User::email),
        make_column("password", &User::password));
  }
};

} // namespace model