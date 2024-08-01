#pragma once
#include <crow/common.h>
#include <crow/http_request.h>
#include <crow/json.h>
#include <list>
#include <map>
#include <string>

#define EMPTY ""
#define controller_register_api_route_auth(_controller, _name, _route,         \
                                           _description, _method, _handler)    \
  controller::routes[#_controller].push_back(                                  \
      {_name, "/api/" #_controller _route, _description, _method, true});      \
  CROW_ROUTE(app, "/api/" #_controller _route)                                 \
      .name(_name)                                                             \
      .CROW_MIDDLEWARES(app, middleware::auth)                                 \
      .methods(_method)([this](const crow::request &req) {                     \
        try {                                                                  \
          return this->_handler(req);                                          \
        } catch (std::runtime_error & e) {                                     \
          crow::json::wvalue resp;                                             \
          resp["error"] = e.what();                                            \
          return crow::response{crow::status::BAD_REQUEST, resp};              \
        }                                                                      \
      });

#define controller_register_api_route(_controller, _name, _route,              \
                                      _description, _method, _handler)         \
  controller::routes[#_controller].push_back(                                  \
      {_name, "/api/" #_controller _route, _description, _method, false});     \
  CROW_ROUTE(app, "/api/" #_controller _route)                                 \
      .name(_name)                                                             \
      .methods(_method)([this](const crow::request &req) {                     \
        try {                                                                  \
          return this->_handler(req);                                          \
        } catch (std::runtime_error & e) {                                     \
          crow::json::wvalue resp;                                             \
          resp["error"] = e.what();                                            \
          return crow::response{crow::status::BAD_REQUEST, resp};              \
        }                                                                      \
      });

namespace controller {

struct route_descr {
  std::string name;
  std::string route;
  std::string description;
  crow::HTTPMethod method;
  bool auth;

  crow::json::wvalue to_json() const {
    crow::json::wvalue result;
    result["name"] = name;
    result["route"] = route;
    result["description"] = description;
    result["method"] = crow::method_name(method);
    result["auth"] = auth;
    return result;
  }
};

class controller {
public:
  static std::map<std::string, std::list<route_descr>> routes;

  static inline std::string get_param(const crow::request &request,
                                      const std::string name) {
    if (const char *value_c_str = request.url_params.get(name)) {
      return std::string(*value_c_str != '\0' ? value_c_str : "");
    }
    throw std::runtime_error("expected path param: " + name);
    return "";
  }

  static inline std::string get_param_or(const crow::request &request,
                                         const std::string name,
                                         const std::string &default_value) {
    if (const char *value_c_str = request.url_params.get(name)) {
      return std::string(*value_c_str != '\0' ? value_c_str : "");
    }
    return default_value;
  }
};
} // namespace controller
