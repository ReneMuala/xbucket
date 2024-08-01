#include "server.hpp"
#include "constants/filesystem.hpp"
#include "controller/artifact.hpp"
#include "controller/auth.hpp"
#include "controller/bucket.hpp"
#include "controller/controller.internal.hpp"
#include "controller/docs.hpp"
#include "controller/user.hpp"
#include "crow/common.h"
#include "crow/http_request.h"
#include "middleware/auth.hpp"
#include "model/model.hpp"
#include "service/artifact.hpp"
#include "service/bucket.hpp"
#include "service/user.hpp"
#include "view/view.hpp"
#include <crow/app.h>
#include <cstddef>
#include <cstdlib>

using Session = crow::SessionMiddleware<crow::FileStore>;

namespace server {

crow::App<crow::CookieParser, middleware::auth, Session> app{
    Session{
        crow::FileStore{constants::filesystem::xbucket_sessions_dir},
    },
};

void mount_views() {
  controller::controller::routes["view"].push_back(controller::route_descr{
      .name = "home",
      .route = "/",
      .description = "home page",
      .method = crow::HTTPMethod::Get,
      .auth = false
  });
  CROW_ROUTE(app, "/").methods(crow::HTTPMethod::Get)(view::index);
}

void make_directories() {
  using namespace constants::filesystem;
  std::filesystem::create_directories(xbucket_dir);
  std::filesystem::create_directories(xbucket_db_dir);
  std::filesystem::create_directories(xbucket_uploads_dir);
  std::filesystem::create_directories(xbucket_sessions_dir);
}

void run() {
  make_directories();
  mount_views();
  std::srand(std::time(NULL));
  auto storage = model::get_storage();
  auto us = service::user(storage);
  auto bs = service::bucket(storage);
  auto as = service::artifact(storage);
  controller::auth ac(app, us);
  controller::user uc(app, us);
  controller::bucket bc(app, bs);
  controller::artifact arc(app, as);
  controller::docs dc(app);

  app.bindaddr("0.0.0.0")
      .port(8080)
      .multithreaded()
      .server_name("xbucket/dev")
      .run();
}
} // namespace server
