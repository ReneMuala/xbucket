#pragma once

#include "../lib/html.hpp"

namespace view {
std::string index() {
  using namespace static_html;
  return html<head<
                meta<charset, "UTF-8">,
                meta<viewport, "width=device-width, initial-scale=1.0">>,
           body<
                h1<"hello world","class='name'">
           >>{};
}
} // namespace view