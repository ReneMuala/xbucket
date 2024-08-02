#include "../controller/controller.internal.hpp"
#include "../lib/hyper.hpp"
#include "crow/common.h"
#include "view.hpp"
#include <regex>
#include <string>
namespace view {
using namespace hyper;

std::string row_example(const std::string &label, const std::string &sample) {
  return part<tr<td<$colspan<"6">, small<p<$<"label">>>,
                    pre<$contenteditable<"true">, $<"sample">>>>>{
      {{"label", label}, {"sample", sample}}};
}

std::string beautify_json_sample(std::string json) {
  json = std::regex_replace(json, std::regex("\","), "\",\n");
  json = std::regex_replace(json, std::regex("\\{"), "{\n");
  json = std::regex_replace(json, std::regex("\\}"), "\n}");
  json = std::regex_replace(json, std::regex("\n\""), "\n\t\"");
  return json;
}

std::string route_row(const controller::route_descr &desc) {
  return part<
      tr<td<strong<$<"method">>>,
         td<a<$href<"<<route_link>>">, code<$<"route">>>>, td<$<"name">>,
         td<$<"description">>,
         td<input<$type<"checkbox">, $disabled<"true">, $class<"<<auth>>">>>>,
      $<"input_example">, $<"output_example">>{
      {{"method", crow::method_name(desc.method)},
       {"route", desc.route},
       {"route_link", desc.route},
       {"name", desc.name},
       {"description", desc.description},
       {"auth", desc.auth ? "auth" : ""},
       {"input_example",
        desc.input_sample
            ? row_example("input", beautify_json_sample(
                                       desc.input_sample.value().dump()))
            : std::string("")},
       {"output_example",
        desc.output_sample
            ? row_example("output", beautify_json_sample(
                                        desc.output_sample.value().dump()))
            : std::string("")}}};
}

std::string
routes_table(const std::pair<std::string, std::list<controller::route_descr>>
                 &routes_section) {
  std::string rows = "";

  for (auto &r : routes_section.second) {
    rows += route_row(r);
  }
  return part<
      section<h4<$<"name">>,
              table<tr<th<text<"Method">>, th<text<"Route">>, th<text<"Name">>,
                       th<text<"Description">>, td<text<"Auth">>>,
                    $<"rows">>>>{
      {{"name", routes_section.first}, {"rows", rows}}};
}

std::string routes() {
  static std::string routes_table_content = "";
  static bool routes_table_content_rendered = false;

  if (not routes_table_content_rendered) {
    for (auto &r : controller::controller::routes) {
      routes_table_content += routes_table(r);
    }
    routes_table_content_rendered = true;
  }

  return routes_table_content;
}

std::string index() {
  static auto docs = routes();
  return html<
      head<meta<charset, "UTF-8">,
           meta<viewport, "width=device-width, initial-scale=1.0">,
           meta<author, "Rene Muala">, title<text<"xbucket server">>,
           style<text<SakuraCSS>>>,
      body<h1<text<"xbucket is running">>,
           p<text<"Xbucket is a powerful and flexible bucket management HTTP "
                  "server that offers advanced support for image compression "
                  "and manipulation. It is designed to be integrated as a "
                  "centralized bucket server for diverse systems, where each "
                  "system has isolated access to a set of files.">>,
           hr<>, h2<text<"Api documentation">>, $<"docs">,
           script<text<"document.querySelectorAll('input.auth[type="
                       "checkbox]').forEach(e=>e.checked=true)">>>>{
      {{"docs", docs}}};
}
} // namespace view
