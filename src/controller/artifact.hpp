#pragma once
#include "../middleware/auth.hpp"
#include "../model/model.hpp"
#include "../service/artifact.hpp"
#include "controller.internal.hpp"
#include "crow/common.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/json.h"
#include "crow/logging.h"
#include "crow/utility.h"
#include <algorithm>
#include <crow/app.h>
#include <crow/multipart.h>
#include <ctime>
#include <exception>
#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>

namespace controller {
using Session = crow::SessionMiddleware<crow::FileStore>;
template <typename S, typename... M> class artifact : public controller {
  crow::Crow<M...> &app;
  service::artifact<S> &service;

public:
  artifact(crow::Crow<M...> &app, service::artifact<S> &service)
      : service(service), app(app) {
    controller_register_api_route_auth_io(
        artifact, "read", EMPTY,
        "Read artifact's metadata or download its file (if dl==true). (path: "
        "id<int>, bucket_id<int>, "
        "dl<bool?>)",
        "GET"_method, read, {}, model::artifact::to_json_sample());
    controller_register_api_route_auth_io(
        artifact, "create", EMPTY,
        "Create artifacts from multipart upload. "
        "(path: bucket_id<int>)",
        "POST"_method, create, {}, model::artifact::to_json_sample());
    // controller_register_api_route_auth(artifact, "update", EMPTY,
    //                                    "Update artifact", "PUT"_method,
    //                                    update);
    controller_register_api_route_auth(
        artifact, "remove", EMPTY,
        "Remove artifact (path: id<int>, bucket_id<int>)", "DELETE"_method,
        remove);
  }

  std::string get_random_filename(std::string prefix,
                                  std::string original_filename) {
    constexpr auto salts = 10000;
    return prefix + std::to_string(rand() % salts + salts) +
           std::string{original_filename.begin() + original_filename.rfind("."),
                       original_filename.end()};
  }

  std::vector<model::artifact> get_multipart_uploads(const crow::request &req) {
    std::vector<model::artifact> artifacts;
    auto bucket_id = atoi(get_param(req, "bucket_id").c_str());
    crow::multipart::message file_message(req);
    for (const auto &part : file_message.part_map) {
      const auto &part_name = part.first;
      const auto &part_value = part.second;
      // Extract the file name
      auto headers_it = part_value.headers.find("Content-Disposition");
      if (headers_it == part_value.headers.end()) {
        throw std::runtime_error("No Content-Disposition found");
      }
      auto params_it = headers_it->second.params.find("filename");
      if (params_it == headers_it->second.params.end()) {
        throw std::runtime_error("Part with name " + part_name +
                                 " should have a file");
      }
      const std::string outfile_name = get_random_filename(
          req.remote_ip_address + "." + std::to_string(bucket_id) + "." +
              std::to_string((int)time(NULL)),
          params_it->second);

      // Create a new file with the extracted file name and write file contents
      // to it
      std::ofstream out_file(constants::filesystem::xbucket_uploads_dir +
                             outfile_name);
      if (!out_file) {
        throw std::runtime_error("Write to file failed\n");
        continue;
      }
      out_file << part_value.body;
      out_file.close();

      artifacts.push_back(
          model::artifact{.name = part_name,
                          .filename = outfile_name,
                          .original_filename = params_it->second,
                          .bucket_id = bucket_id});
    }
    return artifacts;
  }

  std::vector<crow::json::wvalue>
  store_artifacts_from_uploads(std::vector<model::artifact> &artifacts,
                               std::function<int(model::artifact &)> callback) {
    auto response_vector = std::vector<crow::json::wvalue>{};
    if (artifacts.size()) {
      std::reference_wrapper<model::artifact> first_artifact =
          artifacts.front();
      model::get_storage().transaction([&] mutable {
        try {
          callback(first_artifact.get());
          response_vector.push_back(first_artifact.get().to_json());
          for (auto it = artifacts.begin() + 1; it < artifacts.end(); it++) {
            it->super = first_artifact.get().id;
            callback(*it);
            response_vector.push_back(it->to_json());
          }
          return true;
        } catch (std::system_error &e) {
          throw e;
          return false;
        }
      });
    }
    return response_vector;
  }

  crow::response create(const crow::request &req) {
    auto artifacts = get_multipart_uploads(req);
    auto response = crow::json::wvalue{};
    if (artifacts.empty()) {
      response["error"] = "No multipart file provied";
      return crow::response{crow::status::BAD_REQUEST, response};
    }
    try {
      auto stored_artifacts = store_artifacts_from_uploads(
          artifacts, [this](model::artifact &artifact) {
            return service.insert(artifact);
          });
      response = std::move(stored_artifacts);
      return response;
    } catch (std::system_error &e) {
      response["error"] = "Bucket not found";
      return crow::response{crow::status::NOT_FOUND, response};
    }
  }

  crow::response update(const crow::request &req) {
    auto new_artifact = model::artifact::from_json(crow::json::load(req.body));
    auto id = std::atoi(get_param(req, "id").c_str());
    if (auto old_artifact = service.get_with_bucket_and_user(
            new_artifact.id, new_artifact.bucket_id,
            app.template get_context<Session>(req).get("id", -1))) {
      new_artifact.created_at = old_artifact.value().created_at;
      service.update(new_artifact);
      return crow::response{new_artifact.to_json()};
    }
    return crow::response{crow::status::NOT_FOUND};
  }

  crow::response read(const crow::request &req) {
    auto id = std::atoi(get_param(req, "id").c_str());
    auto bucket_id = std::atoi(get_param(req, "bucket_id").c_str());
    auto download = get_param_or(req, "dl", "false");
    if (auto artifact = service.get_with_bucket_and_user(
            id, bucket_id,
            app.template get_context<Session>(req).get("id", -1))) {
      if (download == "true") {
        crow::response res;
        res.set_static_file_info(constants::filesystem::xbucket_uploads_dir +
                                 artifact.value().filename);
        return res;
      }
      return crow::response{artifact.value().to_json()};
    }
    return crow::response{crow::status::NOT_FOUND};
  }

  crow::response remove(const crow::request &req) {
    auto id = std::atoi(get_param(req, "id").c_str());
    auto bucket_id = std::atoi(get_param(req, "bucket_id").c_str());
    if (auto artifact = service.get_with_bucket_and_user(
            id, bucket_id,
            app.template get_context<Session>(req).get("id", -1))) {
      service.remove(artifact.value());
      return crow::response{crow::status::NO_CONTENT};
    }
    return crow::response{crow::status::NOT_FOUND};
  }
};
} // namespace controller
