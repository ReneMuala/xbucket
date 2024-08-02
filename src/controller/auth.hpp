#pragma once
#include "../middleware/auth.hpp"
#include "../model/auth.hpp"
#include "../service/user.hpp"
#include "../util/json.hpp"
#include "controller.internal.hpp"
#include "crow/common.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/json.h"
#include "crow/utility.h"
#include <crow/app.h>
#include <functional>
#include <optional>
#include <stdexcept>

namespace controller {
using Session = crow::SessionMiddleware<crow::FileStore>;
template <typename S, typename... M> class auth : public controller {
  crow::Crow<M...> &app;
  service::user<S> &service;

public:
  auth(crow::Crow<M...> &app, service::user<S> &service)
      : service(service), app(app) {

    controller_register_api_route_io(
        auth, "login", "/login", "Session login", "POST"_method, login,
        model::auth::from_json_sample(), model::user::to_json_sample());
    controller_register_api_route_auth(auth, "logout", "/logout",
                                       "Session logout", "POST"_method, logout);
  }

  crow::response logout(const crow::request &req) {
    app.template get_context<Session>(req).set("id", -1);
    return crow::response{crow::status::NO_CONTENT};
  }

  crow::response login(const crow::request &req) {
    auto &session = app.template get_context<Session>(req);
    session.set("id", -1);
    auto body = crow::json::load(req.body);
    auto email = util::json::get<std::string>(body, "email");
    auto password = util::json::get<std::string>(body, "password");
    std::optional<model::user> user = service.get_login(email, password);
    if (user) {
      session.set("id", user->id);
      return crow::response{user.value().to_json()};
    } else {
      return crow::response{crow::status::NOT_FOUND};
    }
  }
};
} // namespace controller
