#pragma once

#include "../lib/html.hpp"

namespace view {
inline std::string index() {
  using namespace static_html;
  return html<head<meta<charset, "UTF-8">,
                   meta<viewport, "width=device-width, initial-scale=1.0">>,
              body<h1<$id<"de">, $class<"something">, text<"hello world">>,
                   h1<text<"...">>>>{};
}

} // namespace view