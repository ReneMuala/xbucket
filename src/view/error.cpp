#include "view.hpp"
#include "../lib/hyper.hpp"

namespace view {
    using namespace hyper;
    std::string error(const std::string & desc){
        return html<
            head<
                title< text<"Error">>
            >,
            body<
                h1<text<"Error">>,
                p<text<"An error occurred.">>,
                h4<text<"description">>,
                code<$<"desc">>
            >
        >{{{"desc", desc}}};
    }
};
