#pragma once

#include "../model/artifact.hpp"
#include "../model/bucket.hpp"
#include "crow/logging.h"
#include "sqlite_orm/sqlite_orm.h"
#include <optional>
#include <string>
#include <system_error>

namespace service {
template <typename S> class bucket {
  S &storage;

public:
  bucket(S &storage) : storage(storage) {}
  int insert(model::bucket &bucket) {
    bucket.created_at = bucket.updated_at =
        storage.select(sqlite_orm::datetime("now", "+2 hours")).front();
    return bucket.id = storage.template insert<model::bucket>(bucket);
  }

  void update(model::bucket &bucket) {
    bucket.updated_at =
        storage.select(sqlite_orm::datetime("now", "+2 hours")).front();
    storage.template update<model::bucket>(bucket);
  }

  std::optional<model::bucket> get_with_user(int id, int user_id) {
    using namespace sqlite_orm;
    try {
      auto data = storage.template get_all<model::bucket>(
          where(c(&model::bucket::id) == id and
                c(&model::bucket::user_id) == user_id));
      if (data.size() == 1) {
        return std::move(data.front());
      }
    } catch (std::system_error &e) {
      CROW_LOG_ERROR << __FUNCTION__ << ": " << e.what();
    }
    return {};
  }

  void remove(const model::bucket &bucket) {
    using namespace sqlite_orm;
    storage.transaction([&] mutable {
      try {
        storage.template remove_all<model::artifact>(
            where(c(&model::artifact::bucket_id) == bucket.id));
        storage.template remove_all<model::bucket>(
            where(c(&model::bucket::id) == bucket.id));
        return true;
      } catch (std::system_error &e) {
        return false;
      }
    });
  }

  std::vector<model::bucket> get_children(model::bucket &bucket) {
    using namespace sqlite_orm;
    return storage.template get_all<model::bucket>(
        where(c(&model::bucket::super) == bucket.id));
  }

  int add_child(const model::bucket &bucket, model::bucket &sub_bucket) {
    sub_bucket.super = bucket.id;
    update(sub_bucket);
    return sub_bucket.id;
  }
};
} // namespace service
