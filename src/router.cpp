#include "router.hpp"
#include "crow/common.h"
#include "crow/http_request.h"
#include "view/view.hpp"
#include <crow/app.h>

crow::SimpleApp app;

namespace router {
    std::string handle_login(const crow::request & req){
        crow::json::rvalue json = crow::json::load(req.body);
        if(json["username"] == "1" and json["password"] == "1"){
            return view::login_success();
        }
        return view::login_fail();
    }

    void mount(){
        CROW_ROUTE(app, "/")(view::index);
        CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::Get)(view::login);
        CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::Post)(handle_login);
    }

    void run(){
        app.port(8080).multithreaded().run();
    }
}
