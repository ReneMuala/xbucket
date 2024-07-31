#pragma once
#include "crow/app.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/logging.h"
#include "crow/middleware.h"
#include <crow/middlewares/session.h>
#include <functional>
namespace middleware {
using Session = crow::SessionMiddleware<crow::FileStore>;

struct auth : crow::ILocalMiddleware {
  struct context {};

  template <typename AllContext>
  void before_handle(crow::request &req, crow::response &res, context &,
                     AllContext &ctx) {
    auto &context = ctx.template get<Session>();
    if (context.get("id", -1) == -1) {
      res.code = crow::status::FORBIDDEN;
      res.end();
    }
  }

  void after_handle(crow::request &req, crow::response &res, context &ctx) {}
};
} // namespace middleware
