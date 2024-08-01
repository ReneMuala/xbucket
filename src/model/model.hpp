#pragma once

#include "../constants/filesystem.hpp"
#include "artifact.hpp"
#include "bucket.hpp"
#include "sqlite_orm/sqlite_orm.h"
#include "user.hpp"
#include <string>

namespace model {

inline auto
get_storage(const std::string &path = constants::filesystem::xbucket_db_name) {

  using namespace sqlite_orm;
  static auto did_storage_init = false;
  static auto storage = make_storage(
      constants::filesystem::xbucket_db_dir + path, user::make_table(),
      bucket::make_table(), artifact::make_table());
  if (!did_storage_init) {
    storage.sync_schema();
    did_storage_init = true;
  }

  return storage;
}
} // namespace model
