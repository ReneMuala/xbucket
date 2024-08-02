#pragma once
#include "../middleware/auth.hpp"
#include "../service/user.hpp"
#include "controller.internal.hpp"
#include "crow/common.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/json.h"
#include "crow/utility.h"
#include <crow/app.h>
#include <optional>
#include <stdexcept>

namespace controller {
using Session = crow::SessionMiddleware<crow::FileStore>;
template <typename S, typename... M> class user : public controller {
  crow::Crow<M...> &app;
  service::user<S> &service;

public:
  user(crow::Crow<M...> &app, service::user<S> &service)
      : service(service), app(app) {
    controller_register_api_route_auth_io(
        user, "read", EMPTY, "Read current user", crow::HTTPMethod::Get, read, {}, model::user::to_json_sample());
    controller_register_api_route_io(user, "create", EMPTY, "Create a new user",
                                  crow::HTTPMethod::Post, create, model::user::from_json_sample(), model::user::to_json_sample());
    controller_register_api_route_auth_io(user, "update", EMPTY, "Update current user",
                                  "PUT"_method, update, model::user::from_json_sample(), model::user::to_json_sample());
    controller_register_api_route_auth(user, "delete", EMPTY, "Delete current user", "DELETE"_method,remove);
  }

  crow::response create(const crow::request &req) {
    crow::json::rvalue body = crow::json::load(req.body);
    auto user = model::user::from_json(body);
    auto id = service.insert(user);
    return crow::response{user.to_json()};
  }

  crow::response update(const crow::request &req){
    if (auto user = service.get(app.template get_context<Session>(req).get("id", -1))){
        auto new_user = model::user::from_json(crow::json::load(req.body));
        new_user.id = user.value().id;
        new_user.created_at = user.value().created_at;
        service.update(new_user);
        return crow::response{new_user.to_json()};
    }
    return crow::response{crow::status::FORBIDDEN};
  }

  crow::response read(const crow::request &req) {
    if (auto user = service.get(app.template get_context<Session>(req).get("id", -1))){
        return crow::response{user.value().to_json()};
    }
    return crow::response{crow::status::NOT_FOUND};
  }

  crow::response remove(const crow::request &req) {
    if (auto user = service.get(app.template get_context<Session>(req).get("id", -1))){
        service.remove(user.value());
        return crow::response{crow::status::NO_CONTENT};
    }
    return crow::response{crow::status::NOT_FOUND};
  }
};
} // namespace controller
