#pragma once

#include "../model/artifact.hpp"
#include "crow/logging.h"
#include "sqlite_orm/sqlite_orm.h"
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

namespace service {
template <typename S> class artifact {
  S &storage;

public:
  artifact(S &storage) : storage(storage) {}
  int insert(model::artifact &artifact) {
    artifact.created_at = artifact.updated_at =
        storage.select(sqlite_orm::datetime("now", "+2 hours")).front();
    return artifact.id = storage.template insert<model::artifact>(artifact);
  }

  void update(model::artifact &artifact) {
    artifact.updated_at =
        storage.select(sqlite_orm::datetime("now", "+2 hours")).front();
    storage.template update<model::artifact>(artifact);
  }

  std::optional<model::artifact> get_with_bucket_and_user(int id, int bucket_id,
                                                          int user_id) {
    using namespace sqlite_orm;
    try {
      auto data = storage.template get_all<model::artifact>(
          where(c(&model::artifact::id) == id and
                c(&model::artifact::bucket_id) ==
                    select(&model::bucket::id,
                           where(c(&model::bucket::id) == bucket_id and
                                 c(&model::bucket::user_id) == user_id),
                           limit(1))));
      if (data.size() == 1) {
        return std::move(data.front());
      }
    } catch (std::system_error &e) {
      CROW_LOG_ERROR << __FUNCTION__ << ": " << e.what();
    }
    return {};
  }

  void remove(const model::artifact &artifact) {
    using namespace sqlite_orm;
    if (std::filesystem::remove(artifact.filename)) {
      CROW_LOG_INFO << "Artifact file removed: " << artifact.filename;
    } else {
      CROW_LOG_ERROR << "Failed to remove artifact file: " << artifact.filename;
    }
    storage.template remove_all<model::artifact>(
        where(c(&model::artifact::id) == artifact.id));
  }

  std::vector<model::artifact> get_children(model::artifact &artifact) {
    using namespace sqlite_orm;
    return storage.template get_all<model::artifact>(
        where(c(&model::artifact::super) == artifact.id));
  }

  int add_child(const model::artifact &artifact,
                model::artifact &sub_artifact) {
    sub_artifact.super = artifact.id;
    update(sub_artifact);
    return sub_artifact.id;
  }
};
} // namespace service
