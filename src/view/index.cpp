#include "../lib/hyper.hpp"
#include "view.hpp"

namespace view {
using namespace hyper;
std::string index() {
  return html<head<meta<charset, "UTF-8">,
                   meta<viewport, "width=device-width, initial-scale=1.0">,
                   meta<author, "Rene Muala">, title<text<"xbucket server">>,
                   style<text<SakuraCSS>>>,
              body<h1<text<"xbucket is running">>,
                   p<text<"See ">, a<$href<"/api/docs">, text<"the docs">>,
                     text<".">>>>{};
}
} // namespace view
