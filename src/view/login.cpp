#include "../lib/hyper.hpp"
#include "view.hpp"
namespace view {
using namespace hyper;

std::string login_success() {
  return html<head<title<text<"login">>, style<text<SakuraCSS>>>,
              body<center<h1<text<"login success">>>>>{};
}

std::string login_form() {
  return part<form<
      hx::$post<"/login">, hx::$target<"center">, hx::$swap<"innerHTML">,
      hx::$ext<"json-enc">,
      p<label<text<"username">>, input<$type<"text">, $name<"username">>>,
      p<label<text<"password">>, input<$type<"password">, $name<"password">>>,
      p<input<$type<"submit">, $value<"login">>>>>{};
}

std::string login_fail() {
  return part<h1<text<"login">>, p<$style<"color:red">, text<"login failed">>,
              $<"form">>{{{"form", login_form()}}};
}

std::string login() {
  return html<head<title<text<"login">>, style<text<SakuraCSS>>>,
              body<center<$id<"content">, h1<text<"login">>, $<"form">>>>{
             {{"form", login_form()}}}
         << htmx2;
}
}; // namespace view
