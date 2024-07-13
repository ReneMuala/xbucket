#pragma once

#include "../lib/hyper.hpp"

namespace view {
  using namespace hyper;
inline std::string index() {
     auto name = "Baimo";
  return html<
               head<
                    meta<charset, "UTF-8">,
                    meta<viewport, "width=device-width, initial-scale=1.0">,
                    meta<author, "Rene Muala">,
                    base<text<"/">>,
                    title<text<"My first htmx page">>
                   >,
               body<
                    h1<$id<"de">, $class<"something">,
                         text<"hello ">, $<"name">
                         >,
                    p<text<"My text">>,
                    comment<
                         text<"This is a comment">
                         >,
                    h1<$id<"my-id">,$style<"color:red">,$onclick<"toggle_color()">,
                         text<"this red">
                         >,
                    p<$style<"color: green">,
                         text<"a simple paragraph ">,
                         strong<text<"bold">>
                         >,
                    hyper::div<
                         p<text<"p in a div">>
                         >,
                    button<hx::$target<"/account">, hx::$delete<"body">,
                         text<"Delete your account">
                         >,
                    p<
                         strong<text<"im important">>
                         >,
                    script<
                         text<R"(
                         function toggle_color() {
                              var element = document.getElementById("my-id");
                              if (element.style.color == "red") {
                                   element.style.color = "black";
                              } else {
                                   element.style.color = "red";
                              }
                         }
                    )">
                    >
               >
          >{
               // {"name", name}
          };
}

} // namespace view
