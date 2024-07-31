#pragma once

#include <iterator>
#include <string>
#include <system_error>
#include "../model/user.hpp"
#include "../model/bucket.hpp"
#include "crow/logging.h"
#include "sqlite_orm/sqlite_orm.h"
#include <optional>

namespace service {
    template <typename S>
    class user {
        S & storage;
        public:
        user(S & storage) : storage(storage) {}
        int insert(model::user & user) {
            user.created_at = user.updated_at = storage.select(sqlite_orm::datetime("now", "+2 hours")).front();
            return user.id = storage.template insert<model::user>(user);
        }

        void update(model::user & user) {
            user.updated_at = storage.select(sqlite_orm::datetime("now", "+2 hours")).front();
            storage.template update<model::user>(user);
        }

        std::optional<model::user> get(int id) {
            try {
                return std::move(storage.template get<model::user>(id));
            } catch (std::system_error & e) {
                return {};
            }
        }

        std::vector<model::user> get_children(model::user & user){
            using namespace sqlite_orm;
            return storage.template get_all<model::user>(where(c(&model::user::super) == user.id));
        }

        std::optional<model::user> get_login(const std::string & email, const std::string & password) {
            using namespace sqlite_orm;

            try {
                auto data = storage.template get_all<model::user>(where(c(&model::user::email) == email and c(&model::user::password) == password));
                if(data.size() == 1){
                    return std::move(data.front());
                }
            } catch (std::system_error & e) {
                CROW_LOG_ERROR << __FUNCTION__ << ": " << e.what();
            }
            return {};
        }

        void remove(const model::user & user) {
            using namespace sqlite_orm;
            storage.begin_transaction();
            try {
                storage.template remove_all<model::bucket>(where(c(&model::bucket::user_id) == user.id));
                storage.template remove_all<model::user>(where(c(&model::user::id) == user.id));
                storage.commit();
            } catch(std::system_error & e){
                storage.rollback();
            }
        }

        int add_child(const model::user & user, model::user & sub_user){
            sub_user.super = user.id;
            update(sub_user);
            return sub_user.id;
        }
    };
}
