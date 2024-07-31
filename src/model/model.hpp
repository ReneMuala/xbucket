#pragma once

#include <string>
#include "sqlite_orm/sqlite_orm.h"
#include "user.hpp"
#include "bucket.hpp"

namespace model {
    inline auto make_storage(const std::string & path = "chilldb.sqlite"){
        static auto did_storage_init = false;
        static auto storage = sqlite_orm::make_storage(path,
            user::make_table(),
            bucket::make_table()
        );

        if(!did_storage_init){
            storage.sync_schema();
            did_storage_init = true;
        }

        return storage;
    }
}
