#pragma once
#include "../model/user.hpp"
namespace service {
template <class Storage> class User {
public:
  Storage &storage;
  User(Storage &storage): storage(storage) {}
  inline auto get_all() { return storage.template get_all<model::User>(); }
};
}; // namespace service