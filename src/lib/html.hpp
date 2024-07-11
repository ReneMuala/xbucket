
#include <map>
#include <string>
#include <utility>
namespace static_html {

template <class InputIt, class Size, class OutputIt>
constexpr OutputIt copy_n(InputIt first, Size count, OutputIt result) {
  if (count > 0) {
    *result++ = *first;
    for (Size i = 1; i < count; ++i) {
      *result++ = *++first;
    }
  }
  return result;
}

template <size_t N>

struct const_string {
  constexpr const_string(const char (&str)[N]) { copy_n(str, N, value); }

  char value[N]{};
  constexpr bool empty() const { return N == 0; }
  constexpr const char *c_str() const { return &value[0]; }
};

class block_tag {
  std::string &content;
  const std::string tag;

public:
  block_tag(const std::string &tag, std::string &content, const std::string & inject = "",const std::map<std::string, std::string> &params = {})
      : tag(std::move(tag)), content(content) {
    content += "<";
    content += tag;
    for (auto &[key, value] : params) {
      content += " ";
      content += key;
      content += "=\"";
      content += value;
      content += "\"";
    }
    content += (inject.empty() ? "" : " " + inject) + ">";
  }

  ~block_tag() {
    content += "</";
    content += tag;
    content += ">\n";
  }
};

class inline_tag {
  std::string &content;
  const std::string tag;

public:
  inline_tag(const std::string &tag, std::string &content
             ,const std::string & inject = "",const std::map<std::string, std::string> &params = {})
      : tag(std::move(tag)), content(content) {
    content += "<";
    content += tag;
    for (auto &[key, value] : params) {
      content += " ";
      content += key;
      content += "=\"";
      content += value;
      content += "\"";
    }
    content += (inject.empty() ? "" : " " + inject) + ">\n";
  }
};

#define RESTRICT_TAGS(TAG_NAME, BASE_TAG)                                      \
  static_assert(((std::is_base_of_v<BASE_TAG, TAG_NAME>) && ...),              \
                "<" #TAG_NAME "> must contain type 'void' or an element that " \
                "is a subclass of " #BASE_TAG " for all template parameters");

class head_element {};
class html_element {};
class body_element {};

template <typename head = void, typename body = void>
class html : html_element {
public:
  std::string content;
  html() {
    block_tag it("html", content);
    if constexpr (!std::is_same_v<head, void>) {
      head{content};
    }

    if constexpr (!std::is_same_v<body, void>) {
      body{content};
    }
  }

  operator std::string() const { return content; }
};

template <typename... children> class head : head_element {
public:
  std::string &content;
  head(std::string &content) : content(content) {
    block_tag it("head", content);
    RESTRICT_TAGS(children, head_element);
    (children(content), ...);
  }

  operator std::string() const { return content; }
};

enum meta_type { charset, author, viewport };
static constexpr std::string meta_type_strs[] = {
    "charset",
    "author",
    "viewport",
};

template <meta_type type, const_string value = "None">
class meta : head_element {

  std::map<std::string, std::string> pairs;

public:
  std::string &content;
  std::string value_str = value.c_str();
  meta(std::string &content) : content(content) {
    auto k = meta_type_strs[type];
    auto v = value_str;
    pairs[k] = v;
    inline_tag it("meta", content, "", pairs);
  }

  operator std::string() const { return content; }
};

template <typename... children> class body : body_element {
public:
  std::string &content;
  body(std::string &content) : content(content) {
    block_tag it("body", content);
    RESTRICT_TAGS(children, body_element);
    (children(content), ...);
  }

  operator std::string() const { return content; }
};

template <const_string value = "None", const_string inject="">
class h1 : body_element {

public:
  std::string &content;
  h1(std::string &content) : content(content) {
    block_tag it("h1", content, inject.c_str());
    content += value.c_str();
  }

  operator std::string() const { return content; }
};

} // namespace static_html