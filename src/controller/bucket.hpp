#pragma once
#include "../middleware/auth.hpp"
#include "../service/bucket.hpp"
#include "controller.internal.hpp"
#include "crow/common.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/json.h"
#include "crow/logging.h"
#include "crow/utility.h"
#include <crow/app.h>
#include <optional>
#include <stdexcept>

namespace controller {
using Session = crow::SessionMiddleware<crow::FileStore>;
template <typename S, typename... M> class bucket : public controller {
  crow::Crow<M...> &app;
  service::bucket<S> &service;

public:
  bucket(crow::Crow<M...> &app, service::bucket<S> &service)
      : service(service), app(app) {
    controller_register_api_route_auth_io(
        bucket, "read", EMPTY, "Read bucket (path: id<int>)", "GET"_method,
        read, {}, model::bucket::to_json_sample());
    controller_register_api_route_auth_io(
        bucket, "create", EMPTY, "Create a new bucket", "POST"_method, create,
        model::bucket::from_json_sample(), model::bucket::to_json_sample());
    controller_register_api_route_auth_io(
        bucket, "update", EMPTY, "Update a bucket (path: id<int>)",
        "PUT"_method, update, model::bucket::from_json_sample(),
        model::bucket::to_json_sample());
    controller_register_api_route_auth(bucket, "remove", EMPTY, "Remove a bucket","DELETE"_method,remove);
  }

  crow::response create(const crow::request &req) {
    crow::json::wvalue body = crow::json::load(req.body);
    body["user_id"] = app.template get_context<Session>(req).get("id", -1);
    auto bucket = model::bucket::from_json(crow::json::load(body.dump()));
    auto id = service.insert(bucket);
    return crow::response{bucket.to_json()};
  }

  crow::response update(const crow::request &req){
    auto new_bucket = model::bucket::from_json(crow::json::load(req.body));
    if (auto old_bucket = service.get_with_user(new_bucket.id, new_bucket.user_id)){
        new_bucket.created_at = old_bucket.value().created_at;
        service.update(new_bucket);
        return crow::response{new_bucket.to_json()};
    }
    return crow::response{crow::status::NOT_FOUND};
  }

  crow::response read(const crow::request &req) {
    auto id = std::atoi(get_param(req, "id").c_str());
    if(auto bucket = service.get_with_user(id, app.template get_context<Session>(req).get("id", -1))){
        return crow::response{bucket.value().to_json()};
    }
    return crow::response{crow::status::NOT_FOUND};
  }

  crow::response remove(const crow::request &req) {
    auto id = std::atoi(get_param(req, "id").c_str());
    if (auto bucket = service.get_with_user(id,app.template get_context<Session>(req).get("id", -1))){
        service.remove(bucket.value());
        return crow::response{crow::status::NO_CONTENT};
    }
    return crow::response{crow::status::NOT_FOUND};
  }
};
} // namespace controller
