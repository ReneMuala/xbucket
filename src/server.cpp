#include "server.hpp"
#include "crow/common.h"
#include "crow/http_request.h"
#include "service/user.hpp"
#include "service/bucket.hpp"
#include "view/view.hpp"
#include <crow/app.h>
#include "model/model.hpp"
#include "middleware/auth.hpp"
#include "controller/auth.hpp"
#include "controller/docs.hpp"
#include "controller/user.hpp"
#include "controller/bucket.hpp"


using Session = crow::SessionMiddleware<crow::FileStore>;

namespace server {

    crow::App<crow::CookieParser, middleware::auth, Session> app{
          Session{
              crow::FileStore{"./"},
          },
      };

    void run(){
        auto storage = model::make_storage();
        auto us = service::user(storage);
        auto bs = service::bucket(storage);

        controller::auth ac(app, us);
        controller::user uc(app, us);
        controller::bucket bc(app, bs);
        controller::docs dc(app);
        app .bindaddr("0.0.0.0")
            .port(8080)
            .multithreaded()
            .run();
    }
}
