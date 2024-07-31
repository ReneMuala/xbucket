#pragma once
#include "crow/common.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/json.h"
#include "crow/utility.h"
#include <crow/app.h>
#include <optional>
#include <stdexcept>
#include "../middleware/auth.hpp"
#include "controller.internal.hpp"
namespace controller {
  using Session = crow::SessionMiddleware<crow::FileStore>;
template <typename... M> class docs {
  crow::Crow<M...> &app;

public:
  docs(crow::Crow<M...> &app)
      :  app(app) {
    controller_register_api_route(docs, "routes", "", "Docs", "GET"_method, get_docs);
 }

  crow::response get_docs(const crow::request &req){
    crow::json::wvalue resp;
    for(const auto & [name, routes] : controller::controller::routes){
      auto routes_vect = std::vector<crow::json::wvalue>{};
      for(auto& route : routes){
        routes_vect.push_back(route.to_json());
      }
      resp[name] = std::move(routes_vect);
    }
    return crow::response{resp};
  }
};
} // namespace controller
