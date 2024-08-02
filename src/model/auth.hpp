#pragma once
#include "../util/json.hpp"
#include "crow/json.h"
#include "crow/logging.h"
#include <string>
#include <system_error>
namespace model {
struct auth {
  std::string email;
  std::string password;

  inline crow::json::wvalue to_json() const {
    return {
        {"email", email},
        {"password", password},
    };
  }

  static inline crow::json::wvalue to_json_sample() {
    static auto sample =
        from_json(crow::json::load(from_json_sample().dump())).to_json();
    return sample;
  }

  static inline crow::json::wvalue from_json_sample() {
    static auto wsample =
        crow::json::wvalue{{"email", "string"}, {"password", "string"}};
    static auto sample = crow::json::load(wsample.dump());
    static bool tested = false;

    if (not tested) {
      tested = true;
      try {
        /// test the sample
        static auto _ = model::auth::from_json(sample);
      } catch (std::system_error &e) {
        CROW_LOG_CRITICAL << "sample test failed: " << __FUNCTION__;
      }
    }
    return sample;
  }

  static inline model::auth from_json(const crow::json::rvalue &json) {
    return {
        .email = util::json::get<std::string>(json, "email"),
        .password = util::json::get<std::string>(json, "password"),
    };
  }

  static inline auto make_table() {
    throw std::runtime_error("auth is not persistent.");
  }
};
} // namespace model
