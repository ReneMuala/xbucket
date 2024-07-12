#pragma once

#include "../lib/hyper.hpp"

namespace view {
  using namespace hyper;
inline std::string index() {
  return html<
               head<
                    meta<charset, "UTF-8">,
                    meta<viewport, "width=device-width, initial-scale=1.0">,
                    meta<author, "Rene Muala">
                   >,
               body<
                    h1<$id<"de">, $class<"something">, text<"hello world">>,
                    comment<text<"This is a comment">>,
                    h1<$style<"color:red">,$onclick<"toggle_name(323)">,text<"this red">>,
                    p<$style<"color: green">,text<"a simple paragraph ">,strong<text<"bold">>>,
                    hyper::div<p<text<"p in a div">>>
                    >
               >{};
}

} // namespace view