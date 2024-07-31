#pragma once
#include <crow/json.h>

namespace util {
   namespace json {
       template <typename T>
       T get(const crow::json::rvalue & body ,const std::string & field) {
         try {
             return (T)body[field];
         } catch (std::exception & e) {
             throw std::runtime_error("expected field: " + field);
         }
       }

       template <typename T>
       T get_or(const crow::json::rvalue & body ,const std::string & field, T default_value) {
             return body.has(field) ? (T)body[field] : default_value;
       }
   }
}
