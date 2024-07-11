#include "model/bucket.hpp"
#include "model/user.hpp"
#include "service/user.hpp"
#include "view/index.hpp"
#include <cstring>
#include <httplib.h>
#include <iostream>
#include <sqlite_orm/sqlite_orm.h>

using namespace sqlite_orm;

int main(int argc, char** argv) {
    auto storage = make_storage("db.sqlite",
                                model::User::make_table(),
                                model::Bucket::make_table());
    storage.sync_schema();
    service::User user_serive(storage);

    user_serive.get_all();

    httplib::Server server;

    server.Get("/", [](const httplib::Request &, httplib::Response &res) {
      res.set_content((std::string)view::index(), "text/html");
    });

    server.listen("0.0.0.0", 80);
    // storage.remove_all<User>();
    // storage.remove_all<Bucket>();
    // auto user_id = storage.insert(User{
    //     .id = 0,
    //     .name = "Alex",
    //     .email = "alex@gmail.com",
    //     .password = "123456",
    // });
    
    // storage.insert(Bucket{
    //     .id = 0,
    //     .name = "Bucket 1",
    //     .folder = "folder1",
    //     .description = "Description 1",
    //     .user_id = user_id,
    // });

    // storage.insert(Bucket{
    //     .id = 1,
    //     .name = "Bucket 2",
    //     .folder = "folder2",
    //     .description = "Description 1",
    //     .user_id = user_id,
    // });

    // for(auto & user : storage.get_all<User>()){
    //     std::cout << "User: " << user.name << "\nbuckets: " << std::endl;
    //     for(auto & bucket : user.get_buckets(storage)){
    //         std::cout << "Bucket: " << bucket.name << std::endl;
    //     }
    // }
    
    return 0;
}
