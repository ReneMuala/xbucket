
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
namespace hyper {

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
  block_tag(const std::string &tag, std::string &content,
            const std::string &inject = "",
            const std::map<std::string, std::string> &params = {})
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
    content += ">";
  }
};

class inline_tag {
  std::string &content;
  const std::string tag;

public:
  inline_tag(const std::string &tag, std::string &content,
             const std::string &inject = "",
             const std::map<std::string, std::string> &params = {})
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
  explicit html(const std::map<std::string, std::string> &fields = {}) {
    block_tag it("html", content);
    if constexpr (!std::is_same_v<head, void>) {
      head{content};
    }

    if constexpr (!std::is_same_v<body, void>) {
      body{content};
    }

    for (const auto &[k, v] : fields) {
      const auto pos = content.find("<<" + k + ">>");
      if (pos != std::string::npos)
        content.replace(pos, k.length() + 4, v);
    }
  }

  operator std::string() const { return content; }

  template<typename INJECTABLE>
  html & operator << (INJECTABLE & injectable) {
      injectable.inject(content);
      return *this;
  }
};
///! Meta Info: Defines information about the document
template <typename... children> class head : head_element {
public:
  std::string &content;
  head(std::string &content) : content(content) {
    block_tag it("head", content);
    // RESTRICT_TAGS(children, head_element);
    (children(content), ...);
  }

  operator std::string() const { return content; }
};

enum meta_type { charset, author, viewport, http_equiv, content, name };
static constexpr const char *meta_type_strs[] = {
    "charset", "author", "viewport", "http-equiv", "content", "name"};

///! Meta Info: Defines metadata about an HTML document
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

///! Formatting: Defines a container for content that should be hidden when the
///page loads
template <typename... children> class template$ : body_element {
public:
  std::string &content;
  template$(std::string &content) : content(content) {
    block_tag it("template", content);
    RESTRICT_TAGS(children, body_element);
    (children(content), ...);
  }

  operator std::string() const { return content; }
};

#define CREATE_BODY_INLINE_COMPONENT(NAME)                                     \
  template <typename... children> class NAME : body_element {                  \
  public:                                                                      \
    std::string &content;                                                      \
    NAME(std::string &content) : content(content) {                            \
      inline_tag it(#NAME, content);                                           \
      (children(content), ...);                                                \
    }                                                                          \
    operator std::string() const { return content; }                           \
  };

///! Basic HTML: Defines a comment
template <typename... children> class comment : body_element {
public:
  std::string &content;
  comment(std::string &content) : content(content) {
    content += "<!--";
    (children(content), ...);
    content += "--!>";
  }
  operator std::string() const { return content; }
};

#define CREATE_BODY_BLOCK_COMPONENT(NAME)                                      \
  template <typename... children> class NAME : body_element {                  \
  public:                                                                      \
    std::string &content;                                                      \
    NAME(std::string &content) : content(content) {                            \
      block_tag it(#NAME, content);                                            \
      (children(content), ...);                                                \
    }                                                                          \
    operator std::string() const { return content; }                           \
  };

///! Basic HTML: Defines a title for the document
CREATE_BODY_BLOCK_COMPONENT(title);

///! Basic HTML: Defines heading level 1
CREATE_BODY_BLOCK_COMPONENT(h1);
///! Basic HTML: Defines heading level 2
CREATE_BODY_BLOCK_COMPONENT(h2);
///! Basic HTML: Defines heading level 3
CREATE_BODY_BLOCK_COMPONENT(h3);
///! Basic HTML: Defines heading level 4
CREATE_BODY_BLOCK_COMPONENT(h4);
///! Basic HTML: Defines heading level 5
CREATE_BODY_BLOCK_COMPONENT(h5);
///! Basic HTML: Defines heading level 6
CREATE_BODY_BLOCK_COMPONENT(h6);
///! Basic HTML: Defines a paragraph
CREATE_BODY_BLOCK_COMPONENT(p);
///! Basic HTML: Inserts a single line break
CREATE_BODY_INLINE_COMPONENT(br);
///! Basic HTML: Defines a thematic change in the content
CREATE_BODY_INLINE_COMPONENT(hr);

///! Formatting: Not supported in HTML5. Use abbr instead. Defines an acronym
CREATE_BODY_BLOCK_COMPONENT(acronym);
///! Formatting: Defines an abbreviation or an acronym
CREATE_BODY_BLOCK_COMPONENT(abbr);
///! Formatting: Defines contact information for the author/owner of a
///document/article
CREATE_BODY_BLOCK_COMPONENT(address);
///! Formatting: Defines bold text
CREATE_BODY_BLOCK_COMPONENT(b);
///! Formatting: Isolates a part of text that might be formatted in a different
///direction from other text outside it
CREATE_BODY_BLOCK_COMPONENT(bdi);
///! Formatting: Overrides the current text direction
CREATE_BODY_BLOCK_COMPONENT(bdo);
///! Formatting: Not supported in HTML5. Use CSS instead. Defines big text
CREATE_BODY_BLOCK_COMPONENT(big);
///! Formatting: Defines a section that is quoted from another source
CREATE_BODY_BLOCK_COMPONENT(blockquote);
///! Formatting: Not supported in HTML5. Use CSS instead. Defines centered text
CREATE_BODY_BLOCK_COMPONENT(center);
///! Formatting: Defines the title of a work
CREATE_BODY_BLOCK_COMPONENT(cite);
///! Formatting: Defines a piece of computer code
CREATE_BODY_BLOCK_COMPONENT(code);
///! Formatting: Defines text that has been deleted from a document
CREATE_BODY_BLOCK_COMPONENT(del);
///! Formatting: Specifies a term that is going to be defined within the content
CREATE_BODY_BLOCK_COMPONENT(dfn);
///! Formatting: Defines emphasized text
CREATE_BODY_BLOCK_COMPONENT(em);
///! Formatting: Not supported in HTML5. Use CSS instead. Defines font, color,
///and size for text
CREATE_BODY_BLOCK_COMPONENT(font);
///! Formatting: Defines a part of text in an alternate voice or mood
CREATE_BODY_BLOCK_COMPONENT(i);
///! Formatting: Defines a text that has been inserted into a document
CREATE_BODY_BLOCK_COMPONENT(ins);
///! Formatting: Defines keyboard input
CREATE_BODY_BLOCK_COMPONENT(kbd);
///! Formatting: Defines marked/highlighted text
CREATE_BODY_BLOCK_COMPONENT(mark);
///! Formatting: Defines a scalar measurement within a known range (a gauge)
CREATE_BODY_BLOCK_COMPONENT(meter);
///! Formatting: Defines preformatted text
CREATE_BODY_BLOCK_COMPONENT(pre);
///! Formatting: Represents the progress of a task
CREATE_BODY_BLOCK_COMPONENT(progress);
///! Formatting: Defines a short quotation
CREATE_BODY_BLOCK_COMPONENT(q);
///! Formatting: Defines what to show in browsers that do not support ruby
///annotations
CREATE_BODY_BLOCK_COMPONENT(rp);
///! Formatting: Defines an explanation/pronunciation of characters (for East
///Asian typography)
CREATE_BODY_BLOCK_COMPONENT(rt);
///! Formatting: Defines a ruby annotation (for East Asian typography)
CREATE_BODY_BLOCK_COMPONENT(ruby);
///! Formatting: Defines text that is no longer correct
CREATE_BODY_BLOCK_COMPONENT(s);
///! Formatting: Defines sample output from a computer program
CREATE_BODY_BLOCK_COMPONENT(samp);
///! Formatting: Defines smaller text
CREATE_BODY_BLOCK_COMPONENT(small);
///! Formatting: Not supported in HTML5. Use del or s instead. Defines
///strikethrough text
CREATE_BODY_BLOCK_COMPONENT(strike);
///! Formatting: Defines important text
CREATE_BODY_BLOCK_COMPONENT(strong);
///! Formatting: Defines subscripted text
CREATE_BODY_BLOCK_COMPONENT(sub);
///! Formatting: Defines superscripted text
CREATE_BODY_BLOCK_COMPONENT(sup);
///! Formatting: Defines a specific time (or datetime)
CREATE_BODY_BLOCK_COMPONENT(time);
///! Formatting: Not supported in HTML5. Use CSS instead. Defines teletype text
CREATE_BODY_BLOCK_COMPONENT(tt);
///! Formatting: Defines some text that is unarticulated and styled differently
///from normal text
CREATE_BODY_BLOCK_COMPONENT(u);
///! Formatting: Defines a variable
CREATE_BODY_BLOCK_COMPONENT(var);
///! Formatting: Defines a possible line-break
CREATE_BODY_INLINE_COMPONENT(wbr);

///! Forms and Input: Defines an HTML form for user input
CREATE_BODY_BLOCK_COMPONENT(form);
///! Forms and Input: Defines an input control
CREATE_BODY_INLINE_COMPONENT(input);
///! Forms and Input: Defines a multiline input control (text area)
CREATE_BODY_BLOCK_COMPONENT(textarea);
///! Forms and Input: Defines a clickable button
CREATE_BODY_BLOCK_COMPONENT(button);
///! Forms and Input: Defines a drop-down list
CREATE_BODY_BLOCK_COMPONENT(select);
///! Forms and Input: Defines a group of related options in a drop-down list
CREATE_BODY_BLOCK_COMPONENT(optgroup);
///! Forms and Input: Defines an option in a drop-down list
CREATE_BODY_BLOCK_COMPONENT(option);
///! Forms and Input: Defines a label for aninput element
CREATE_BODY_BLOCK_COMPONENT(label);
///! Forms and Input: Groups related elements in a form
CREATE_BODY_BLOCK_COMPONENT(fieldset);
///! Forms and Input: Defines a caption for afieldset element
CREATE_BODY_BLOCK_COMPONENT(legend);
///! Forms and Input: Specifies a list of pre-defined options for input controls
CREATE_BODY_BLOCK_COMPONENT(datalist);
///! Forms and Input: Defines the result of a calculation
CREATE_BODY_BLOCK_COMPONENT(output);

///! Frames: Not supported in HTML5. Defines a window (a frame) in a frameset
CREATE_BODY_BLOCK_COMPONENT(frame);
///! Frames: Not supported in HTML5. Defines a set of frames
CREATE_BODY_BLOCK_COMPONENT(frameset);
///! Frames: Not supported in HTML5. Defines an alternate content for users that
///do not support frames
CREATE_BODY_BLOCK_COMPONENT(noframes);
///! Frames: Defines an inline frame
CREATE_BODY_BLOCK_COMPONENT(iframe);

///! Images: Defines an image
CREATE_BODY_INLINE_COMPONENT(img);
///! Images: Defines a client-side image map
CREATE_BODY_BLOCK_COMPONENT(map);
///! Images: Defines an area inside an image map
CREATE_BODY_INLINE_COMPONENT(area);
///! Images: Used to draw graphics, on the fly, via scripting (usually
///JavaScript)
CREATE_BODY_BLOCK_COMPONENT(canvas);
///! Images: Defines a caption for a figure element
CREATE_BODY_BLOCK_COMPONENT(figcaption);
///! Images: Specifies self-contained content
CREATE_BODY_BLOCK_COMPONENT(figure);
///! Images: Defines a container for multiple image resources
CREATE_BODY_BLOCK_COMPONENT(picture);
///! Images: Defines a container for SVG graphics
CREATE_BODY_BLOCK_COMPONENT(svg);

///! Audio / Video: Defines sound content
CREATE_BODY_BLOCK_COMPONENT(audio);
///! Audio / Video: Defines multiple media resources for media elements (video,
///audio and picture)
CREATE_BODY_INLINE_COMPONENT(source);
///! Audio / Video: Defines text tracks for media elements (video and audio)
CREATE_BODY_INLINE_COMPONENT(track);
///! Audio / Video: Defines a video or movie
CREATE_BODY_BLOCK_COMPONENT(video);

///! Links: Defines a hyperlink
CREATE_BODY_BLOCK_COMPONENT(a);
///! Links: Defines the relationship between a document and an external resource
///(most used to link to style sheets)
CREATE_BODY_BLOCK_COMPONENT(link);
///! Links: Defines navigation links
CREATE_BODY_BLOCK_COMPONENT(nav);

///! Lists: Defines an alternative unordered list
CREATE_BODY_BLOCK_COMPONENT(menu);
///! Lists: Defines an unordered list
CREATE_BODY_BLOCK_COMPONENT(ul);
///! Lists: Defines an ordered list
CREATE_BODY_BLOCK_COMPONENT(ol);
///! Lists: Defines a list item
CREATE_BODY_BLOCK_COMPONENT(li);
///! Lists: Not supported in HTML5. Use ul instead. Defines a directory list
CREATE_BODY_BLOCK_COMPONENT(dir);
///! Lists: Defines a description list
CREATE_BODY_BLOCK_COMPONENT(dl);
///! Lists: Defines a term/name in a description list
CREATE_BODY_BLOCK_COMPONENT(dt);
///! Lists: Defines a description of a term/name in a description list
CREATE_BODY_BLOCK_COMPONENT(dd);

///! Tables: Defines a table
CREATE_BODY_BLOCK_COMPONENT(table);
///! Tables: Defines a table caption
CREATE_BODY_BLOCK_COMPONENT(caption);
///! Tables: Defines a header cell in a table
CREATE_BODY_BLOCK_COMPONENT(th);
///! Tables: Defines a row in a table
CREATE_BODY_BLOCK_COMPONENT(tr);
///! Tables: Defines a cell in a table
CREATE_BODY_BLOCK_COMPONENT(td);
///! Tables: Groups the header content in a table
CREATE_BODY_BLOCK_COMPONENT(thead);
///! Tables: Groups the body content in a table
CREATE_BODY_BLOCK_COMPONENT(tbody);
///! Tables: Groups the footer content in a table
CREATE_BODY_BLOCK_COMPONENT(tfoot);
///! Tables: Specifies column properties for each column within acolgroup
///element
CREATE_BODY_BLOCK_COMPONENT(col);
///! Tables: Specifies a group of one or more columns in a table for formatting
CREATE_BODY_BLOCK_COMPONENT(colgroup);

///! Styles and Semantics: Defines style information for a document
CREATE_BODY_BLOCK_COMPONENT(style);
///! Styles and Semantics: Defines a section in a document
CREATE_BODY_BLOCK_COMPONENT(div);
///! Styles and Semantics: Defines a section in a document
CREATE_BODY_BLOCK_COMPONENT(span);
///! Styles and Semantics: Defines a header for a document or section
CREATE_BODY_BLOCK_COMPONENT(header);
///! Styles and Semantics: Defines a header and related content
CREATE_BODY_BLOCK_COMPONENT(hgroup);
///! Styles and Semantics: Defines a footer for a document or section
CREATE_BODY_BLOCK_COMPONENT(footer);
///! Styles and Semantics: Specifies the main content of a document
CREATE_BODY_BLOCK_COMPONENT(main);
///! Styles and Semantics: Defines a section in a document
CREATE_BODY_BLOCK_COMPONENT(section);
///! Styles and Semantics: Defines a search section
CREATE_BODY_BLOCK_COMPONENT(search);
///! Styles and Semantics: Defines an article
CREATE_BODY_BLOCK_COMPONENT(article);
///! Styles and Semantics: Defines content aside from the page content
CREATE_BODY_BLOCK_COMPONENT(aside);
///! Styles and Semantics: Defines additional details that the user can view or
///hide
CREATE_BODY_BLOCK_COMPONENT(details);
///! Styles and Semantics: Defines a dialog box or window
CREATE_BODY_BLOCK_COMPONENT(dialog);
///! Styles and Semantics: Defines a visible heading for adetails element
CREATE_BODY_BLOCK_COMPONENT(summary);
///! Styles and Semantics: Adds a machine-readable translation of a given
///content
CREATE_BODY_BLOCK_COMPONENT(data);

#define CREATE_ATTRIBUTE(NAME)                                                 \
  template <const_string value = "None"> class $##NAME : body_element {        \
  public:                                                                      \
    std::string &content;                                                      \
    std::map<std::string, std::string> pairs;                                  \
    $##NAME(std::string &content) : content(content) {                         \
      content.insert(content.rfind(">"),                                       \
                     std::string(" " #NAME "=\"") + value.c_str() + "\"");     \
    }                                                                          \
    operator std::string() const { return content; }                           \
  };

///! <form>	Specifies the character encodings that are to be used for the
///form submission
template <const_string value = "None"> class $accept_charset : body_element {
public:
  std::string &content;
  std::map<std::string, std::string> pairs;
  $accept_charset(std::string &content) : content(content) {
    content.insert(content.rfind(">"),
                   std::string(" accept-charset=\"") + value.c_str() + "\"");
  }
  operator std::string() const { return content; }
};

///! Global Attributes	Used to store custom data private to the page or
///application
template <const_string key = "None", const_string value = "None">
class $data_ : body_element {
public:
  std::string &content;
  std::map<std::string, std::string> pairs;
  $data_(std::string &content) : content(content) {
    content.insert(content.rfind(">"),
                   " " + std::string(key) + "=\"" + value.c_str() + "\"");
  }
  operator std::string() const { return content; }
};

///! <input>	Specifies the types of files that the server accepts (only for
///type="file")
CREATE_ATTRIBUTE(accept);
///! Global Attributes	Specifies a shortcut key to activate/focus an element
CREATE_ATTRIBUTE(accesskey);
///! <form>	Specifies where to send the form-data when a form is submitted
CREATE_ATTRIBUTE(action);
///!	Not supported in HTML 5.	Specifies the alignment according to
///surrounding elements. Use CSS instead
CREATE_ATTRIBUTE(align);
///! <area>, <img>, <input>	Specifies an alternate text when the original
///element fails to display
CREATE_ATTRIBUTE(alt);
// <script>	Specifies that the script is executed asynchronously (only for
// external scripts)
CREATE_ATTRIBUTE(async)
// <form>, <input>	Specifies whether the <form> or the <input> element
// should have autocomplete enabled
CREATE_ATTRIBUTE(autocomplete)
// <button>, <input>, <select>, <textarea>	Specifies that the element
// should automatically get focus when the page loads
CREATE_ATTRIBUTE(autofocus)
// <audio>, <video>	Specifies that the audio/video will start playing as
// soon as it is ready
CREATE_ATTRIBUTE(autoplay)
// Not supported in HTML 5.	Specifies the background color of an element.
// Use CSS instead
CREATE_ATTRIBUTE(bgcolor)
// Not supported in HTML 5.	Specifies the width of the border of an element.
// Use CSS instead
CREATE_ATTRIBUTE(border)
// <script>	Specifies the character encoding
CREATE_ATTRIBUTE(charset)
///! <input>	Specifies that an <input> element should be pre-selected when
///the page loads (for type="checkbox" or type="radio")
CREATE_ATTRIBUTE(checked);
///! <blockquote>, <del>, <ins>, <q>	Specifies a URL which explains the
///quote/deleted/inserted text
CREATE_ATTRIBUTE(cite);
///! Global Attributes	Specifies one or more class names for an element (refers
///to a class in a style sheet)
CREATE_ATTRIBUTE(class);
///! Not supported in HTML 5.	Specifies the text color of an element. Use CSS
///instead
CREATE_ATTRIBUTE(color);
///! <textarea>	Specifies the visible width of a text area
CREATE_ATTRIBUTE(cols);
///! <td>, <th>	Specifies the number of columns a table cell should span
CREATE_ATTRIBUTE(colspan);
///! Global Attributes	Specifies whether the content of an element is editable
///or not
CREATE_ATTRIBUTE(contenteditable);
///! <audio>, <video>	Specifies that audio/video controls should be displayed
///(such as a play/pause button etc.)
CREATE_ATTRIBUTE(controls);
///! <area>	Specifies the coordinates of the area
CREATE_ATTRIBUTE(coords);
///! <object>	Specifies the URL of the resource to be used by the object
CREATE_ATTRIBUTE(data);
///! <del>, <ins>, <time>	Specifies the date and time
CREATE_ATTRIBUTE(datetime);
///! <track>	Specifies that the track is to be enabled if the user's
///preferences do not indicate that another track would be more appropriate
CREATE_ATTRIBUTE(default);
///! <script>	Specifies that the script is executed when the page has finished
///parsing (only for external scripts)
CREATE_ATTRIBUTE(defer);
///! Global Attributes	Specifies the text direction for the content in an
///element
CREATE_ATTRIBUTE(dir);
///! <input>, <textarea>	Specifies that the text direction will be
///submitted
CREATE_ATTRIBUTE(dirname);
///! <button>, <fieldset>, <input>, <optgroup>, <option>, <select>, <textarea>
///Specifies that the specified element/group of elements should be disabled
CREATE_ATTRIBUTE(disabled);
///! <a>, <area>	Specifies that the target will be downloaded when a user
///clicks on the hyperlink
CREATE_ATTRIBUTE(download);
///! Global Attributes	Specifies whether an element is draggable or not
CREATE_ATTRIBUTE(draggable);
///! <form>	Specifies how the form-data should be encoded when submitting it
///to the server (only for method="post")
CREATE_ATTRIBUTE(enctype);
///! Global Attributes	Specifies the text of the enter-key on a virtual
///keyboard
CREATE_ATTRIBUTE(enterkeyhint);
///! <label>, <output>	Specifies which form element(s) a label/calculation is
///bound to
CREATE_ATTRIBUTE(for);
///! <button>, <fieldset>, <input>, <label>, <meter>, <object>, <output>,
///<select>, <textarea>	Specifies the name of the form the element belongs to
CREATE_ATTRIBUTE(form);
///! <button>, <input>	Specifies where to send the form-data when a form is
///submitted. Only for type="submit"
CREATE_ATTRIBUTE(formaction);
///! <td>, <th>	Specifies one or more headers cells a cell is related to
CREATE_ATTRIBUTE(headers);
///! <canvas>, <embed>, <iframe>, <img>, <input>, <object>, <video>
///Specifies the height of the element
CREATE_ATTRIBUTE(height);
///! Global Attributes	Specifies that an element is not yet, or is no longer,
///relevant
CREATE_ATTRIBUTE(hidden);
///! <meter>	Specifies the range that is considered to be a high value
CREATE_ATTRIBUTE(high);
///! <a>, <area>, <base>, <link>	Specifies the URL of the page the link
///goes to
CREATE_ATTRIBUTE(href);
///! <a>, <area>, <link>	Specifies the language of the linked document
CREATE_ATTRIBUTE(hreflang);
///! Global Attributes	Specifies a unique id for an element
CREATE_ATTRIBUTE(id);
///! Global Attributes	Specifies that the browser should ignore this section
CREATE_ATTRIBUTE(inert);
///! Global Attributes	Specifies the mode of a virtual keyboard
CREATE_ATTRIBUTE(inputmode);
///! <img>	Specifies an image as a server-side image map
CREATE_ATTRIBUTE(ismap);
///! <track>	Specifies the kind of text track
CREATE_ATTRIBUTE(kind);
///! <track>, <option>, <optgroup>	Specifies the title of the text track
CREATE_ATTRIBUTE(label);
///! Global Attributes	Specifies the language of the element's content
CREATE_ATTRIBUTE(lang);
///! <input>	Refers to a <datalist> element that contains pre-defined options
///for an <input> element
CREATE_ATTRIBUTE(list);
///! <audio>, <video>	Specifies that the audio/video will start over again,
///every time it is finished
CREATE_ATTRIBUTE(loop);
///! <meter>	Specifies the range that is considered to be a low value
CREATE_ATTRIBUTE(low);
///! <input>, <meter>, <progress>	Specifies the maximum value
CREATE_ATTRIBUTE(max);
///! <input>, <textarea>	Specifies the maximum number of characters
///allowed in an element
CREATE_ATTRIBUTE(maxlength);
///! <a>, <area>, <link>, <source>, <style>	Specifies what media/device the
///linked document is optimized for
CREATE_ATTRIBUTE(media);
///! <form>	Specifies the HTTP method to use when sending form-data
CREATE_ATTRIBUTE(method);
///! <input>, <meter>	Specifies a minimum value
CREATE_ATTRIBUTE(min);
///! <input>, <select>	Specifies that a user can enter more than one value
CREATE_ATTRIBUTE(multiple);
///! <video>, <audio>	Specifies that the audio output of the video should be
///muted
CREATE_ATTRIBUTE(muted);
///! <button>, <fieldset>, <form>, <iframe>, <input>, <map>, <meta>, <object>,
///<output>, <param>, <select>, <textarea>	Specifies the name of the
///element
CREATE_ATTRIBUTE(name);
///! <form>	Specifies that the form should not be validated when submitted
CREATE_ATTRIBUTE(novalidate);
///! <audio>, <embed>, <img>, <object>, <video>	Script to be run on abort
CREATE_ATTRIBUTE(onabort);
///! <body>	Script to be run after the document is printed
CREATE_ATTRIBUTE(onafterprint);
///! <body>	Script to be run before the document is printed
CREATE_ATTRIBUTE(onbeforeprint);
///! <body>	Script to be run when the document is about to be unloaded
CREATE_ATTRIBUTE(onbeforeunload);
///! All visible elements.	Script to be run when the element loses focus
CREATE_ATTRIBUTE(onblur);
///! <audio>, <embed>, <object>, <video>	Script to be run when a file is
///ready to start playing (when it has buffered enough to begin)
CREATE_ATTRIBUTE(oncanplay);
///! <audio>, <video>	Script to be run when a file can be played all the way
///to the end without pausing for buffering
CREATE_ATTRIBUTE(oncanplaythrough);
///! All visible elements.	Script to be run when the value of the element
///is changed
CREATE_ATTRIBUTE(onchange);
///! All visible elements.	Script to be run when the element is being
///clicked
CREATE_ATTRIBUTE(onclick);
///! All visible elements.	Script to be run when a context menu is
///triggered
CREATE_ATTRIBUTE(oncontextmenu);
///! All visible elements.	Script to be run when the content of the element
///is being copied
CREATE_ATTRIBUTE(oncopy);
///! <track>	Script to be run when the cue changes in a <track> element
CREATE_ATTRIBUTE(oncuechange);
///! All visible elements.	Script to be run when the content of the element
///is being cut
CREATE_ATTRIBUTE(oncut);
///! All visible elements.	Script to be run when the element is being
///double-clicked
CREATE_ATTRIBUTE(ondblclick);
///! All visible elements.	Script to be run when the element is being
///dragged
CREATE_ATTRIBUTE(ondrag);
///! All visible elements.	Script to be run at the end of a drag operation
CREATE_ATTRIBUTE(ondragend);
///! All visible elements.	Script to be run when an element has been
///dragged to a valid drop target
CREATE_ATTRIBUTE(ondragenter);
///! All visible elements.	Script to be run when an element leaves a valid
///drop target
CREATE_ATTRIBUTE(ondragleave);
///! All visible elements.	Script to be run when an element is being
///dragged over a valid drop target
CREATE_ATTRIBUTE(ondragover);
///! All visible elements.	Script to be run at the start of a drag
///operation
CREATE_ATTRIBUTE(ondragstart);
///! All visible elements.	Script to be run when dragged element is being
///dropped
CREATE_ATTRIBUTE(ondrop);
///! <audio>, <video>	Script to be run when the length of the media changes
CREATE_ATTRIBUTE(ondurationchange);
///! <audio>, <video>	Script to be run when something bad happens and the file
///is suddenly unavailable (like unexpectedly disconnects)
CREATE_ATTRIBUTE(onemptied);
///! <audio>, <video>	Script to be run when the media has reach the end (a
///useful event for messages like "thanks for listening")
CREATE_ATTRIBUTE(onended);
///! <audio>, <body>, <embed>, <img>, <object>, <script>, <style>, <video>
///Script to be run when an error occurs
CREATE_ATTRIBUTE(onerror);
///! All visible elements.	Script to be run when the element gets focus
CREATE_ATTRIBUTE(onfocus);
///! <body>	Script to be run when there has been changes to the anchor part
///of the a URL
CREATE_ATTRIBUTE(onhashchange);
///! All visible elements.	Script to be run when the element gets user
///input
CREATE_ATTRIBUTE(oninput);
///! All visible elements.	Script to be run when the element is invalid
CREATE_ATTRIBUTE(oninvalid);
///! All visible elements.	Script to be run when a user is pressing a key
CREATE_ATTRIBUTE(onkeydown);
///! All visible elements.	Script to be run when a user presses a key
CREATE_ATTRIBUTE(onkeypress);
///! All visible elements.	Script to be run when a user releases a key
CREATE_ATTRIBUTE(onkeyup);
///! <body>, <iframe>, <img>, <input>, <link>, <script>, <style>	Script
///to be run when the element is finished loading
CREATE_ATTRIBUTE(onload);
///! <audio>, <video>	Script to be run when media data is loaded
CREATE_ATTRIBUTE(onloadeddata);
///! <audio>, <video>	Script to be run when meta data (like dimensions and
///duration) are loaded
CREATE_ATTRIBUTE(onloadedmetadata);
///! <audio>, <video>	Script to be run just as the file begins to load before
///anything is actually loaded
CREATE_ATTRIBUTE(onloadstart);
///! All visible elements.	Script to be run when a mouse button is pressed
///down on an element
CREATE_ATTRIBUTE(onmousedown);
///! All visible elements.	Script to be run as long as the  mouse pointer
///is moving over an element
CREATE_ATTRIBUTE(onmousemove);
///! All visible elements.	Script to be run when a mouse pointer moves out
///of an element
CREATE_ATTRIBUTE(onmouseout);
///! All visible elements.	Script to be run when a mouse pointer moves over
///an element
CREATE_ATTRIBUTE(onmouseover);
///! All visible elements.	Script to be run when a mouse button is released
///over an element
CREATE_ATTRIBUTE(onmouseup);
///! All visible elements.	Script to be run when a mouse wheel is being
///scrolled over an element
CREATE_ATTRIBUTE(onmousewheel);
///! <body>	Script to be run when the browser starts to work offline
CREATE_ATTRIBUTE(onoffline);
///! <body>	Script to be run when the browser starts to work online
CREATE_ATTRIBUTE(ononline);
///! <body>	Script to be run when a user navigates away from a page
CREATE_ATTRIBUTE(onpagehide);
///! <body>	Script to be run when a user navigates to a page
CREATE_ATTRIBUTE(onpageshow);
///! All visible elements.	Script to be run when the user pastes some
///content in an element
CREATE_ATTRIBUTE(onpaste);
///! <audio>, <video>	Script to be run when the media is paused either by the
///user or programmatically
CREATE_ATTRIBUTE(onpause);
///! <audio>, <video>	Script to be run when the media has started playing
CREATE_ATTRIBUTE(onplay);
///! <audio>, <video>	Script to be run when the media has started playing
CREATE_ATTRIBUTE(onplaying);
///! <body>	Script to be run when the window's history changes.
CREATE_ATTRIBUTE(onpopstate);
///! <audio>, <video>	Script to be run when the browser is in the process of
///getting the media data
CREATE_ATTRIBUTE(onprogress);
///! <audio>, <video>	Script to be run each time the playback rate changes
///(like when a user switches to a slow motion or fast forward mode).
CREATE_ATTRIBUTE(onratechange);
///! <form>	Script to be run when a reset button in a form is clicked.
CREATE_ATTRIBUTE(onreset);
///! <body>	Script to be run when the browser window is being resized.
CREATE_ATTRIBUTE(onresize);
///! All visible elements.	Script to be run when an element's scrollbar is
///being scrolled
CREATE_ATTRIBUTE(onscroll);
///! <input>	Script to be run when the user writes something in a search
///field (for <input type="search">)
CREATE_ATTRIBUTE(onsearch);
///! <audio>, <video>	Script to be run when the seeking attribute is set to
///false indicating that seeking has ended
CREATE_ATTRIBUTE(onseeked);
///! <audio>, <video>	Script to be run when the seeking attribute is set to
///true indicating that seeking is active
CREATE_ATTRIBUTE(onseeking);
///! All visible elements.	Script to be run when the element gets selected
CREATE_ATTRIBUTE(onselect);
///! <audio>, <video>	Script to be run when the browser is unable to fetch the
///media data for whatever reason
CREATE_ATTRIBUTE(onstalled);
///! <body>	Script to be run when a Web Storage area is updated
CREATE_ATTRIBUTE(onstorage);
///! <form>	Script to be run when a form is submitted
CREATE_ATTRIBUTE(onsubmit);
///! <audio>, <video>	Script to be run when fetching the media data is stopped
///before it is completely loaded for whatever reason
CREATE_ATTRIBUTE(onsuspend);
///! <audio>, <video>	Script to be run when the playing position has changed
///(like when the user fast forwards to a different point in the media)
CREATE_ATTRIBUTE(ontimeupdate);
///! <details>	Script to be run when the user opens or closes the <details>
///element
CREATE_ATTRIBUTE(ontoggle);
///! <body>	Script to be run when a page has unloaded (or the browser window
///has been closed)
CREATE_ATTRIBUTE(onunload);
///! <audio>, <video>	Script to be run each time the volume of a video/audio
///has been changed
CREATE_ATTRIBUTE(onvolumechange);
///! <audio>, <video>	Script to be run when the media has paused but is
///expected to resume (like when the media pauses to buffer more data)
CREATE_ATTRIBUTE(onwaiting);
///! All visible elements.	Script to be run when the mouse wheel rolls up
///or down over an element
CREATE_ATTRIBUTE(onwheel);
///! <details>	Specifies that the details should be visible (open) to the user
CREATE_ATTRIBUTE(open);
///! <meter>	Specifies what value is the optimal value for the gauge
CREATE_ATTRIBUTE(optimum);
///! <input>	Specifies a regular expression that an <input> element's value
///is checked against
CREATE_ATTRIBUTE(pattern);
///! <input>, <textarea>	Specifies a short hint that describes the
///expected value of the element
CREATE_ATTRIBUTE(placeholder);
///! Global Attributes	Specifies a popover element
CREATE_ATTRIBUTE(popover);
///! <button>, <input>	Specifies which popover element to invoked
CREATE_ATTRIBUTE(popovertarget);
///! <button>, <input>	Specifies what happens to the popover element when the
///button is clicked
CREATE_ATTRIBUTE(popovertargetaction);
///! <video>	Specifies an image to be shown while the video is downloading,
///or until the user hits the play button
CREATE_ATTRIBUTE(poster);
///! <audio>, <video>	Specifies if and how the author thinks the audio/video
///should be loaded when the page loads
CREATE_ATTRIBUTE(preload);
///! <input>, <textarea>	Specifies that the element is read-only
CREATE_ATTRIBUTE(readonly);
///! <a>, <area>, <form>, <link>	Specifies the relationship between the
///current document and the linked document
CREATE_ATTRIBUTE(rel);
///! <input>, <select>, <textarea>	Specifies that the element must be
///filled out before submitting the form
CREATE_ATTRIBUTE(required);
///! <ol>	Specifies that the list order should be descending (9,8,7...)
CREATE_ATTRIBUTE(reversed);
///! <textarea>	Specifies the visible number of lines in a text area
CREATE_ATTRIBUTE(rows);
///! <td>, <th>	Specifies the number of rows a table cell should span
CREATE_ATTRIBUTE(rowspan);
///! <iframe>	Enables an extra set of restrictions for the content in an
///<iframe>
CREATE_ATTRIBUTE(sandbox);
///! <th>	Specifies whether a header cell is a header for a column, row,
///or group of columns or rows
CREATE_ATTRIBUTE(scope);
///! <option>	Specifies that an option should be pre-selected when the page
///loads
CREATE_ATTRIBUTE(selected);
///! <area>	Specifies the shape of the area
CREATE_ATTRIBUTE(shape);
///! <input>, <select>	Specifies the width, in characters (for <input>) or
///specifies the number of visible options (for <select>)
CREATE_ATTRIBUTE(size);
///! <img>, <link>, <source>	Specifies the size of the linked resource
CREATE_ATTRIBUTE(sizes);
///! <col>, <colgroup>	Specifies the number of columns to span
CREATE_ATTRIBUTE(span);
///! Global Attributes	Specifies whether the element is to have its spelling
///and grammar checked or not
CREATE_ATTRIBUTE(spellcheck);
///! <audio>, <embed>, <iframe>, <img>, <input>, <script>, <source>, <track>,
///<video>	Specifies the URL of the media file
CREATE_ATTRIBUTE(src);
///! <iframe>	Specifies the HTML content of the page to show in the <iframe>
CREATE_ATTRIBUTE(srcdoc);
///! <track>	Specifies the language of the track text data (required if
///kind="subtitles")
CREATE_ATTRIBUTE(srclang);
///! <img>, <source>	Specifies the URL of the image to use in different
///situations
CREATE_ATTRIBUTE(srcset);
///! <ol>	Specifies the start value of an ordered list
CREATE_ATTRIBUTE(start);
///! <input>	Specifies the legal number intervals for an input field
CREATE_ATTRIBUTE(step);
///! Global Attributes	Specifies an inline CSS style for an element
CREATE_ATTRIBUTE(style);
///! Global Attributes	Specifies the tabbing order of an element
CREATE_ATTRIBUTE(tabindex);
///! <a>, <area>, <base>, <form>	Specifies the target for where to open
///the linked document or where to submit the form
CREATE_ATTRIBUTE(target);
///! Global Attributes	Specifies extra information about an element
CREATE_ATTRIBUTE(title);
///! Global Attributes	Specifies whether the content of an element should be
///translated or not
CREATE_ATTRIBUTE(translate);
///! <a>, <button>, <embed>, <input>, <link>, <menu>, <object>, <script>,
///<source>, <style>	Specifies the type of element
CREATE_ATTRIBUTE(type);
///! <img>, <object>	Specifies an image as a client-side image map
CREATE_ATTRIBUTE(usemap);
///! <button>, <input>, <li>, <option>, <meter>, <progress>, <param>
///Specifies the value of the element
CREATE_ATTRIBUTE(value);
///! <canvas>, <embed>, <iframe>, <img>, <input>, <object>, <video>
///Specifies the width of the element
CREATE_ATTRIBUTE(width);
///! <textarea>	Specifies how the text in a text area is to be wrapped when
///submitted in a form
CREATE_ATTRIBUTE(wrap);

///! Meta Info: Specifies the base URL/target for all relative URLs in a
///document
CREATE_BODY_INLINE_COMPONENT(base);
///! Meta Info: Not supported in HTML5. Use CSS instead. Specifies a default
///color, size, and font for all text in a document
CREATE_BODY_BLOCK_COMPONENT(basefont);

///! Programming: Defines a client-side script
CREATE_BODY_BLOCK_COMPONENT(script);
///! Programming: Defines an alternate content for users that do not support
///client-side scripts
CREATE_BODY_BLOCK_COMPONENT(noscript);
///! Programming: Not supported in HTML5. Useembed orobject instead. Defines an
///embedded applet
CREATE_BODY_BLOCK_COMPONENT(applet);
///! Programming: Defines a container for an external (non-HTML) application
CREATE_BODY_BLOCK_COMPONENT(embed);
///! Programming: Defines an embedded object
CREATE_BODY_BLOCK_COMPONENT(object);
///! Programming: Defines a parameter for an object
CREATE_BODY_BLOCK_COMPONENT(param);

template <const_string _text> class text : body_element {
public:
  std::string &content;
  text(std::string &content) : content(content) { content += _text.c_str(); }
  operator std::string() const { return content; }
};

template <const_string _text> class $ : body_element {
public:
  std::string &content;
  $(std::string &content) : content(content) {
    content += "<<";
    content += _text.c_str();
    content += ">>";
  }
  operator std::string() const { return content; }
};

namespace css {
#define DEFINE_CSS_PROP(NAME, KEY)                                             \
  inline const std::pair<std::string, std::string> NAME(std::string value) {   \
    return {KEY, value};                                                       \
  }

///! Specifies an accent color for user-interface controls
DEFINE_CSS_PROP(accent_color, "accent-color");
///! Specifies the alignment between the lines inside a flexible container when
/// the items do not use all available space
DEFINE_CSS_PROP(align_content, "align-content");
///! Specifies the alignment for items inside a flexible container
DEFINE_CSS_PROP(align_items, "align-items");
///! Specifies the alignment for selected items inside a flexible container
DEFINE_CSS_PROP(align_self, "align-self");
///! Resets all properties (except unicode-bidi and direction)
DEFINE_CSS_PROP(all, "all");
///! A shorthand property for all the animation-* properties
DEFINE_CSS_PROP(animation, "animation");
///! Specifies a delay for the start of an animation
DEFINE_CSS_PROP(animation_delay, "animation-delay");
///! Specifies whether an animation should be played forwards, backwards or in
/// alternate cycles
DEFINE_CSS_PROP(animation_direction, "animation-direction");
///! Specifies how long an animation should take to complete one cycle
DEFINE_CSS_PROP(animation_duration, "animation-duration");
///! Specifies a style for the element when the animation is not playing (before
/// it starts, after it ends, or both)
DEFINE_CSS_PROP(animation_fill_mode, "animation-fill-mode");
///! Specifies the number of times an animation should be played
DEFINE_CSS_PROP(animation_iteration_count, "animation-iteration-count");
///! Specifies a name for the @keyframes animation
DEFINE_CSS_PROP(animation_name, "animation-name");
///! Specifies whether the animation is running or paused
DEFINE_CSS_PROP(animation_play_state, "animation-play-state");
///! Specifies the speed curve of an animation
DEFINE_CSS_PROP(animation_timing_function, "animation-timing-function");
///! Specifies preferred aspect ratio of an element
DEFINE_CSS_PROP(aspect_ratio, "aspect-ratio");
///! Defines a graphical effect to the area behind an element
DEFINE_CSS_PROP(backdrop_filter, "backdrop-filter");
///! Defines whether or not the back face of an element should be visible when
/// facing the user
DEFINE_CSS_PROP(backface_visibility, "backface-visibility");
///! A shorthand property for all the background-* properties
DEFINE_CSS_PROP(background, "background");
///! Sets whether a background image scrolls with the rest of the page, or is
/// fixed
DEFINE_CSS_PROP(background_attachment, "background-attachment");
///! Specifies the blending mode of each background layer (color/image)
DEFINE_CSS_PROP(background_blend_mode, "background-blend-mode");
///! Defines how far the background (color or image) should extend within an
/// element
DEFINE_CSS_PROP(background_clip, "background-clip");
///! Specifies the background color of an element
DEFINE_CSS_PROP(background_color, "background-color");
///! Specifies one or more background images for an element
DEFINE_CSS_PROP(background_image, "background-image");
///! Specifies the origin position of a background image
DEFINE_CSS_PROP(background_origin, "background-origin");
///! Specifies the position of a background image
DEFINE_CSS_PROP(background_position, "background-position");
///! Specifies the position of a background image on x-axis
DEFINE_CSS_PROP(background_position_x, "background-position-x");
///! Specifies the position of a background image on y-axis
DEFINE_CSS_PROP(background_position_y, "background-position-y");
///! Sets if/how a background image will be repeated
DEFINE_CSS_PROP(background_repeat, "background-repeat");
///! Specifies the size of the background images
DEFINE_CSS_PROP(background_size, "background-size");
///! Specifies the size of an element in block direction
DEFINE_CSS_PROP(block_size, "block-size");
///! A shorthand property for border-width, border-style and border-color
DEFINE_CSS_PROP(border, "border");
///! A shorthand property for border-block-width, border-block-style and
/// border-block-color
DEFINE_CSS_PROP(border_block, "border-block");
///! Sets the color of the borders at start and end in the block direction
DEFINE_CSS_PROP(border_block_color, "border-block-color");
///! A shorthand property for border-block-end-width, border-block-end-style and
/// border-block-end-color
DEFINE_CSS_PROP(border_block_end, "border-block-end");
///! Sets the color of the border at the end in the block direction
DEFINE_CSS_PROP(border_block_end_color, "border-block-end-color");
///! Sets the style of the border at the end in the block direction
DEFINE_CSS_PROP(border_block_end_style, "border-block-end-style");
///! Sets the width of the border at the end in the block direction
DEFINE_CSS_PROP(border_block_end_width, "border-block-end-width");
///! A shorthand property for border-block-start-width, border-block-start-style
/// and border-block-start-color
DEFINE_CSS_PROP(border_block_start, "border-block-start");
///! Sets the color of the border at the start in the block direction
DEFINE_CSS_PROP(border_block_start_color, "border-block-start-color");
///! Sets the style of the border at the start in the block direction
DEFINE_CSS_PROP(border_block_start_style, "border-block-start-style");
///! Sets the width of the border at the start in the block direction
DEFINE_CSS_PROP(border_block_start_width, "border-block-start-width");
///! Sets the style of the borders at start and end in the block direction
DEFINE_CSS_PROP(border_block_style, "border-block-style");
///! Sets the width of the borders at start and end in the block direction
DEFINE_CSS_PROP(border_block_width, "border-block-width");
///! A shorthand property for border-bottom-width, border-bottom-style and
/// border-bottom-color
DEFINE_CSS_PROP(border_bottom, "border-bottom");
///! Sets the color of the bottom border
DEFINE_CSS_PROP(border_bottom_color, "border-bottom-color");
///! Defines the radius of the border of the bottom-left corner
DEFINE_CSS_PROP(border_bottom_left_radius, "border-bottom-left-radius");
///! Defines the radius of the border of the bottom-right corner
DEFINE_CSS_PROP(border_bottom_right_radius, "border-bottom-right-radius");
///! Sets the style of the bottom border
DEFINE_CSS_PROP(border_bottom_style, "border-bottom-style");
///! Sets the width of the bottom border
DEFINE_CSS_PROP(border_bottom_width, "border-bottom-width");
///! Sets whether table borders should collapse into a single border or be
/// separated
DEFINE_CSS_PROP(border_collapse, "border-collapse");
///! Sets the color of the four borders
DEFINE_CSS_PROP(border_color, "border-color");
///! Sets the radius of the corner between the block-end and the inline-end
/// sides of the element
DEFINE_CSS_PROP(border_end_end_radius, "border-end-end-radius");
///! Sets the radius of the corner between the block-end and the inline-start
/// sides of the element
DEFINE_CSS_PROP(border_end_start_radius, "border-end-start-radius");
///! A shorthand property for all the border-image-* properties
DEFINE_CSS_PROP(border_image, "border-image");
///! Specifies the amount by which the border image area extends beyond the
/// border box
DEFINE_CSS_PROP(border_image_outset, "border-image-outset");
///! Specifies whether the border image should be repeated, rounded or stretched
DEFINE_CSS_PROP(border_image_repeat, "border-image-repeat");
///! Specifies how to slice the border image
DEFINE_CSS_PROP(border_image_slice, "border-image-slice");
///! Specifies the path to the image to be used as a border
DEFINE_CSS_PROP(border_image_source, "border-image-source");
///! Specifies the width of the border image
DEFINE_CSS_PROP(border_image_width, "border-image-width");
///! A shorthand property for border-inline-width, border-inline-style and
/// border-inline-color
DEFINE_CSS_PROP(border_inline, "border-inline");
///! Sets the color of the borders at start and end in the inline direction
DEFINE_CSS_PROP(border_inline_color, "border-inline-color");
///! A shorthand property for border-inline-end-width, border-inline-end-style
/// and border-inline-end-color
DEFINE_CSS_PROP(border_inline_end, "border-inline-end");
///! Sets the color of the border at the end in the inline direction
DEFINE_CSS_PROP(border_inline_end_color, "border-inline-end-color");
///! Sets the style of the border at the end in the inline direction
DEFINE_CSS_PROP(border_inline_end_style, "border-inline-end-style");
///! Sets the width of the border at the end in the inline direction
DEFINE_CSS_PROP(border_inline_end_width, "border-inline-end-width");
///! A shorthand property for border-inline-start-width,
/// border-inline-start-style and border-inline-start-color
DEFINE_CSS_PROP(border_inline_start, "border-inline-start");
///! Sets the color of the border at the start in the inline direction
DEFINE_CSS_PROP(border_inline_start_color, "border-inline-start-color");
///! Sets the style of the border at the start in the inline direction
DEFINE_CSS_PROP(border_inline_start_style, "border-inline-start-style");
///! Sets the width of the border at the start in the inline direction
DEFINE_CSS_PROP(border_inline_start_width, "border-inline-start-width");
///! Sets the style of the borders at start and end in the inline direction
DEFINE_CSS_PROP(border_inline_style, "border-inline-style");
///! Sets the width of the borders at start and end in the inline direction
DEFINE_CSS_PROP(border_inline_width, "border-inline-width");
///! A shorthand property for all the border-left-* properties
DEFINE_CSS_PROP(border_left, "border-left");
///! Sets the color of the left border
DEFINE_CSS_PROP(border_left_color, "border-left-color");
///! Sets the style of the left border
DEFINE_CSS_PROP(border_left_style, "border-left-style");
///! Sets the width of the left border
DEFINE_CSS_PROP(border_left_width, "border-left-width");
///! A shorthand property for the four border-*-radius properties
DEFINE_CSS_PROP(border_radius, "border-radius");
///! A shorthand property for all the border-right-* properties
DEFINE_CSS_PROP(border_right, "border-right");
///! Sets the color of the right border
DEFINE_CSS_PROP(border_right_color, "border-right-color");
///! Sets the style of the right border
DEFINE_CSS_PROP(border_right_style, "border-right-style");
///! Sets the width of the right border
DEFINE_CSS_PROP(border_right_width, "border-right-width");
///! Sets the distance between the borders of adjacent cells
DEFINE_CSS_PROP(border_spacing, "border-spacing");
///! Sets the radius of the corner between the block-start and the inline-end
/// sides of the element
DEFINE_CSS_PROP(border_start_end_radius, "border-start-end-radius");
///! Sets the radius of the corner between the block-start and the inline-start
/// sides of the element
DEFINE_CSS_PROP(border_start_start_radius, "border-start-start-radius");
///! Sets the style of the four borders
DEFINE_CSS_PROP(border_style, "border-style");
///! A shorthand property for border-top-width, border-top-style and
/// border-top-color
DEFINE_CSS_PROP(border_top, "border-top");
///! Sets the color of the top border
DEFINE_CSS_PROP(border_top_color, "border-top-color");
///! Defines the radius of the border of the top-left corner
DEFINE_CSS_PROP(border_top_left_radius, "border-top-left-radius");
///! Defines the radius of the border of the top-right corner
DEFINE_CSS_PROP(border_top_right_radius, "border-top-right-radius");
///! Sets the style of the top border
DEFINE_CSS_PROP(border_top_style, "border-top-style");
///! Sets the width of the top border
DEFINE_CSS_PROP(border_top_width, "border-top-width");
///! Sets the width of the four borders
DEFINE_CSS_PROP(border_width, "border-width");
///! Sets the elements position, from the bottom of its parent element
DEFINE_CSS_PROP(bottom, "bottom");
///! Sets the behavior of the background and border of an element at page-break,
/// or, for in-line elements, at line-break.
DEFINE_CSS_PROP(box_decoration_break, "box-decoration-break");
///! The box-reflect property is used to create a reflection of an element.
DEFINE_CSS_PROP(box_reflect, "box-reflect");
///! Attaches one or more shadows to an element
DEFINE_CSS_PROP(box_shadow, "box-shadow");
///! Defines how the width and height of an element are calculated: should they
/// include padding and borders, or not
DEFINE_CSS_PROP(box_sizing, "box-sizing");
///! Specifies whether or not a page-, column-, or region-break should occur
/// after the specified element
DEFINE_CSS_PROP(break_after, "break-after");
///! Specifies whether or not a page-, column-, or region-break should occur
/// before the specified element
DEFINE_CSS_PROP(break_before, "break-before");
///! Specifies whether or not a page-, column-, or region-break should occur
/// inside the specified element
DEFINE_CSS_PROP(break_inside, "break-inside");
///! Specifies the placement of a table caption
DEFINE_CSS_PROP(caption_side, "caption-side");
///! Specifies the color of the cursor (caret) in inputs, textareas, or any
/// element that is editable
DEFINE_CSS_PROP(caret_color, "caret-color");
///! Specifies the character encoding used in the style sheet
DEFINE_CSS_PROP(at_charset, "@charset");
///! Specifies what should happen with the element that is next to a floating
/// element
DEFINE_CSS_PROP(clear, "clear");
///! Clips an absolutely positioned element
DEFINE_CSS_PROP(clip, "clip");
///! Clips an element to a basic shape or to an SVG source
DEFINE_CSS_PROP(clip_path, "clip-path");
///! Sets the color of text
DEFINE_CSS_PROP(color, "color");
///! Specifies the number of columns an element should be divided into
DEFINE_CSS_PROP(column_count, "column-count");
///! Specifies how to fill columns, balanced or not
DEFINE_CSS_PROP(column_fill, "column-fill");
///! Specifies the gap between the columns
DEFINE_CSS_PROP(column_gap, "column-gap");
///! A shorthand property for all the column-rule-* properties
DEFINE_CSS_PROP(column_rule, "column-rule");
///! Specifies the color of the rule between columns
DEFINE_CSS_PROP(column_rule_color, "column-rule-color");
///! Specifies the style of the rule between columns
DEFINE_CSS_PROP(column_rule_style, "column-rule-style");
///! Specifies the width of the rule between columns
DEFINE_CSS_PROP(column_rule_width, "column-rule-width");
///! Specifies how many columns an element should span across
DEFINE_CSS_PROP(column_span, "column-span");
///! Specifies the column width
DEFINE_CSS_PROP(column_width, "column-width");
///! A shorthand property for column-width and column-count
DEFINE_CSS_PROP(columns, "columns");
///! Used with the :before and :after pseudo-elements, to insert generated
/// content
DEFINE_CSS_PROP(content, "content");
///! Increases or decreases the value of one or more CSS counters
DEFINE_CSS_PROP(counter_increment, "counter-increment");
///! Creates or resets one or more CSS counters
DEFINE_CSS_PROP(counter_reset, "counter-reset");
///! Creates or sets one or more CSS counters
DEFINE_CSS_PROP(counter_set, "counter-set");
///! Specifies the mouse cursor to be displayed when pointing over an element
DEFINE_CSS_PROP(cursor, "cursor");
///! Specifies the text direction/writing direction
DEFINE_CSS_PROP(direction, "direction");
///! Specifies how a certain HTML element should be displayed
DEFINE_CSS_PROP(display, "display");
///! Specifies whether or not to display borders and background on empty cells
/// in a table
DEFINE_CSS_PROP(empty_cells, "empty-cells");
///! Defines effects (e.g. blurring or color shifting) on an element before the
/// element is displayed
DEFINE_CSS_PROP(filter, "filter");
///! A shorthand property for the flex-grow, flex-shrink, and the flex-basis
/// properties
DEFINE_CSS_PROP(flex, "flex");
///! Specifies the initial length of a flexible item
DEFINE_CSS_PROP(flex_basis, "flex-basis");
///! Specifies the direction of the flexible items
DEFINE_CSS_PROP(flex_direction, "flex-direction");
///! A shorthand property for the flex-direction and the flex-wrap properties
DEFINE_CSS_PROP(flex_flow, "flex-flow");
///! Specifies how much the item will grow relative to the rest
DEFINE_CSS_PROP(flex_grow, "flex-grow");
///! Specifies how the item will shrink relative to the rest
DEFINE_CSS_PROP(flex_shrink, "flex-shrink");
///! Specifies whether the flexible items should wrap or not
DEFINE_CSS_PROP(flex_wrap, "flex-wrap");
///! Specifies whether an element should float to the left, right, or not at all
DEFINE_CSS_PROP(float$, "float");
///! A shorthand property for the font-style, font-variant, font-weight,
/// font-size/line-height, and the font-family properties
DEFINE_CSS_PROP(font, "font");
///! A rule that allows websites to download and use fonts other than the
///"web-safe" fonts
DEFINE_CSS_PROP(at_font_face, "@font-face");
///! Specifies the font family for text
DEFINE_CSS_PROP(font_family, "font-family");
///! Allows control over advanced typographic features in OpenType fonts
DEFINE_CSS_PROP(font_feature_settings, "font-feature-settings");
///! Allows authors to use a common name in font-variant-alternate for feature
/// activated differently in OpenType
DEFINE_CSS_PROP(at_font_feature_values, "@font-feature-values");
///! Controls the usage of the kerning information (how letters are spaced)
DEFINE_CSS_PROP(font_kerning, "font-kerning");
///! Controls the usage of language-specific glyphs in a typeface
DEFINE_CSS_PROP(font_language_override, "font-language-override");
///! Specifies the font size of text
DEFINE_CSS_PROP(font_size, "font-size");
///! Preserves the readability of text when font fallback occurs
DEFINE_CSS_PROP(font_size_adjust, "font-size-adjust");
///! Selects a normal, condensed, or expanded face from a font family
DEFINE_CSS_PROP(font_stretch, "font-stretch");
///! Specifies the font style for text
DEFINE_CSS_PROP(font_style, "font-style");
///! Controls which missing typefaces (bold or italic) may be synthesized by the
/// browser
DEFINE_CSS_PROP(font_synthesis, "font-synthesis");
///! Specifies whether or not a text should be displayed in a small-caps font
DEFINE_CSS_PROP(font_variant, "font-variant");
///! Controls the usage of alternate glyphs associated to alternative names
/// defined in @font-feature-values
DEFINE_CSS_PROP(font_variant_alternates, "font-variant-alternates");
///! Controls the usage of alternate glyphs for capital letters
DEFINE_CSS_PROP(font_variant_caps, "font-variant-caps");
///! Controls the usage of alternate glyphs for East Asian scripts (e.g Japanese
/// and Chinese)
DEFINE_CSS_PROP(font_variant_east_asian, "font-variant-east-asian");
///! Controls which ligatures and contextual forms are used in textual content
/// of the elements it applies to
DEFINE_CSS_PROP(font_variant_ligatures, "font-variant-ligatures");
///! Controls the usage of alternate glyphs for numbers, fractions, and ordinal
/// markers
DEFINE_CSS_PROP(font_variant_numeric, "font-variant-numeric");
///! Controls the usage of alternate glyphs of smaller size positioned as
/// superscript or subscript regarding the baseline of the font
DEFINE_CSS_PROP(font_variant_position, "font-variant-position");
///! Specifies the weight of a font
DEFINE_CSS_PROP(font_weight, "font-weight");
///! A shorthand property for the row-gap and the column-gap properties
DEFINE_CSS_PROP(gap, "gap");
///! A shorthand property for the grid-template-rows, grid-template-columns,
/// grid-template-areas, grid-auto-rows, grid-auto-columns, and the
/// grid-auto-flow properties
DEFINE_CSS_PROP(grid, "grid");
///! Either specifies a name for the grid item, or this property is a shorthand
/// property for the grid-row-start, grid-column-start, grid-row-end, and
/// grid-column-end properties
DEFINE_CSS_PROP(grid_area, "grid-area");
///! Specifies a default column size
DEFINE_CSS_PROP(grid_auto_columns, "grid-auto-columns");
///! Specifies how auto-placed items are inserted in the grid
DEFINE_CSS_PROP(grid_auto_flow, "grid-auto-flow");
///! Specifies a default row size
DEFINE_CSS_PROP(grid_auto_rows, "grid-auto-rows");
///! A shorthand property for the grid-column-start and the grid-column-end
/// properties
DEFINE_CSS_PROP(grid_column, "grid-column");
///! Specifies where to end the grid item
DEFINE_CSS_PROP(grid_column_end, "grid-column-end");
///! Specifies the size of the gap between columns
DEFINE_CSS_PROP(grid_column_gap, "grid-column-gap");
///! Specifies where to start the grid item
DEFINE_CSS_PROP(grid_column_start, "grid-column-start");
///! A shorthand property for the grid-row-gap and grid-column-gap properties
DEFINE_CSS_PROP(grid_gap, "grid-gap");
///! A shorthand property for the grid-row-start and the grid-row-end properties
DEFINE_CSS_PROP(grid_row, "grid-row");
///! Specifies where to end the grid item
DEFINE_CSS_PROP(grid_row_end, "grid-row-end");
///! Specifies the size of the gap between rows
DEFINE_CSS_PROP(grid_row_gap, "grid-row-gap");
///! Specifies where to start the grid item
DEFINE_CSS_PROP(grid_row_start, "grid-row-start");
///! A shorthand property for the grid-template-rows, grid-template-columns and
/// grid-areas properties
DEFINE_CSS_PROP(grid_template, "grid-template");
///! Specifies how to display columns and rows, using named grid items
DEFINE_CSS_PROP(grid_template_areas, "grid-template-areas");
///! Specifies the size of the columns, and how many columns in a grid layout
DEFINE_CSS_PROP(grid_template_columns, "grid-template-columns");
///! Specifies the size of the rows in a grid layout
DEFINE_CSS_PROP(grid_template_rows, "grid-template-rows");
///! Specifies whether a punctuation character may be placed outside the line
/// box
DEFINE_CSS_PROP(hanging_punctuation, "hanging-punctuation");
///! Sets the height of an element
DEFINE_CSS_PROP(height, "height");
///! Sets how to split words to improve the layout of text
DEFINE_CSS_PROP(hyphens, "hyphens");
///! Sets the character used at the end of line, before a hyphenation break
DEFINE_CSS_PROP(hypenate_character, "hypenate-character");
///! Specifies the type of algorithm to use for image scaling
DEFINE_CSS_PROP(image_rendering, "image-rendering");
///! Allows you to import a style sheet into another style sheet
DEFINE_CSS_PROP(at_import, "@import");
///! Specifies the size of an element in the inline direction
DEFINE_CSS_PROP(inline_size, "inline-size");
///! Specifies the distance between an element and the parent element
DEFINE_CSS_PROP(inset, "inset");
///! Specifies the distance between an element and the parent element in the
/// block direction
DEFINE_CSS_PROP(inset_block, "inset-block");
///! Specifies the distance between the end of an element and the parent element
/// in the block direction
DEFINE_CSS_PROP(inset_block_end, "inset-block-end");
///! Specifies the distance between the start of an element and the parent
/// element in the block direction
DEFINE_CSS_PROP(inset_block_start, "inset-block-start");
///! Specifies the distance between an element and the parent element in the
/// inline direction
DEFINE_CSS_PROP(inset_inline, "inset-inline");
///! Specifies the distance between the end of an element and the parent element
/// in the inline direction
DEFINE_CSS_PROP(inset_inline_end, "inset-inline-end");
///! Specifies the distance between the start of an element and the parent
/// element in the inline direction
DEFINE_CSS_PROP(inset_inline_start, "inset-inline-start");
///! Specifies the alignment between the items inside a flexible container when
/// the items do not use all available space
DEFINE_CSS_PROP(justify_content, "justify-content");
///! Is set on the grid container. Specifies the alignment of grid items in the
/// inline direction
DEFINE_CSS_PROP(justify_items, "justify-items");
///! Is set on the grid item. Specifies the alignment of the grid item in the
/// inline direction
DEFINE_CSS_PROP(justify_self, "justify-self");
///! Specifies the animation code
DEFINE_CSS_PROP(at_keyframes, "@keyframes");
///! Specifies the left position of a positioned element
DEFINE_CSS_PROP(left, "left");
///! Increases or decreases the space between characters in a text
DEFINE_CSS_PROP(letter_spacing, "letter-spacing");
///! Specifies how/if to break lines
DEFINE_CSS_PROP(line_break, "line-break");
///! Sets the line height
DEFINE_CSS_PROP(line_height, "line-height");
///! Sets all the properties for a list in one declaration
DEFINE_CSS_PROP(list_style, "list-style");
///! Specifies an image as the list-item marker
DEFINE_CSS_PROP(list_style_image, "list-style-image");
///! Specifies the position of the list-item markers (bullet points)
DEFINE_CSS_PROP(list_style_position, "list-style-position");
///! Specifies the type of list-item marker
DEFINE_CSS_PROP(list_style_type, "list-style-type");
///! Sets all the margin properties in one declaration
DEFINE_CSS_PROP(margin, "margin");
///! Specifies the margin in the block direction
DEFINE_CSS_PROP(margin_block, "margin-block");
///! Specifies the margin at the end in the block direction
DEFINE_CSS_PROP(margin_block_end, "margin-block-end");
///! Specifies the margin at the start in the block direction
DEFINE_CSS_PROP(margin_block_start, "margin-block-start");
///! Sets the bottom margin of an element
DEFINE_CSS_PROP(margin_bottom, "margin-bottom");
///! Specifies the margin in the inline direction
DEFINE_CSS_PROP(margin_inline, "margin-inline");
///! Specifies the margin at the end in the inline direction
DEFINE_CSS_PROP(margin_inline_end, "margin-inline-end");
///! Specifies the margin at the start in the inline direction
DEFINE_CSS_PROP(margin_inline_start, "margin-inline-start");
///! Sets the left margin of an element
DEFINE_CSS_PROP(margin_left, "margin-left");
///! Sets the right margin of an element
DEFINE_CSS_PROP(margin_right, "margin-right");
///! Sets the top margin of an element
DEFINE_CSS_PROP(margin_top, "margin-top");
///! Hides parts of an element by masking or clipping an image at specific
/// places
DEFINE_CSS_PROP(mask, "mask");
///! Specifies the mask area
DEFINE_CSS_PROP(mask_clip, "mask-clip");
///! Represents a compositing operation used on the current mask layer with the
/// mask layers below it
DEFINE_CSS_PROP(mask_composite, "mask-composite");
///! Specifies an image to be used as a mask layer for an element
DEFINE_CSS_PROP(mask_image, "mask-image");
///! Specifies whether the mask layer image is treated as a luminance mask or as
/// an alpha mask
DEFINE_CSS_PROP(mask_mode, "mask-mode");
///! Specifies the origin position (the mask position area) of a mask layer
/// image
DEFINE_CSS_PROP(mask_origin, "mask-origin");
///! Sets the starting position of a mask layer image (relative to the mask
/// position area)
DEFINE_CSS_PROP(mask_position, "mask-position");
///! Specifies how the mask layer image is repeated
DEFINE_CSS_PROP(mask_repeat, "mask-repeat");
///! Specifies the size of a mask layer image
DEFINE_CSS_PROP(mask_size, "mask-size");
///! Specifies whether an SVG <mask> element is treated as a luminance mask or
/// as an alpha mask
DEFINE_CSS_PROP(mask_type, "mask-type");
///! Sets the maximum height of an element
DEFINE_CSS_PROP(max_height, "max-height");
///! Sets the maximum width of an element
DEFINE_CSS_PROP(max_width, "max-width");
///! Sets the style rules for different media types/devices/sizes
DEFINE_CSS_PROP(at_media, "@media");
///! Sets the maximum size of an element in the block direction
DEFINE_CSS_PROP(max_block_size, "max-block-size");
///! Sets the maximum size of an element in the inline direction
DEFINE_CSS_PROP(max_inline_size, "max-inline-size");
///! Sets the minimum size of an element in the block direction
DEFINE_CSS_PROP(min_block_size, "min-block-size");
///! Sets the minimum size of an element in the inline direction
DEFINE_CSS_PROP(min_inline_size, "min-inline-size");
///! Sets the minimum height of an element
DEFINE_CSS_PROP(min_height, "min-height");
///! Sets the minimum width of an element
DEFINE_CSS_PROP(min_width, "min-width");
///! Specifies how an element's content should blend with its direct parent
/// background
DEFINE_CSS_PROP(mix_blend_mode, "mix-blend-mode");
///! Specifies how the contents of a replaced element should be fitted to the
/// box established by its used height and width
DEFINE_CSS_PROP(object_fit, "object-fit");
///! Specifies the alignment of the replaced element inside its box
DEFINE_CSS_PROP(object_position, "object-position");
///! Is a shorthand, and specifies how to animate an element along a path
DEFINE_CSS_PROP(offset, "offset");
///! Specifies a point on an element that is fixed to the path it is animated
/// along
DEFINE_CSS_PROP(offset_anchor, "offset-anchor");
///! Specifies the position along a path where an animated element is placed
DEFINE_CSS_PROP(offset_distance, "offset-distance");
///! Specifies the path an element is animated along
DEFINE_CSS_PROP(offset_path, "offset-path");
///! Specifies rotation of an element as it is animated along a path
DEFINE_CSS_PROP(offset_rotate, "offset-rotate");
///! Sets the opacity level for an element
DEFINE_CSS_PROP(opacity, "opacity");
///! Sets the order of the flexible item, relative to the rest
DEFINE_CSS_PROP(order, "order");
///! Sets the minimum number of lines that must be left at the bottom of a page
/// or column
DEFINE_CSS_PROP(orphans, "orphans");
///! A shorthand property for the outline-width, outline-style, and the
/// outline-color properties
DEFINE_CSS_PROP(outline, "outline");
///! Sets the color of an outline
DEFINE_CSS_PROP(outline_color, "outline-color");
///! Offsets an outline, and draws it beyond the border edge
DEFINE_CSS_PROP(outline_offset, "outline-offset");
///! Sets the style of an outline
DEFINE_CSS_PROP(outline_style, "outline-style");
///! Sets the width of an outline
DEFINE_CSS_PROP(outline_width, "outline-width");
///! Specifies what happens if content overflows an element's box
DEFINE_CSS_PROP(overflow, "overflow");
///! Specifies whether or not content in viewable area in a scrollable contianer
/// should be pushed down when new content is loaded above
DEFINE_CSS_PROP(overflow_anchor, "overflow-anchor");
///! Specifies whether or not the browser can break lines with long words, if
/// they overflow the container
DEFINE_CSS_PROP(overflow_wrap, "overflow-wrap");
///! Specifies whether or not to clip the left/right edges of the content, if it
/// overflows the element's content area
DEFINE_CSS_PROP(overflow_x, "overflow-x");
///! Specifies whether or not to clip the top/bottom edges of the content, if it
/// overflows the element's content area
DEFINE_CSS_PROP(overflow_y, "overflow-y");
///! Specifies whether to have scroll chaining or overscroll affordance in x-
/// and y-directions
DEFINE_CSS_PROP(overscroll_behavior, "overscroll-behavior");
///! Specifies whether to have scroll chaining or overscroll affordance in the
/// block direction
DEFINE_CSS_PROP(overscroll_behavior_block, "overscroll-behavior-block");
///! Specifies whether to have scroll chaining or overscroll affordance in the
/// inline direction
DEFINE_CSS_PROP(overscroll_behavior_inline, "overscroll-behavior-inline");
///! Specifies whether to have scroll chaining or overscroll affordance in
/// x-direction
DEFINE_CSS_PROP(overscroll_behavior_x, "overscroll-behavior-x");
///! Specifies whether to have scroll chaining or overscroll affordance in
/// y-directions
DEFINE_CSS_PROP(overscroll_behavior_y, "overscroll-behavior-y");
///! A shorthand property for all the padding-* properties
DEFINE_CSS_PROP(padding, "padding");
///! Specifies the padding in the block direction
DEFINE_CSS_PROP(padding_block, "padding-block");
///! Specifies the padding at the end in the block direction
DEFINE_CSS_PROP(padding_block_end, "padding-block-end");
///! Specifies the padding at the start in the block direction
DEFINE_CSS_PROP(padding_block_start, "padding-block-start");
///! Sets the bottom padding of an element
DEFINE_CSS_PROP(padding_bottom, "padding-bottom");
///! Specifies the padding in the inline direction
DEFINE_CSS_PROP(padding_inline, "padding-inline");
///! Specifies the padding at the end in the inline direction
DEFINE_CSS_PROP(padding_inline_end, "padding-inline-end");
///! Specifies the padding at the start in the inline direction
DEFINE_CSS_PROP(padding_inline_start, "padding-inline-start");
///! Sets the left padding of an element
DEFINE_CSS_PROP(padding_left, "padding-left");
///! Sets the right padding of an element
DEFINE_CSS_PROP(padding_right, "padding-right");
///! Sets the top padding of an element
DEFINE_CSS_PROP(padding_top, "padding-top");
///! Sets the page-break behavior after an element
DEFINE_CSS_PROP(page_break_after, "page-break-after");
///! Sets the page-break behavior before an element
DEFINE_CSS_PROP(page_break_before, "page-break-before");
///! Sets the page-break behavior inside an element
DEFINE_CSS_PROP(page_break_inside, "page-break-inside");
///! Sets the order of how an SVG element or text is painted.
DEFINE_CSS_PROP(paint_order, "paint-order");
///! Gives a 3D-positioned element some perspective
DEFINE_CSS_PROP(perspective, "perspective");
///! Defines at which position the user is looking at the 3D-positioned element
DEFINE_CSS_PROP(perspective_origin, "perspective-origin");
///! Specifies align-content and justify-content property values for flexbox and
/// grid layouts
DEFINE_CSS_PROP(place_content, "place-content");
///! Specifies align-items and justify-items property values for grid layouts
DEFINE_CSS_PROP(place_items, "place-items");
///! Specifies align-self and justify-self property values for grid layouts
DEFINE_CSS_PROP(place_self, "place-self");
///! Defines whether or not an element reacts to pointer events
DEFINE_CSS_PROP(pointer_events, "pointer-events");
///! Specifies the type of positioning method used for an element (static,
/// relative, absolute or fixed)
DEFINE_CSS_PROP(position, "position");
///! Sets the type of quotation marks for embedded quotations
DEFINE_CSS_PROP(quotes, "quotes");
///! Defines if (and how) an element is resizable by the user
DEFINE_CSS_PROP(resize, "resize");
///! Specifies the right position of a positioned element
DEFINE_CSS_PROP(right, "right");
///! Specifies the rotation of an element
DEFINE_CSS_PROP(rotate, "rotate");
///! Specifies the gap between the grid rows
DEFINE_CSS_PROP(row_gap, "row-gap");
///! Specifies the size of an element by scaling up or down
DEFINE_CSS_PROP(scale, "scale");
///! Specifies whether to smoothly animate the scroll position in a scrollable
/// box, instead of a straight jump
DEFINE_CSS_PROP(scroll_behavior, "scroll-behavior");
///! Specifies the margin between the snap position and the container
DEFINE_CSS_PROP(scroll_margin, "scroll-margin");
///! Specifies the margin between the snap position and the container in the
/// block direction
DEFINE_CSS_PROP(scroll_margin_block, "scroll-margin-block");
///! Specifies the end margin between the snap position and the container in the
/// block direction
DEFINE_CSS_PROP(scroll_margin_block_end, "scroll-margin-block-end");
///! Specifies the start margin between the snap position and the container in
/// the block direction
DEFINE_CSS_PROP(scroll_margin_block_start, "scroll-margin-block-start");
///! Specifies the margin between the snap position on the bottom side and the
/// container
DEFINE_CSS_PROP(scroll_margin_bottom, "scroll-margin-bottom");
///! Specifies the margin between the snap position and the container in the
/// inline direction
DEFINE_CSS_PROP(scroll_margin_inline, "scroll-margin-inline");
///! Specifies the end margin between the snap position and the container in the
/// inline direction
DEFINE_CSS_PROP(scroll_margin_inline_end, "scroll-margin-inline-end");
///! Specifies the start margin between the snap position and the container in
/// the inline direction
DEFINE_CSS_PROP(scroll_margin_inline_start, "scroll-margin-inline-start");
///! Specifies the margin between the snap position on the left side and the
/// container
DEFINE_CSS_PROP(scroll_margin_left, "scroll-margin-left");
///! Specifies the margin between the snap position on the right side and the
/// container
DEFINE_CSS_PROP(scroll_margin_right, "scroll-margin-right");
///! Specifies the margin between the snap position on the top side and the
/// container
DEFINE_CSS_PROP(scroll_margin_top, "scroll-margin-top");
///! Specifies the distance from the container to the snap position on the child
/// elements
DEFINE_CSS_PROP(scroll_padding, "scroll-padding");
///! Specifies the distance in block direction from the container to the snap
/// position on the child elements
DEFINE_CSS_PROP(scroll_padding_block, "scroll-padding-block");
///! Specifies the distance in block direction from the end of the container to
/// the snap position on the child elements
DEFINE_CSS_PROP(scroll_padding_block_end, "scroll-padding-block-end");
///! Specifies the distance in block direction from the start of the container
/// to the snap position on the child elements
DEFINE_CSS_PROP(scroll_padding_block_start, "scroll-padding-block-start");
///! Specifies the distance from the bottom of the container to the snap
/// position on the child elements
DEFINE_CSS_PROP(scroll_padding_bottom, "scroll-padding-bottom");
///! Specifies the distance in inline direction from the container to the snap
/// position on the child elements
DEFINE_CSS_PROP(scroll_padding_inline, "scroll-padding-inline");
///! Specifies the distance in inline direction from the end of the container to
/// the snap position on the child elements
DEFINE_CSS_PROP(scroll_padding_inline_end, "scroll-padding-inline-end");
///! Specifies the distance in inline direction from the start of the container
/// to the snap position on the child elements
DEFINE_CSS_PROP(scroll_padding_inline_start, "scroll-padding-inline-start");
///! Specifies the distance from the left side of the container to the snap
/// position on the child elements
DEFINE_CSS_PROP(scroll_padding_left, "scroll-padding-left");
///! Specifies the distance from the right side of the container to the snap
/// position on the child elements
DEFINE_CSS_PROP(scroll_padding_right, "scroll-padding-right");
///! Specifies the distance from the top of the container to the snap position
/// on the child elements
DEFINE_CSS_PROP(scroll_padding_top, "scroll-padding-top");
///! Specifies where to position elements when the user stops scrolling
DEFINE_CSS_PROP(scroll_snap_align, "scroll-snap-align");
///! Specifies scroll behaviour after fast swipe on trackpad or touch screen
DEFINE_CSS_PROP(scroll_snap_stop, "scroll-snap-stop");
///! Specifies how snap behaviour should be when scrolling
DEFINE_CSS_PROP(scroll_snap_type, "scroll-snap-type");
///! Specifies the color of the scrollbar of an element
DEFINE_CSS_PROP(scrollbar_color, "scrollbar-color");
///! Specifies the width of a tab character
DEFINE_CSS_PROP(tab_size, "tab-size");
///! Defines the algorithm used to lay out table cells, rows, and columns
DEFINE_CSS_PROP(table_layout, "table-layout");
///! Specifies the horizontal alignment of text
DEFINE_CSS_PROP(text_align, "text-align");
///! Describes how the last line of a block or a line right before a forced line
/// break is aligned when text-align is "justify"
DEFINE_CSS_PROP(text_align_last, "text-align-last");
///! Specifies the combination of multiple characters into the space of a single
/// character
DEFINE_CSS_PROP(text_combine_upright, "text-combine-upright");
///! Specifies the decoration added to text
DEFINE_CSS_PROP(text_decoration, "text-decoration");
///! Specifies the color of the text-decoration
DEFINE_CSS_PROP(text_decoration_color, "text-decoration-color");
///! Specifies the type of line in a text-decoration
DEFINE_CSS_PROP(text_decoration_line, "text-decoration-line");
///! Specifies the style of the line in a text decoration
DEFINE_CSS_PROP(text_decoration_style, "text-decoration-style");
///! Specifies the thickness of the decoration line
DEFINE_CSS_PROP(text_decoration_thickness, "text-decoration-thickness");
///! A shorthand property for the text-emphasis-style and text-emphasis-color
/// properties
DEFINE_CSS_PROP(text_emphasis, "text-emphasis");
///! Specifies the color of emphasis marks
DEFINE_CSS_PROP(text_emphasis_color, "text-emphasis-color");
///! Specifies the position of emphasis marks
DEFINE_CSS_PROP(text_emphasis_position, "text-emphasis-position");
///! Specifies the style of emphasis marks
DEFINE_CSS_PROP(text_emphasis_style, "text-emphasis-style");
///! Specifies the indentation of the first line in a text-block
DEFINE_CSS_PROP(text_indent, "text-indent");
///! Specifies the justification method used when text-align is "justify"
DEFINE_CSS_PROP(text_justify, "text-justify");
///! Defines the orientation of characters in a line
DEFINE_CSS_PROP(text_orientation, "text-orientation");
///! Specifies what should happen when text overflows the containing element
DEFINE_CSS_PROP(text_overflow, "text-overflow");
///! Adds shadow to text
DEFINE_CSS_PROP(text_shadow, "text-shadow");
///! Controls the capitalization of text
DEFINE_CSS_PROP(text_transform, "text-transform");
///! Specifies the offset distance of the underline text decoration
DEFINE_CSS_PROP(text_underline_offset, "text-underline-offset");
///! Specifies the position of the underline text decoration
DEFINE_CSS_PROP(text_underline_position, "text-underline-position");
///! Specifies the top position of a positioned element
DEFINE_CSS_PROP(top, "top");
///! Applies a 2D or 3D transformation to an element
DEFINE_CSS_PROP(transform, "transform");
///! Allows you to change the position on transformed elements
DEFINE_CSS_PROP(transform_origin, "transform-origin");
///! Specifies how nested elements are rendered in 3D space
DEFINE_CSS_PROP(transform_style, "transform-style");
///! A shorthand property for all the transition-* properties
DEFINE_CSS_PROP(transition, "transition");
///! Specifies when the transition effect will start
DEFINE_CSS_PROP(transition_delay, "transition-delay");
///! Specifies how many seconds or milliseconds a transition effect takes to
/// complete
DEFINE_CSS_PROP(transition_duration, "transition-duration");
///! Specifies the name of the CSS property the transition effect is for
DEFINE_CSS_PROP(transition_property, "transition-property");
///! Specifies the speed curve of the transition effect
DEFINE_CSS_PROP(transition_timing_function, "transition-timing-function");
///! Specifies the position of an element
DEFINE_CSS_PROP(translate, "translate");
///! Used together with the direction property to set or return whether the text
/// should be overridden to support multiple languages in the same document
DEFINE_CSS_PROP(unicode_bidi, "unicode-bidi");
///! Specifies whether the text of an element can be selected
DEFINE_CSS_PROP(user_select, "user-select");
///! Sets the vertical alignment of an element
DEFINE_CSS_PROP(vertical_align, "vertical-align");
///! Specifies whether or not an element is visible
DEFINE_CSS_PROP(visibility, "visibility");
///! Specifies how white-space inside an element is handled
DEFINE_CSS_PROP(white_space, "white-space");
///! Sets the minimum number of lines that must be left at the top of a page or
/// column
DEFINE_CSS_PROP(widows, "widows");
///! Sets the width of an element
DEFINE_CSS_PROP(width, "width");
///! Specifies how words should break when reaching the end of a line
DEFINE_CSS_PROP(word_break, "word-break");
///! Increases or decreases the space between words in a text
DEFINE_CSS_PROP(word_spacing, "word-spacing");
///! Allows long, unbreakable words to be broken and wrap to the next line
DEFINE_CSS_PROP(word_wrap, "word-wrap");
///! Specifies whether lines of text are laid out horizontally or vertically
DEFINE_CSS_PROP(writing_mode, "writing-mode");
///! Sets the stack order of a positioned element
DEFINE_CSS_PROP(z_index, "z-index");

///! quoted string
inline const std::string qs(std::string uqs){
    return "\"" + uqs + "\"";
}

class css_error : std::runtime_error {
    public:
    css_error(std::string desc, std::string hint): std::runtime_error("css-error: " + desc + ". hint: " + hint){}
};

class builder {
    std::optional<std::reference_wrapper<std::string>> context_data;
    std::map<std::string, std::string> context;
    bool use_cache = true;
    std::string cache;
public:
builder(){}
builder(const std::string & css): cache(std::move(css)){}
  inline builder &operator()(const std::string & selector) {
      // if(context.contains(selector)){
      //     throw css_error("Selector \"" + selector + "\" was already specified", "try specifying another selector.");
      // }
      context_data = std::ref(context[selector]);
      return *this;
  }

  inline builder &operator<<(const std::pair<std::string, std::string> & prop) {
      use_cache = false;
      if(not context_data){
        throw css_error("No selector specified, use css(<selector>) before << <css-prop>.", "try specifying some selector, eg: css(\"*\")");
      }
      context_data.value().get() += prop.first + ":" + prop.second + ";";
    return *this;
  }

  inline void inject(std::string & dest){
      auto pos = dest.find("</head>");
      if(pos !=  std::string::npos)
          dest.insert(pos, "<style>\n" + build() + "\n</style>\n");
      else
          dest += "<style>\n" + build() + "\n</style>\n";
  }

  inline std::string build() {
      if(not use_cache) {
        cache.clear();
        for(const auto & [selector, props] : context){
            cache += selector + "{" + props + "}";
        }
      }
      return cache;
  }
};
} // namespace css

namespace js {



    class builder {
        std::string context;
        public:
        builder(){}
        builder(const std::string & js): context(std::move(js)){}

        inline builder & operator <<(const std::string & script){
            context += script;
            return *this;
        }

        inline void inject(std::string & dest){
            auto pos = dest.rfind("</body>");
            if(pos !=  std::string::npos)
                dest.insert(pos, "<script>\n" + context + "\n</script>\n");
            else
                dest += "<script>\n" + context + "\n</script>\n";
        }
    };
}
namespace hx {
    constexpr auto htmx_2_0_1_str = R"htmx(
            var htmx=function(){"use strict";const Q={onLoad:null,process:null,on:null,off:null,trigger:null,ajax:null,find:null,findAll:null,closest:null,values:function(e,t){const n=cn(e,t||"post");return n.values},remove:null,addClass:null,removeClass:null,toggleClass:null,takeClass:null,swap:null,defineExtension:null,removeExtension:null,logAll:null,logNone:null,logger:null,config:{historyEnabled:true,historyCacheSize:10,refreshOnHistoryMiss:false,defaultSwapStyle:"innerHTML",defaultSwapDelay:0,defaultSettleDelay:20,includeIndicatorStyles:true,indicatorClass:"htmx-indicator",requestClass:"htmx-request",addedClass:"htmx-added",settlingClass:"htmx-settling",swappingClass:"htmx-swapping",allowEval:true,allowScriptTags:true,inlineScriptNonce:"",inlineStyleNonce:"",attributesToSettle:["class","style","width","height"],withCredentials:false,timeout:0,wsReconnectDelay:"full-jitter",wsBinaryType:"blob",disableSelector:"[hx-disable], [data-hx-disable]",scrollBehavior:"instant",defaultFocusScroll:false,getCacheBusterParam:false,globalViewTransitions:false,methodsThatUseUrlParams:["get","delete"],selfRequestsOnly:true,ignoreTitle:false,scrollIntoViewOnBoost:true,triggerSpecsCache:null,disableInheritance:false,responseHandling:[{code:"204",swap:false},{code:"[23]..",swap:true},{code:"[45]..",swap:false,error:true}],allowNestedOobSwaps:true},parseInterval:null,_:null,version:"2.0.1"};Q.onLoad=$;Q.process=kt;Q.on=be;Q.off=we;Q.trigger=he;Q.ajax=Hn;Q.find=r;Q.findAll=p;Q.closest=g;Q.remove=K;Q.addClass=Y;Q.removeClass=o;Q.toggleClass=W;Q.takeClass=ge;Q.swap=ze;Q.defineExtension=Un;Q.removeExtension=Bn;Q.logAll=z;Q.logNone=J;Q.parseInterval=d;Q._=_;const n={addTriggerHandler:Et,bodyContains:le,canAccessLocalStorage:j,findThisElement:Ee,filterValues:dn,swap:ze,hasAttribute:s,getAttributeValue:te,getClosestAttributeValue:re,getClosestMatch:T,getExpressionVars:Cn,getHeaders:hn,getInputValues:cn,getInternalData:ie,getSwapSpecification:pn,getTriggerSpecs:lt,getTarget:Ce,makeFragment:k,mergeObjects:ue,makeSettleInfo:xn,oobSwap:Te,querySelectorExt:fe,settleImmediately:Gt,shouldCancel:dt,triggerEvent:he,triggerErrorEvent:ae,withExtensions:Ut};const v=["get","post","put","delete","patch"];const R=v.map(function(e){return"[hx-"+e+"], [data-hx-"+e+"]"}).join(", ");const O=e("head");function e(e,t=false){return new RegExp(`<${e}(\\s[^>]*>|>)([\\s\\S]*?)<\\/${e}>`,t?"gim":"im")}function d(e){if(e==undefined){return undefined}let t=NaN;if(e.slice(-2)=="ms"){t=parseFloat(e.slice(0,-2))}else if(e.slice(-1)=="s"){t=parseFloat(e.slice(0,-1))*1e3}else if(e.slice(-1)=="m"){t=parseFloat(e.slice(0,-1))*1e3*60}else{t=parseFloat(e)}return isNaN(t)?undefined:t}function ee(e,t){return e instanceof Element&&e.getAttribute(t)}function s(e,t){return!!e.hasAttribute&&(e.hasAttribute(t)||e.hasAttribute("data-"+t))}function te(e,t){return ee(e,t)||ee(e,"data-"+t)}function u(e){const t=e.parentElement;if(!t&&e.parentNode instanceof ShadowRoot)return e.parentNode;return t}function ne(){return document}function H(e,t){return e.getRootNode?e.getRootNode({composed:t}):ne()}function T(e,t){while(e&&!t(e)){e=u(e)}return e||null}function q(e,t,n){const r=te(t,n);const o=te(t,"hx-disinherit");var i=te(t,"hx-inherit");if(e!==t){if(Q.config.disableInheritance){if(i&&(i==="*"||i.split(" ").indexOf(n)>=0)){return r}else{return null}}if(o&&(o==="*"||o.split(" ").indexOf(n)>=0)){return"unset"}}return r}function re(t,n){let r=null;T(t,function(e){return!!(r=q(t,ce(e),n))});if(r!=="unset"){return r}}function a(e,t){const n=e instanceof Element&&(e.matches||e.matchesSelector||e.msMatchesSelector||e.mozMatchesSelector||e.webkitMatchesSelector||e.oMatchesSelector);return!!n&&n.call(e,t)}function L(e){const t=/<([a-z][^\/\0>\x20\t\r\n\f]*)/i;const n=t.exec(e);if(n){return n[1].toLowerCase()}else{return""}}function N(e){const t=new DOMParser;return t.parseFromString(e,"text/html")}function A(e,t){while(t.childNodes.length>0){e.append(t.childNodes[0])}}function I(e){const t=ne().createElement("script");se(e.attributes,function(e){t.setAttribute(e.name,e.value)});t.textContent=e.textContent;t.async=false;if(Q.config.inlineScriptNonce){t.nonce=Q.config.inlineScriptNonce}return t}function P(e){return e.matches("script")&&(e.type==="text/javascript"||e.type==="module"||e.type==="")}function D(e){Array.from(e.querySelectorAll("script")).forEach(e=>{if(P(e)){const t=I(e);const n=e.parentNode;try{n.insertBefore(t,e)}catch(e){w(e)}finally{e.remove()}}})}function k(e){const t=e.replace(O,"");const n=L(t);let r;if(n==="html"){r=new DocumentFragment;const i=N(e);A(r,i.body);r.title=i.title}else if(n==="body"){r=new DocumentFragment;const i=N(t);A(r,i.body);r.title=i.title}else{const i=N('<body><template class="internal-htmx-wrapper">'+t+"</template></body>");r=i.querySelector("template").content;r.title=i.title;var o=r.querySelector("title");if(o&&o.parentNode===r){o.remove();r.title=o.innerText}}if(r){if(Q.config.allowScriptTags){D(r)}else{r.querySelectorAll("script").forEach(e=>e.remove())}}return r}function oe(e){if(e){e()}}function t(e,t){return Object.prototype.toString.call(e)==="[object "+t+"]"}function M(e){return typeof e==="function"}function X(e){return t(e,"Object")}function ie(e){const t="htmx-internal-data";let n=e[t];if(!n){n=e[t]={}}return n}function F(t){const n=[];if(t){for(let e=0;e<t.length;e++){n.push(t[e])}}return n}function se(t,n){if(t){for(let e=0;e<t.length;e++){n(t[e])}}}function U(e){const t=e.getBoundingClientRect();const n=t.top;const r=t.bottom;return n<window.innerHeight&&r>=0}function le(e){const t=e.getRootNode&&e.getRootNode();if(t&&t instanceof window.ShadowRoot){return ne().body.contains(t.host)}else{return ne().body.contains(e)}}function B(e){return e.trim().split(/\s+/)}function ue(e,t){for(const n in t){if(t.hasOwnProperty(n)){e[n]=t[n]}}return e}function S(e){try{return JSON.parse(e)}catch(e){w(e);return null}}function j(){const e="htmx:localStorageTest";try{localStorage.setItem(e,e);localStorage.removeItem(e);return true}catch(e){return false}}function V(t){try{const e=new URL(t);if(e){t=e.pathname+e.search}if(!/^\/$/.test(t)){t=t.replace(/\/+$/,"")}return t}catch(e){return t}}function _(e){return vn(ne().body,function(){return eval(e)})}function $(t){const e=Q.on("htmx:load",function(e){t(e.detail.elt)});return e}function z(){Q.logger=function(e,t,n){if(console){console.log(t,e,n)}}}function J(){Q.logger=null}function r(e,t){if(typeof e!=="string"){return e.querySelector(t)}else{return r(ne(),e)}}function p(e,t){if(typeof e!=="string"){return e.querySelectorAll(t)}else{return p(ne(),e)}}function E(){return window}function K(e,t){e=y(e);if(t){E().setTimeout(function(){K(e);e=null},t)}else{u(e).removeChild(e)}}function ce(e){return e instanceof Element?e:null}function G(e){return e instanceof HTMLElement?e:null}function Z(e){return typeof e==="string"?e:null}function h(e){return e instanceof Element||e instanceof Document||e instanceof DocumentFragment?e:null}function Y(e,t,n){e=ce(y(e));if(!e){return}if(n){E().setTimeout(function(){Y(e,t);e=null},n)}else{e.classList&&e.classList.add(t)}}function o(e,t,n){let r=ce(y(e));if(!r){return}if(n){E().setTimeout(function(){o(r,t);r=null},n)}else{if(r.classList){r.classList.remove(t);if(r.classList.length===0){r.removeAttribute("class")}}}}function W(e,t){e=y(e);e.classList.toggle(t)}function ge(e,t){e=y(e);se(e.parentElement.children,function(e){o(e,t)});Y(ce(e),t)}function g(e,t){e=ce(y(e));if(e&&e.closest){return e.closest(t)}else{do{if(e==null||a(e,t)){return e}}while(e=e&&ce(u(e)));return null}}function l(e,t){return e.substring(0,t.length)===t}function pe(e,t){return e.substring(e.length-t.length)===t}function i(e){const t=e.trim();if(l(t,"<")&&pe(t,"/>")){return t.substring(1,t.length-2)}else{return t}}function m(e,t,n){e=y(e);if(t.indexOf("closest ")===0){return[g(ce(e),i(t.substr(8)))]}else if(t.indexOf("find ")===0){return[r(h(e),i(t.substr(5)))]}else if(t==="next"){return[ce(e).nextElementSibling]}else if(t.indexOf("next ")===0){return[me(e,i(t.substr(5)),!!n)]}else if(t==="previous"){return[ce(e).previousElementSibling]}else if(t.indexOf("previous ")===0){return[ye(e,i(t.substr(9)),!!n)]}else if(t==="document"){return[document]}else if(t==="window"){return[window]}else if(t==="body"){return[document.body]}else if(t==="root"){return[H(e,!!n)]}else if(t.indexOf("global ")===0){return m(e,t.slice(7),true)}else{return F(h(H(e,!!n)).querySelectorAll(i(t)))}}var me=function(t,e,n){const r=h(H(t,n)).querySelectorAll(e);for(let e=0;e<r.length;e++){const o=r[e];if(o.compareDocumentPosition(t)===Node.DOCUMENT_POSITION_PRECEDING){return o}}};var ye=function(t,e,n){const r=h(H(t,n)).querySelectorAll(e);for(let e=r.length-1;e>=0;e--){const o=r[e];if(o.compareDocumentPosition(t)===Node.DOCUMENT_POSITION_FOLLOWING){return o}}};function fe(e,t){if(typeof e!=="string"){return m(e,t)[0]}else{return m(ne().body,e)[0]}}function y(e,t){if(typeof e==="string"){return r(h(t)||document,e)}else{return e}}function xe(e,t,n){if(M(t)){return{target:ne().body,event:Z(e),listener:t}}else{return{target:y(e),event:Z(t),listener:n}}}function be(t,n,r){_n(function(){const e=xe(t,n,r);e.target.addEventListener(e.event,e.listener)});const e=M(n);return e?n:r}function we(t,n,r){_n(function(){const e=xe(t,n,r);e.target.removeEventListener(e.event,e.listener)});return M(n)?n:r}const ve=ne().createElement("output");function Se(e,t){const n=re(e,t);if(n){if(n==="this"){return[Ee(e,t)]}else{const r=m(e,n);if(r.length===0){w('The selector "'+n+'" on '+t+" returned no matches!");return[ve]}else{return r}}}}function Ee(e,t){return ce(T(e,function(e){return te(ce(e),t)!=null}))}function Ce(e){const t=re(e,"hx-target");if(t){if(t==="this"){return Ee(e,"hx-target")}else{return fe(e,t)}}else{const n=ie(e);if(n.boosted){return ne().body}else{return e}}}function Re(t){const n=Q.config.attributesToSettle;for(let e=0;e<n.length;e++){if(t===n[e]){return true}}return false}function Oe(t,n){se(t.attributes,function(e){if(!n.hasAttribute(e.name)&&Re(e.name)){t.removeAttribute(e.name)}});se(n.attributes,function(e){if(Re(e.name)){t.setAttribute(e.name,e.value)}})}function He(t,e){const n=jn(e);for(let e=0;e<n.length;e++){const r=n[e];try{if(r.isInlineSwap(t)){return true}}catch(e){w(e)}}return t==="outerHTML"}function Te(e,o,i){let t="#"+ee(o,"id");let s="outerHTML";if(e==="true"){}else if(e.indexOf(":")>0){s=e.substr(0,e.indexOf(":"));t=e.substr(e.indexOf(":")+1,e.length)}else{s=e}const n=ne().querySelectorAll(t);if(n){se(n,function(e){let t;const n=o.cloneNode(true);t=ne().createDocumentFragment();t.appendChild(n);if(!He(s,e)){t=h(n)}const r={shouldSwap:true,target:e,fragment:t};if(!he(e,"htmx:oobBeforeSwap",r))return;e=r.target;if(r.shouldSwap){_e(s,e,e,t,i)}se(i.elts,function(e){he(e,"htmx:oobAfterSwap",r)})});o.parentNode.removeChild(o)}else{o.parentNode.removeChild(o);ae(ne().body,"htmx:oobErrorNoTarget",{content:o})}return e}function qe(e){se(p(e,"[hx-preserve], [data-hx-preserve]"),function(e){const t=te(e,"id");const n=ne().getElementById(t);if(n!=null){e.parentNode.replaceChild(n,e)}})}function Le(l,e,u){se(e.querySelectorAll("[id]"),function(t){const n=ee(t,"id");if(n&&n.length>0){const r=n.replace("'","\\'");const o=t.tagName.replace(":","\\:");const e=h(l);const i=e&&e.querySelector(o+"[id='"+r+"']");if(i&&i!==e){const s=t.cloneNode();Oe(t,i);u.tasks.push(function(){Oe(t,s)})}}})}function Ne(e){return function(){o(e,Q.config.addedClass);kt(ce(e));Ae(h(e));he(e,"htmx:load")}}function Ae(e){const t="[autofocus]";const n=G(a(e,t)?e:e.querySelector(t));if(n!=null){n.focus()}}function c(e,t,n,r){Le(e,n,r);while(n.childNodes.length>0){const o=n.firstChild;Y(ce(o),Q.config.addedClass);e.insertBefore(o,t);if(o.nodeType!==Node.TEXT_NODE&&o.nodeType!==Node.COMMENT_NODE){r.tasks.push(Ne(o))}}}function Ie(e,t){let n=0;while(n<e.length){t=(t<<5)-t+e.charCodeAt(n++)|0}return t}function Pe(t){let n=0;if(t.attributes){for(let e=0;e<t.attributes.length;e++){const r=t.attributes[e];if(r.value){n=Ie(r.name,n);n=Ie(r.value,n)}}}return n}function De(t){const n=ie(t);if(n.onHandlers){for(let e=0;e<n.onHandlers.length;e++){const r=n.onHandlers[e];we(t,r.event,r.listener)}delete n.onHandlers}}function ke(e){const t=ie(e);if(t.timeout){clearTimeout(t.timeout)}if(t.listenerInfos){se(t.listenerInfos,function(e){if(e.on){we(e.on,e.trigger,e.listener)}})}De(e);se(Object.keys(t),function(e){delete t[e]})}function f(e){he(e,"htmx:beforeCleanupElement");ke(e);if(e.children){se(e.children,function(e){f(e)})}}function Me(t,e,n){if(t instanceof Element&&t.tagName==="BODY"){return Ve(t,e,n)}let r;const o=t.previousSibling;c(u(t),t,e,n);if(o==null){r=u(t).firstChild}else{r=o.nextSibling}n.elts=n.elts.filter(function(e){return e!==t});while(r&&r!==t){if(r instanceof Element){n.elts.push(r);r=r.nextElementSibling}else{r=null}}f(t);if(t instanceof Element){t.remove()}else{t.parentNode.removeChild(t)}}function Xe(e,t,n){return c(e,e.firstChild,t,n)}function Fe(e,t,n){return c(u(e),e,t,n)}function Ue(e,t,n){return c(e,null,t,n)}function Be(e,t,n){return c(u(e),e.nextSibling,t,n)}function je(e){f(e);return u(e).removeChild(e)}function Ve(e,t,n){const r=e.firstChild;c(e,r,t,n);if(r){while(r.nextSibling){f(r.nextSibling);e.removeChild(r.nextSibling)}f(r);e.removeChild(r)}}function _e(t,e,n,r,o){switch(t){case"none":return;case"outerHTML":Me(n,r,o);return;case"afterbegin":Xe(n,r,o);return;case"beforebegin":Fe(n,r,o);return;case"beforeend":Ue(n,r,o);return;case"afterend":Be(n,r,o);return;case"delete":je(n);return;default:var i=jn(e);for(let e=0;e<i.length;e++){const s=i[e];try{const l=s.handleSwap(t,n,r,o);if(l){if(typeof l.length!=="undefined"){for(let e=0;e<l.length;e++){const u=l[e];if(u.nodeType!==Node.TEXT_NODE&&u.nodeType!==Node.COMMENT_NODE){o.tasks.push(Ne(u))}}}return}}catch(e){w(e)}}if(t==="innerHTML"){Ve(n,r,o)}else{_e(Q.config.defaultSwapStyle,e,n,r,o)}}}function $e(e,n){se(p(e,"[hx-swap-oob], [data-hx-swap-oob]"),function(e){if(Q.config.allowNestedOobSwaps||e.parentElement===null){const t=te(e,"hx-swap-oob");if(t!=null){Te(t,e,n)}}else{e.removeAttribute("hx-swap-oob");e.removeAttribute("data-hx-swap-oob")}})}function ze(e,t,r,o){if(!o){o={}}e=y(e);const n=document.activeElement;let i={};try{i={elt:n,start:n?n.selectionStart:null,end:n?n.selectionEnd:null}}catch(e){}const s=xn(e);if(r.swapStyle==="textContent"){e.textContent=t}else{let n=k(t);s.title=n.title;if(o.selectOOB){const u=o.selectOOB.split(",");for(let t=0;t<u.length;t++){const c=u[t].split(":",2);let e=c[0].trim();if(e.indexOf("#")===0){e=e.substring(1)}const f=c[1]||"true";const a=n.querySelector("#"+e);if(a){Te(f,a,s)}}}$e(n,s);se(p(n,"template"),function(e){$e(e.content,s);if(e.content.childElementCount===0&&e.content.textContent.trim()===""){e.remove()}});if(o.select){const h=ne().createDocumentFragment();se(n.querySelectorAll(o.select),function(e){h.appendChild(e)});n=h}qe(n);_e(r.swapStyle,o.contextElement,e,n,s)}if(i.elt&&!le(i.elt)&&ee(i.elt,"id")){const d=document.getElementById(ee(i.elt,"id"));const g={preventScroll:r.focusScroll!==undefined?!r.focusScroll:!Q.config.defaultFocusScroll};if(d){if(i.start&&d.setSelectionRange){try{d.setSelectionRange(i.start,i.end)}catch(e){}}d.focus(g)}}e.classList.remove(Q.config.swappingClass);se(s.elts,function(e){if(e.classList){e.classList.add(Q.config.settlingClass)}he(e,"htmx:afterSwap",o.eventInfo)});if(o.afterSwapCallback){o.afterSwapCallback()}if(!r.ignoreTitle){kn(s.title)}const l=function(){se(s.tasks,function(e){e.call()});se(s.elts,function(e){if(e.classList){e.classList.remove(Q.config.settlingClass)}he(e,"htmx:afterSettle",o.eventInfo)});if(o.anchor){const e=ce(y("#"+o.anchor));if(e){e.scrollIntoView({block:"start",behavior:"auto"})}}bn(s.elts,r);if(o.afterSettleCallback){o.afterSettleCallback()}};if(r.settleDelay>0){E().setTimeout(l,r.settleDelay)}else{l()}}function Je(e,t,n){const r=e.getResponseHeader(t);if(r.indexOf("{")===0){const o=S(r);for(const i in o){if(o.hasOwnProperty(i)){let e=o[i];if(!X(e)){e={value:e}}he(n,i,e)}}}else{const s=r.split(",");for(let e=0;e<s.length;e++){he(n,s[e].trim(),[])}}}const Ke=/\s/;const x=/[\s,]/;const Ge=/[_$a-zA-Z]/;const Ze=/[_$a-zA-Z0-9]/;const Ye=['"',"'","/"];const We=/[^\s]/;const Qe=/[{(]/;const et=/[})]/;function tt(e){const t=[];let n=0;while(n<e.length){if(Ge.exec(e.charAt(n))){var r=n;while(Ze.exec(e.charAt(n+1))){n++}t.push(e.substr(r,n-r+1))}else if(Ye.indexOf(e.charAt(n))!==-1){const o=e.charAt(n);var r=n;n++;while(n<e.length&&e.charAt(n)!==o){if(e.charAt(n)==="\\"){n++}n++}t.push(e.substr(r,n-r+1))}else{const i=e.charAt(n);t.push(i)}n++}return t}function nt(e,t,n){return Ge.exec(e.charAt(0))&&e!=="true"&&e!=="false"&&e!=="this"&&e!==n&&t!=="."}function rt(r,o,i){if(o[0]==="["){o.shift();let e=1;let t=" return (function("+i+"){ return (";let n=null;while(o.length>0){const s=o[0];if(s==="]"){e--;if(e===0){if(n===null){t=t+"true"}o.shift();t+=")})";try{const l=vn(r,function(){return Function(t)()},function(){return true});l.source=t;return l}catch(e){ae(ne().body,"htmx:syntax:error",{error:e,source:t});return null}}}else if(s==="["){e++}if(nt(s,n,i)){t+="(("+i+"."+s+") ? ("+i+"."+s+") : (window."+s+"))"}else{t=t+s}n=o.shift()}}}function b(e,t){let n="";while(e.length>0&&!t.test(e[0])){n+=e.shift()}return n}function ot(e){let t;if(e.length>0&&Qe.test(e[0])){e.shift();t=b(e,et).trim();e.shift()}else{t=b(e,x)}return t}const it="input, textarea, select";function st(e,t,n){const r=[];const o=tt(t);do{b(o,We);const l=o.length;const u=b(o,/[,\[\s]/);if(u!==""){if(u==="every"){const c={trigger:"every"};b(o,We);c.pollInterval=d(b(o,/[,\[\s]/));b(o,We);var i=rt(e,o,"event");if(i){c.eventFilter=i}r.push(c)}else{const f={trigger:u};var i=rt(e,o,"event");if(i){f.eventFilter=i}while(o.length>0&&o[0]!==","){b(o,We);const a=o.shift();if(a==="changed"){f.changed=true}else if(a==="once"){f.once=true}else if(a==="consume"){f.consume=true}else if(a==="delay"&&o[0]===":"){o.shift();f.delay=d(b(o,x))}else if(a==="from"&&o[0]===":"){o.shift();if(Qe.test(o[0])){var s=ot(o)}else{var s=b(o,x);if(s==="closest"||s==="find"||s==="next"||s==="previous"){o.shift();const h=ot(o);if(h.length>0){s+=" "+h}}}f.from=s}else if(a==="target"&&o[0]===":"){o.shift();f.target=ot(o)}else if(a==="throttle"&&o[0]===":"){o.shift();f.throttle=d(b(o,x))}else if(a==="queue"&&o[0]===":"){o.shift();f.queue=b(o,x)}else if(a==="root"&&o[0]===":"){o.shift();f[a]=ot(o)}else if(a==="threshold"&&o[0]===":"){o.shift();f[a]=b(o,x)}else{ae(e,"htmx:syntax:error",{token:o.shift()})}}r.push(f)}}if(o.length===l){ae(e,"htmx:syntax:error",{token:o.shift()})}b(o,We)}while(o[0]===","&&o.shift());if(n){n[t]=r}return r}function lt(e){const t=te(e,"hx-trigger");let n=[];if(t){const r=Q.config.triggerSpecsCache;n=r&&r[t]||st(e,t,r)}if(n.length>0){return n}else if(a(e,"form")){return[{trigger:"submit"}]}else if(a(e,'input[type="button"], input[type="submit"]')){return[{trigger:"click"}]}else if(a(e,it)){return[{trigger:"change"}]}else{return[{trigger:"click"}]}}function ut(e){ie(e).cancelled=true}function ct(e,t,n){const r=ie(e);r.timeout=E().setTimeout(function(){if(le(e)&&r.cancelled!==true){if(!pt(n,e,Xt("hx:poll:trigger",{triggerSpec:n,target:e}))){t(e)}ct(e,t,n)}},n.pollInterval)}function ft(e){return location.hostname===e.hostname&&ee(e,"href")&&ee(e,"href").indexOf("#")!==0}function at(e){return g(e,Q.config.disableSelector)}function ht(t,n,e){if(t instanceof HTMLAnchorElement&&ft(t)&&(t.target===""||t.target==="_self")||t.tagName==="FORM"){n.boosted=true;let r,o;if(t.tagName==="A"){r="get";o=ee(t,"href")}else{const i=ee(t,"method");r=i?i.toLowerCase():"get";if(r==="get"){}o=ee(t,"action")}e.forEach(function(e){mt(t,function(e,t){const n=ce(e);if(at(n)){f(n);return}de(r,o,n,t)},n,e,true)})}}function dt(e,t){const n=ce(t);if(!n){return false}if(e.type==="submit"||e.type==="click"){if(n.tagName==="FORM"){return true}if(a(n,'input[type="submit"], button')&&g(n,"form")!==null){return true}if(n instanceof HTMLAnchorElement&&n.href&&(n.getAttribute("href")==="#"||n.getAttribute("href").indexOf("#")!==0)){return true}}return false}function gt(e,t){return ie(e).boosted&&e instanceof HTMLAnchorElement&&t.type==="click"&&(t.ctrlKey||t.metaKey)}function pt(e,t,n){const r=e.eventFilter;if(r){try{return r.call(t,n)!==true}catch(e){const o=r.source;ae(ne().body,"htmx:eventFilter:error",{error:e,source:o});return true}}return false}function mt(s,l,e,u,c){const f=ie(s);let t;if(u.from){t=m(s,u.from)}else{t=[s]}if(u.changed){t.forEach(function(e){const t=ie(e);t.lastValue=e.value})}se(t,function(o){const i=function(e){if(!le(s)){o.removeEventListener(u.trigger,i);return}if(gt(s,e)){return}if(c||dt(e,s)){e.preventDefault()}if(pt(u,s,e)){return}const t=ie(e);t.triggerSpec=u;if(t.handledFor==null){t.handledFor=[]}if(t.handledFor.indexOf(s)<0){t.handledFor.push(s);if(u.consume){e.stopPropagation()}if(u.target&&e.target){if(!a(ce(e.target),u.target)){return}}if(u.once){if(f.triggeredOnce){return}else{f.triggeredOnce=true}}if(u.changed){const n=ie(o);const r=o.value;if(n.lastValue===r){return}n.lastValue=r}if(f.delayed){clearTimeout(f.delayed)}if(f.throttle){return}if(u.throttle>0){if(!f.throttle){l(s,e);f.throttle=E().setTimeout(function(){f.throttle=null},u.throttle)}}else if(u.delay>0){f.delayed=E().setTimeout(function(){l(s,e)},u.delay)}else{he(s,"htmx:trigger");l(s,e)}}};if(e.listenerInfos==null){e.listenerInfos=[]}e.listenerInfos.push({trigger:u.trigger,listener:i,on:o});o.addEventListener(u.trigger,i)})}let yt=false;let xt=null;function bt(){if(!xt){xt=function(){yt=true};window.addEventListener("scroll",xt);setInterval(function(){if(yt){yt=false;se(ne().querySelectorAll("[hx-trigger*='revealed'],[data-hx-trigger*='revealed']"),function(e){wt(e)})}},200)}}function wt(e){if(!s(e,"data-hx-revealed")&&U(e)){e.setAttribute("data-hx-revealed","true");const t=ie(e);if(t.initHash){he(e,"revealed")}else{e.addEventListener("htmx:afterProcessNode",function(){he(e,"revealed")},{once:true})}}}function vt(e,t,n,r){const o=function(){if(!n.loaded){n.loaded=true;t(e)}};if(r>0){E().setTimeout(o,r)}else{o()}}function St(t,n,e){let i=false;se(v,function(r){if(s(t,"hx-"+r)){const o=te(t,"hx-"+r);i=true;n.path=o;n.verb=r;e.forEach(function(e){Et(t,e,n,function(e,t){const n=ce(e);if(g(n,Q.config.disableSelector)){f(n);return}de(r,o,n,t)})})}});return i}function Et(r,e,t,n){if(e.trigger==="revealed"){bt();mt(r,n,t,e);wt(ce(r))}else if(e.trigger==="intersect"){const o={};if(e.root){o.root=fe(r,e.root)}if(e.threshold){o.threshold=parseFloat(e.threshold)}const i=new IntersectionObserver(function(t){for(let e=0;e<t.length;e++){const n=t[e];if(n.isIntersecting){he(r,"intersect");break}}},o);i.observe(ce(r));mt(ce(r),n,t,e)}else if(e.trigger==="load"){if(!pt(e,r,Xt("load",{elt:r}))){vt(ce(r),n,t,e.delay)}}else if(e.pollInterval>0){t.polling=true;ct(ce(r),n,e)}else{mt(r,n,t,e)}}function Ct(e){const t=ce(e);if(!t){return false}const n=t.attributes;for(let e=0;e<n.length;e++){const r=n[e].name;if(l(r,"hx-on:")||l(r,"data-hx-on:")||l(r,"hx-on-")||l(r,"data-hx-on-")){return true}}return false}const Rt=(new XPathEvaluator).createExpression('.//*[@*[ starts-with(name(), "hx-on:") or starts-with(name(), "data-hx-on:") or'+' starts-with(name(), "hx-on-") or starts-with(name(), "data-hx-on-") ]]');function Ot(e,t){if(Ct(e)){t.push(ce(e))}const n=Rt.evaluate(e);let r=null;while(r=n.iterateNext())t.push(ce(r))}function Ht(e){const t=[];if(e instanceof DocumentFragment){for(const n of e.childNodes){Ot(n,t)}}else{Ot(e,t)}return t}function Tt(e){if(e.querySelectorAll){const n=", [hx-boost] a, [data-hx-boost] a, a[hx-boost], a[data-hx-boost]";const r=[];for(const i in Xn){const s=Xn[i];if(s.getSelectors){var t=s.getSelectors();if(t){r.push(t)}}}const o=e.querySelectorAll(R+n+", form, [type='submit'],"+" [hx-ext], [data-hx-ext], [hx-trigger], [data-hx-trigger]"+r.flat().map(e=>", "+e).join(""));return o}else{return[]}}function qt(e){const t=g(ce(e.target),"button, input[type='submit']");const n=Nt(e);if(n){n.lastButtonClicked=t}}function Lt(e){const t=Nt(e);if(t){t.lastButtonClicked=null}}function Nt(e){const t=g(ce(e.target),"button, input[type='submit']");if(!t){return}const n=y("#"+ee(t,"form"),t.getRootNode())||g(t,"form");if(!n){return}return ie(n)}function At(e){e.addEventListener("click",qt);e.addEventListener("focusin",qt);e.addEventListener("focusout",Lt)}function It(t,e,n){const r=ie(t);if(!Array.isArray(r.onHandlers)){r.onHandlers=[]}let o;const i=function(e){vn(t,function(){if(at(t)){return}if(!o){o=new Function("event",n)}o.call(t,e)})};t.addEventListener(e,i);r.onHandlers.push({event:e,listener:i})}function Pt(t){De(t);for(let e=0;e<t.attributes.length;e++){const n=t.attributes[e].name;const r=t.attributes[e].value;if(l(n,"hx-on")||l(n,"data-hx-on")){const o=n.indexOf("-on")+3;const i=n.slice(o,o+1);if(i==="-"||i===":"){let e=n.slice(o+1);if(l(e,":")){e="htmx"+e}else if(l(e,"-")){e="htmx:"+e.slice(1)}else if(l(e,"htmx-")){e="htmx:"+e.slice(5)}It(t,e,r)}}}}function Dt(t){if(g(t,Q.config.disableSelector)){f(t);return}const n=ie(t);if(n.initHash!==Pe(t)){ke(t);n.initHash=Pe(t);he(t,"htmx:beforeProcessNode");if(t.value){n.lastValue=t.value}const e=lt(t);const r=St(t,n,e);if(!r){if(re(t,"hx-boost")==="true"){ht(t,n,e)}else if(s(t,"hx-trigger")){e.forEach(function(e){Et(t,e,n,function(){})})}}if(t.tagName==="FORM"||ee(t,"type")==="submit"&&s(t,"form")){At(t)}he(t,"htmx:afterProcessNode")}}function kt(e){e=y(e);if(g(e,Q.config.disableSelector)){f(e);return}Dt(e);se(Tt(e),function(e){Dt(e)});se(Ht(e),Pt)}function Mt(e){return e.replace(/([a-z0-9])([A-Z])/g,"$1-$2").toLowerCase()}function Xt(e,t){let n;if(window.CustomEvent&&typeof window.CustomEvent==="function"){n=new CustomEvent(e,{bubbles:true,cancelable:true,composed:true,detail:t})}else{n=ne().createEvent("CustomEvent");n.initCustomEvent(e,true,true,t)}return n}function ae(e,t,n){he(e,t,ue({error:t},n))}function Ft(e){return e==="htmx:afterProcessNode"}function Ut(e,t){se(jn(e),function(e){try{t(e)}catch(e){w(e)}})}function w(e){if(console.error){console.error(e)}else if(console.log){console.log("ERROR: ",e)}}function he(e,t,n){e=y(e);if(n==null){n={}}n.elt=e;const r=Xt(t,n);if(Q.logger&&!Ft(t)){Q.logger(e,t,n)}if(n.error){w(n.error);he(e,"htmx:error",{errorInfo:n})}let o=e.dispatchEvent(r);const i=Mt(t);if(o&&i!==t){const s=Xt(i,r.detail);o=o&&e.dispatchEvent(s)}Ut(ce(e),function(e){o=o&&(e.onEvent(t,r)!==false&&!r.defaultPrevented)});return o}let Bt=location.pathname+location.search;function jt(){const e=ne().querySelector("[hx-history-elt],[data-hx-history-elt]");return e||ne().body}function Vt(t,e){if(!j()){return}const n=$t(e);const r=ne().title;const o=window.scrollY;if(Q.config.historyCacheSize<=0){localStorage.removeItem("htmx-history-cache");return}t=V(t);const i=S(localStorage.getItem("htmx-history-cache"))||[];for(let e=0;e<i.length;e++){if(i[e].url===t){i.splice(e,1);break}}const s={url:t,content:n,title:r,scroll:o};he(ne().body,"htmx:historyItemCreated",{item:s,cache:i});i.push(s);while(i.length>Q.config.historyCacheSize){i.shift()}while(i.length>0){try{localStorage.setItem("htmx-history-cache",JSON.stringify(i));break}catch(e){ae(ne().body,"htmx:historyCacheError",{cause:e,cache:i});i.shift()}}}function _t(t){if(!j()){return null}t=V(t);const n=S(localStorage.getItem("htmx-history-cache"))||[];for(let e=0;e<n.length;e++){if(n[e].url===t){return n[e]}}return null}function $t(e){const t=Q.config.requestClass;const n=e.cloneNode(true);se(p(n,"."+t),function(e){o(e,t)});return n.innerHTML}function zt(){const e=jt();const t=Bt||location.pathname+location.search;let n;try{n=ne().querySelector('[hx-history="false" i],[data-hx-history="false" i]')}catch(e){n=ne().querySelector('[hx-history="false"],[data-hx-history="false"]')}if(!n){he(ne().body,"htmx:beforeHistorySave",{path:t,historyElt:e});Vt(t,e)}if(Q.config.historyEnabled)history.replaceState({htmx:true},ne().title,window.location.href)}function Jt(e){if(Q.config.getCacheBusterParam){e=e.replace(/org\.htmx\.cache-buster=[^&]*&?/,"");if(pe(e,"&")||pe(e,"?")){e=e.slice(0,-1)}}if(Q.config.historyEnabled){history.pushState({htmx:true},"",e)}Bt=e}function Kt(e){if(Q.config.historyEnabled)history.replaceState({htmx:true},"",e);Bt=e}function Gt(e){se(e,function(e){e.call(undefined)})}function Zt(o){const e=new XMLHttpRequest;const i={path:o,xhr:e};he(ne().body,"htmx:historyCacheMiss",i);e.open("GET",o,true);e.setRequestHeader("HX-Request","true");e.setRequestHeader("HX-History-Restore-Request","true");e.setRequestHeader("HX-Current-URL",ne().location.href);e.onload=function(){if(this.status>=200&&this.status<400){he(ne().body,"htmx:historyCacheMissLoad",i);const e=k(this.response);const t=e.querySelector("[hx-history-elt],[data-hx-history-elt]")||e;const n=jt();const r=xn(n);kn(e.title);Ve(n,t,r);Gt(r.tasks);Bt=o;he(ne().body,"htmx:historyRestore",{path:o,cacheMiss:true,serverResponse:this.response})}else{ae(ne().body,"htmx:historyCacheMissLoadError",i)}};e.send()}function Yt(e){zt();e=e||location.pathname+location.search;const t=_t(e);if(t){const n=k(t.content);const r=jt();const o=xn(r);kn(n.title);Ve(r,n,o);Gt(o.tasks);E().setTimeout(function(){window.scrollTo(0,t.scroll)},0);Bt=e;he(ne().body,"htmx:historyRestore",{path:e,item:t})}else{if(Q.config.refreshOnHistoryMiss){window.location.reload(true)}else{Zt(e)}}}function Wt(e){let t=Se(e,"hx-indicator");if(t==null){t=[e]}se(t,function(e){const t=ie(e);t.requestCount=(t.requestCount||0)+1;e.classList.add.call(e.classList,Q.config.requestClass)});return t}function Qt(e){let t=Se(e,"hx-disabled-elt");if(t==null){t=[]}se(t,function(e){const t=ie(e);t.requestCount=(t.requestCount||0)+1;e.setAttribute("disabled","")});return t}function en(e,t){se(e,function(e){const t=ie(e);t.requestCount=(t.requestCount||0)-1;if(t.requestCount===0){e.classList.remove.call(e.classList,Q.config.requestClass)}});se(t,function(e){const t=ie(e);t.requestCount=(t.requestCount||0)-1;if(t.requestCount===0){e.removeAttribute("disabled")}})}function tn(t,n){for(let e=0;e<t.length;e++){const r=t[e];if(r.isSameNode(n)){return true}}return false}function nn(e){const t=e;if(t.name===""||t.name==null||t.disabled||g(t,"fieldset[disabled]")){return false}if(t.type==="button"||t.type==="submit"||t.tagName==="image"||t.tagName==="reset"||t.tagName==="file"){return false}if(t.type==="checkbox"||t.type==="radio"){return t.checked}return true}function rn(t,e,n){if(t!=null&&e!=null){if(Array.isArray(e)){e.forEach(function(e){n.append(t,e)})}else{n.append(t,e)}}}function on(t,n,r){if(t!=null&&n!=null){let e=r.getAll(t);if(Array.isArray(n)){e=e.filter(e=>n.indexOf(e)<0)}else{e=e.filter(e=>e!==n)}r.delete(t);se(e,e=>r.append(t,e))}}function sn(t,n,r,o,i){if(o==null||tn(t,o)){return}else{t.push(o)}if(nn(o)){const s=ee(o,"name");let e=o.value;if(o instanceof HTMLSelectElement&&o.multiple){e=F(o.querySelectorAll("option:checked")).map(function(e){return e.value})}if(o instanceof HTMLInputElement&&o.files){e=F(o.files)}rn(s,e,n);if(i){ln(o,r)}}if(o instanceof HTMLFormElement){se(o.elements,function(e){if(t.indexOf(e)>=0){on(e.name,e.value,n)}else{t.push(e)}if(i){ln(e,r)}});new FormData(o).forEach(function(e,t){if(e instanceof File&&e.name===""){return}rn(t,e,n)})}}function ln(e,t){const n=e;if(n.willValidate){he(n,"htmx:validation:validate");if(!n.checkValidity()){t.push({elt:n,message:n.validationMessage,validity:n.validity});he(n,"htmx:validation:failed",{message:n.validationMessage,validity:n.validity})}}}function un(t,e){for(const n of e.keys()){t.delete(n);e.getAll(n).forEach(function(e){t.append(n,e)})}return t}function cn(e,t){const n=[];const r=new FormData;const o=new FormData;const i=[];const s=ie(e);if(s.lastButtonClicked&&!le(s.lastButtonClicked)){s.lastButtonClicked=null}let l=e instanceof HTMLFormElement&&e.noValidate!==true||te(e,"hx-validate")==="true";if(s.lastButtonClicked){l=l&&s.lastButtonClicked.formNoValidate!==true}if(t!=="get"){sn(n,o,i,g(e,"form"),l)}sn(n,r,i,e,l);if(s.lastButtonClicked||e.tagName==="BUTTON"||e.tagName==="INPUT"&&ee(e,"type")==="submit"){const c=s.lastButtonClicked||e;const f=ee(c,"name");rn(f,c.value,o)}const u=Se(e,"hx-include");se(u,function(e){sn(n,r,i,ce(e),l);if(!a(e,"form")){se(h(e).querySelectorAll(it),function(e){sn(n,r,i,e,l)})}});un(r,o);return{errors:i,formData:r,values:An(r)}}function fn(e,t,n){if(e!==""){e+="&"}if(String(n)==="[object Object]"){n=JSON.stringify(n)}const r=encodeURIComponent(n);e+=encodeURIComponent(t)+"="+r;return e}function an(e){e=Ln(e);let n="";e.forEach(function(e,t){n=fn(n,t,e)});return n}function hn(e,t,n){const r={"HX-Request":"true","HX-Trigger":ee(e,"id"),"HX-Trigger-Name":ee(e,"name"),"HX-Target":te(t,"id"),"HX-Current-URL":ne().location.href};wn(e,"hx-headers",false,r);if(n!==undefined){r["HX-Prompt"]=n}if(ie(e).boosted){r["HX-Boosted"]="true"}return r}function dn(n,e){const t=re(e,"hx-params");if(t){if(t==="none"){return new FormData}else if(t==="*"){return n}else if(t.indexOf("not ")===0){se(t.substr(4).split(","),function(e){e=e.trim();n.delete(e)});return n}else{const r=new FormData;se(t.split(","),function(t){t=t.trim();if(n.has(t)){n.getAll(t).forEach(function(e){r.append(t,e)})}});return r}}else{return n}}function gn(e){return!!ee(e,"href")&&ee(e,"href").indexOf("#")>=0}function pn(e,t){const n=t||re(e,"hx-swap");const r={swapStyle:ie(e).boosted?"innerHTML":Q.config.defaultSwapStyle,swapDelay:Q.config.defaultSwapDelay,settleDelay:Q.config.defaultSettleDelay};if(Q.config.scrollIntoViewOnBoost&&ie(e).boosted&&!gn(e)){r.show="top"}if(n){const s=B(n);if(s.length>0){for(let e=0;e<s.length;e++){const l=s[e];if(l.indexOf("swap:")===0){r.swapDelay=d(l.substr(5))}else if(l.indexOf("settle:")===0){r.settleDelay=d(l.substr(7))}else if(l.indexOf("transition:")===0){r.transition=l.substr(11)==="true"}else if(l.indexOf("ignoreTitle:")===0){r.ignoreTitle=l.substr(12)==="true"}else if(l.indexOf("scroll:")===0){const u=l.substr(7);var o=u.split(":");const c=o.pop();var i=o.length>0?o.join(":"):null;r.scroll=c;r.scrollTarget=i}else if(l.indexOf("show:")===0){const f=l.substr(5);var o=f.split(":");const a=o.pop();var i=o.length>0?o.join(":"):null;r.show=a;r.showTarget=i}else if(l.indexOf("focus-scroll:")===0){const h=l.substr("focus-scroll:".length);r.focusScroll=h=="true"}else if(e==0){r.swapStyle=l}else{w("Unknown modifier in hx-swap: "+l)}}}}return r}function mn(e){return re(e,"hx-encoding")==="multipart/form-data"||a(e,"form")&&ee(e,"enctype")==="multipart/form-data"}function yn(t,n,r){let o=null;Ut(n,function(e){if(o==null){o=e.encodeParameters(t,r,n)}});if(o!=null){return o}else{if(mn(n)){return un(new FormData,Ln(r))}else{return an(r)}}}function xn(e){return{tasks:[],elts:[e]}}function bn(e,t){const n=e[0];const r=e[e.length-1];if(t.scroll){var o=null;if(t.scrollTarget){o=ce(fe(n,t.scrollTarget))}if(t.scroll==="top"&&(n||o)){o=o||n;o.scrollTop=0}if(t.scroll==="bottom"&&(r||o)){o=o||r;o.scrollTop=o.scrollHeight}}if(t.show){var o=null;if(t.showTarget){let e=t.showTarget;if(t.showTarget==="window"){e="body"}o=ce(fe(n,e))}if(t.show==="top"&&(n||o)){o=o||n;o.scrollIntoView({block:"start",behavior:Q.config.scrollBehavior})}if(t.show==="bottom"&&(r||o)){o=o||r;o.scrollIntoView({block:"end",behavior:Q.config.scrollBehavior})}}}function wn(r,e,o,i){if(i==null){i={}}if(r==null){return i}const s=te(r,e);if(s){let e=s.trim();let t=o;if(e==="unset"){return null}if(e.indexOf("javascript:")===0){e=e.substr(11);t=true}else if(e.indexOf("js:")===0){e=e.substr(3);t=true}if(e.indexOf("{")!==0){e="{"+e+"}"}let n;if(t){n=vn(r,function(){return Function("return ("+e+")")()},{})}else{n=S(e)}for(const l in n){if(n.hasOwnProperty(l)){if(i[l]==null){i[l]=n[l]}}}}return wn(ce(u(r)),e,o,i)}function vn(e,t,n){if(Q.config.allowEval){return t()}else{ae(e,"htmx:evalDisallowedError");return n}}function Sn(e,t){return wn(e,"hx-vars",true,t)}function En(e,t){return wn(e,"hx-vals",false,t)}function Cn(e){return ue(Sn(e),En(e))}function Rn(t,n,r){if(r!==null){try{t.setRequestHeader(n,r)}catch(e){t.setRequestHeader(n,encodeURIComponent(r));t.setRequestHeader(n+"-URI-AutoEncoded","true")}}}function On(t){if(t.responseURL&&typeof URL!=="undefined"){try{const e=new URL(t.responseURL);return e.pathname+e.search}catch(e){ae(ne().body,"htmx:badResponseUrl",{url:t.responseURL})}}}function C(e,t){return t.test(e.getAllResponseHeaders())}function Hn(e,t,n){e=e.toLowerCase();if(n){if(n instanceof Element||typeof n==="string"){return de(e,t,null,null,{targetOverride:y(n),returnPromise:true})}else{return de(e,t,y(n.source),n.event,{handler:n.handler,headers:n.headers,values:n.values,targetOverride:y(n.target),swapOverride:n.swap,select:n.select,returnPromise:true})}}else{return de(e,t,null,null,{returnPromise:true})}}function Tn(e){const t=[];while(e){t.push(e);e=e.parentElement}return t}function qn(e,t,n){let r;let o;if(typeof URL==="function"){o=new URL(t,document.location.href);const i=document.location.origin;r=i===o.origin}else{o=t;r=l(t,document.location.origin)}if(Q.config.selfRequestsOnly){if(!r){return false}}return he(e,"htmx:validateUrl",ue({url:o,sameHost:r},n))}function Ln(e){if(e instanceof FormData)return e;const t=new FormData;for(const n in e){if(e.hasOwnProperty(n)){if(typeof e[n].forEach==="function"){e[n].forEach(function(e){t.append(n,e)})}else if(typeof e[n]==="object"){t.append(n,JSON.stringify(e[n]))}else{t.append(n,e[n])}}}return t}function Nn(r,o,e){return new Proxy(e,{get:function(t,e){if(typeof e==="number")return t[e];if(e==="length")return t.length;if(e==="push"){return function(e){t.push(e);r.append(o,e)}}if(typeof t[e]==="function"){return function(){t[e].apply(t,arguments);r.delete(o);t.forEach(function(e){r.append(o,e)})}}if(t[e]&&t[e].length===1){return t[e][0]}else{return t[e]}},set:function(e,t,n){e[t]=n;r.delete(o);e.forEach(function(e){r.append(o,e)});return true}})}function An(r){return new Proxy(r,{get:function(e,t){if(typeof t==="symbol"){return Reflect.get(e,t)}if(t==="toJSON"){return()=>Object.fromEntries(r)}if(t in e){if(typeof e[t]==="function"){return function(){return r[t].apply(r,arguments)}}else{return e[t]}}const n=r.getAll(t);if(n.length===0){return undefined}else if(n.length===1){return n[0]}else{return Nn(e,t,n)}},set:function(t,n,e){if(typeof n!=="string"){return false}t.delete(n);if(typeof e.forEach==="function"){e.forEach(function(e){t.append(n,e)})}else{t.append(n,e)}return true},deleteProperty:function(e,t){if(typeof t==="string"){e.delete(t)}return true},ownKeys:function(e){return Reflect.ownKeys(Object.fromEntries(e))},getOwnPropertyDescriptor:function(e,t){return Reflect.getOwnPropertyDescriptor(Object.fromEntries(e),t)}})}function de(t,n,r,o,i,k){let s=null;let l=null;i=i!=null?i:{};if(i.returnPromise&&typeof Promise!=="undefined"){var e=new Promise(function(e,t){s=e;l=t})}if(r==null){r=ne().body}const M=i.handler||Mn;const X=i.select||null;if(!le(r)){oe(s);return e}const u=i.targetOverride||ce(Ce(r));if(u==null||u==ve){ae(r,"htmx:targetError",{target:te(r,"hx-target")});oe(l);return e}let c=ie(r);const f=c.lastButtonClicked;if(f){const L=ee(f,"formaction");if(L!=null){n=L}const N=ee(f,"formmethod");if(N!=null){if(N.toLowerCase()!=="dialog"){t=N}}}const a=re(r,"hx-confirm");if(k===undefined){const K=function(e){return de(t,n,r,o,i,!!e)};const G={target:u,elt:r,path:n,verb:t,triggeringEvent:o,etc:i,issueRequest:K,question:a};if(he(r,"htmx:confirm",G)===false){oe(s);return e}}let h=r;let d=re(r,"hx-sync");let g=null;let F=false;if(d){const A=d.split(":");const I=A[0].trim();if(I==="this"){h=Ee(r,"hx-sync")}else{h=ce(fe(r,I))}d=(A[1]||"drop").trim();c=ie(h);if(d==="drop"&&c.xhr&&c.abortable!==true){oe(s);return e}else if(d==="abort"){if(c.xhr){oe(s);return e}else{F=true}}else if(d==="replace"){he(h,"htmx:abort")}else if(d.indexOf("queue")===0){const Z=d.split(" ");g=(Z[1]||"last").trim()}}if(c.xhr){if(c.abortable){he(h,"htmx:abort")}else{if(g==null){if(o){const P=ie(o);if(P&&P.triggerSpec&&P.triggerSpec.queue){g=P.triggerSpec.queue}}if(g==null){g="last"}}if(c.queuedRequests==null){c.queuedRequests=[]}if(g==="first"&&c.queuedRequests.length===0){c.queuedRequests.push(function(){de(t,n,r,o,i)})}else if(g==="all"){c.queuedRequests.push(function(){de(t,n,r,o,i)})}else if(g==="last"){c.queuedRequests=[];c.queuedRequests.push(function(){de(t,n,r,o,i)})}oe(s);return e}}const p=new XMLHttpRequest;c.xhr=p;c.abortable=F;const m=function(){c.xhr=null;c.abortable=false;if(c.queuedRequests!=null&&c.queuedRequests.length>0){const e=c.queuedRequests.shift();e()}};const U=re(r,"hx-prompt");if(U){var y=prompt(U);if(y===null||!he(r,"htmx:prompt",{prompt:y,target:u})){oe(s);m();return e}}if(a&&!k){if(!confirm(a)){oe(s);m();return e}}let x=hn(r,u,y);if(t!=="get"&&!mn(r)){x["Content-Type"]="application/x-www-form-urlencoded"}if(i.headers){x=ue(x,i.headers)}const B=cn(r,t);let b=B.errors;const j=B.formData;if(i.values){un(j,Ln(i.values))}const V=Ln(Cn(r));const w=un(j,V);let v=dn(w,r);if(Q.config.getCacheBusterParam&&t==="get"){v.set("org.htmx.cache-buster",ee(u,"id")||"true")}if(n==null||n===""){n=ne().location.href}const S=wn(r,"hx-request");const _=ie(r).boosted;let E=Q.config.methodsThatUseUrlParams.indexOf(t)>=0;const C={boosted:_,useUrlParams:E,formData:v,parameters:An(v),unfilteredFormData:w,unfilteredParameters:An(w),headers:x,target:u,verb:t,errors:b,withCredentials:i.credentials||S.credentials||Q.config.withCredentials,timeout:i.timeout||S.timeout||Q.config.timeout,path:n,triggeringEvent:o};if(!he(r,"htmx:configRequest",C)){oe(s);m();return e}n=C.path;t=C.verb;x=C.headers;v=Ln(C.parameters);b=C.errors;E=C.useUrlParams;if(b&&b.length>0){he(r,"htmx:validation:halted",C);oe(s);m();return e}const $=n.split("#");const z=$[0];const R=$[1];let O=n;if(E){O=z;const Y=!v.keys().next().done;if(Y){if(O.indexOf("?")<0){O+="?"}else{O+="&"}O+=an(v);if(R){O+="#"+R}}}if(!qn(r,O,C)){ae(r,"htmx:invalidPath",C);oe(l);return e}p.open(t.toUpperCase(),O,true);p.overrideMimeType("text/html");p.withCredentials=C.withCredentials;p.timeout=C.timeout;if(S.noHeaders){}else{for(const D in x){if(x.hasOwnProperty(D)){const W=x[D];Rn(p,D,W)}}}const H={xhr:p,target:u,requestConfig:C,etc:i,boosted:_,select:X,pathInfo:{requestPath:n,finalRequestPath:O,responsePath:null,anchor:R}};p.onload=function(){try{const t=Tn(r);H.pathInfo.responsePath=On(p);M(r,H);en(T,q);he(r,"htmx:afterRequest",H);he(r,"htmx:afterOnLoad",H);if(!le(r)){let e=null;while(t.length>0&&e==null){const n=t.shift();if(le(n)){e=n}}if(e){he(e,"htmx:afterRequest",H);he(e,"htmx:afterOnLoad",H)}}oe(s);m()}catch(e){ae(r,"htmx:onLoadError",ue({error:e},H));throw e}};p.onerror=function(){en(T,q);ae(r,"htmx:afterRequest",H);ae(r,"htmx:sendError",H);oe(l);m()};p.onabort=function(){en(T,q);ae(r,"htmx:afterRequest",H);ae(r,"htmx:sendAbort",H);oe(l);m()};p.ontimeout=function(){en(T,q);ae(r,"htmx:afterRequest",H);ae(r,"htmx:timeout",H);oe(l);m()};if(!he(r,"htmx:beforeRequest",H)){oe(s);m();return e}var T=Wt(r);var q=Qt(r);se(["loadstart","loadend","progress","abort"],function(t){se([p,p.upload],function(e){e.addEventListener(t,function(e){he(r,"htmx:xhr:"+t,{lengthComputable:e.lengthComputable,loaded:e.loaded,total:e.total})})})});he(r,"htmx:beforeSend",H);const J=E?null:yn(p,r,v);p.send(J);return e}function In(e,t){const n=t.xhr;let r=null;let o=null;if(C(n,/HX-Push:/i)){r=n.getResponseHeader("HX-Push");o="push"}else if(C(n,/HX-Push-Url:/i)){r=n.getResponseHeader("HX-Push-Url");o="push"}else if(C(n,/HX-Replace-Url:/i)){r=n.getResponseHeader("HX-Replace-Url");o="replace"}if(r){if(r==="false"){return{}}else{return{type:o,path:r}}}const i=t.pathInfo.finalRequestPath;const s=t.pathInfo.responsePath;const l=re(e,"hx-push-url");const u=re(e,"hx-replace-url");const c=ie(e).boosted;let f=null;let a=null;if(l){f="push";a=l}else if(u){f="replace";a=u}else if(c){f="push";a=s||i}if(a){if(a==="false"){return{}}if(a==="true"){a=s||i}if(t.pathInfo.anchor&&a.indexOf("#")===-1){a=a+"#"+t.pathInfo.anchor}return{type:f,path:a}}else{return{}}}function Pn(e,t){var n=new RegExp(e.code);return n.test(t.toString(10))}function Dn(e){for(var t=0;t<Q.config.responseHandling.length;t++){var n=Q.config.responseHandling[t];if(Pn(n,e.status)){return n}}return{swap:false}}function kn(e){if(e){const t=r("title");if(t){t.innerHTML=e}else{window.document.title=e}}}function Mn(o,i){const s=i.xhr;let l=i.target;const e=i.etc;const u=i.select;if(!he(o,"htmx:beforeOnLoad",i))return;if(C(s,/HX-Trigger:/i)){Je(s,"HX-Trigger",o)}if(C(s,/HX-Location:/i)){zt();let e=s.getResponseHeader("HX-Location");var t;if(e.indexOf("{")===0){t=S(e);e=t.path;delete t.path}Hn("get",e,t).then(function(){Jt(e)});return}const n=C(s,/HX-Refresh:/i)&&s.getResponseHeader("HX-Refresh")==="true";if(C(s,/HX-Redirect:/i)){location.href=s.getResponseHeader("HX-Redirect");n&&location.reload();return}if(n){location.reload();return}if(C(s,/HX-Retarget:/i)){if(s.getResponseHeader("HX-Retarget")==="this"){i.target=o}else{i.target=ce(fe(o,s.getResponseHeader("HX-Retarget")))}}const c=In(o,i);const r=Dn(s);const f=r.swap;let a=!!r.error;let h=Q.config.ignoreTitle||r.ignoreTitle;let d=r.select;if(r.target){i.target=ce(fe(o,r.target))}var g=e.swapOverride;if(g==null&&r.swapOverride){g=r.swapOverride}if(C(s,/HX-Retarget:/i)){if(s.getResponseHeader("HX-Retarget")==="this"){i.target=o}else{i.target=ce(fe(o,s.getResponseHeader("HX-Retarget")))}}if(C(s,/HX-Reswap:/i)){g=s.getResponseHeader("HX-Reswap")}var p=s.response;var m=ue({shouldSwap:f,serverResponse:p,isError:a,ignoreTitle:h,selectOverride:d},i);if(r.event&&!he(l,r.event,m))return;if(!he(l,"htmx:beforeSwap",m))return;l=m.target;p=m.serverResponse;a=m.isError;h=m.ignoreTitle;d=m.selectOverride;i.target=l;i.failed=a;i.successful=!a;if(m.shouldSwap){if(s.status===286){ut(o)}Ut(o,function(e){p=e.transformResponse(p,s,o)});if(c.type){zt()}if(C(s,/HX-Reswap:/i)){g=s.getResponseHeader("HX-Reswap")}var y=pn(o,g);if(!y.hasOwnProperty("ignoreTitle")){y.ignoreTitle=h}l.classList.add(Q.config.swappingClass);let n=null;let r=null;if(u){d=u}if(C(s,/HX-Reselect:/i)){d=s.getResponseHeader("HX-Reselect")}const x=re(o,"hx-select-oob");const b=re(o,"hx-select");let e=function(){try{if(c.type){he(ne().body,"htmx:beforeHistoryUpdate",ue({history:c},i));if(c.type==="push"){Jt(c.path);he(ne().body,"htmx:pushedIntoHistory",{path:c.path})}else{Kt(c.path);he(ne().body,"htmx:replacedInHistory",{path:c.path})}}ze(l,p,y,{select:d||b,selectOOB:x,eventInfo:i,anchor:i.pathInfo.anchor,contextElement:o,afterSwapCallback:function(){if(C(s,/HX-Trigger-After-Swap:/i)){let e=o;if(!le(o)){e=ne().body}Je(s,"HX-Trigger-After-Swap",e)}},afterSettleCallback:function(){if(C(s,/HX-Trigger-After-Settle:/i)){let e=o;if(!le(o)){e=ne().body}Je(s,"HX-Trigger-After-Settle",e)}oe(n)}})}catch(e){ae(o,"htmx:swapError",i);oe(r);throw e}};let t=Q.config.globalViewTransitions;if(y.hasOwnProperty("transition")){t=y.transition}if(t&&he(o,"htmx:beforeTransition",i)&&typeof Promise!=="undefined"&&document.startViewTransition){const w=new Promise(function(e,t){n=e;r=t});const v=e;e=function(){document.startViewTransition(function(){v();return w})}}if(y.swapDelay>0){E().setTimeout(e,y.swapDelay)}else{e()}}if(a){ae(o,"htmx:responseError",ue({error:"Response Status Error Code "+s.status+" from "+i.pathInfo.requestPath},i))}}const Xn={};function Fn(){return{init:function(e){return null},getSelectors:function(){return null},onEvent:function(e,t){return true},transformResponse:function(e,t,n){return e},isInlineSwap:function(e){return false},handleSwap:function(e,t,n,r){return false},encodeParameters:function(e,t,n){return null}}}function Un(e,t){if(t.init){t.init(n)}Xn[e]=ue(Fn(),t)}function Bn(e){delete Xn[e]}function jn(e,n,r){if(n==undefined){n=[]}if(e==undefined){return n}if(r==undefined){r=[]}const t=te(e,"hx-ext");if(t){se(t.split(","),function(e){e=e.replace(/ /g,"");if(e.slice(0,7)=="ignore:"){r.push(e.slice(7));return}if(r.indexOf(e)<0){const t=Xn[e];if(t&&n.indexOf(t)<0){n.push(t)}}})}return jn(ce(u(e)),n,r)}var Vn=false;ne().addEventListener("DOMContentLoaded",function(){Vn=true});function _n(e){if(Vn||ne().readyState==="complete"){e()}else{ne().addEventListener("DOMContentLoaded",e)}}function $n(){if(Q.config.includeIndicatorStyles!==false){const e=Q.config.inlineStyleNonce?` nonce="${Q.config.inlineStyleNonce}"`:"";ne().head.insertAdjacentHTML("beforeend","<style"+e+">      ."+Q.config.indicatorClass+"{opacity:0}      ."+Q.config.requestClass+" ."+Q.config.indicatorClass+"{opacity:1; transition: opacity 200ms ease-in;}      ."+Q.config.requestClass+"."+Q.config.indicatorClass+"{opacity:1; transition: opacity 200ms ease-in;}      </style>")}}function zn(){const e=ne().querySelector('meta[name="htmx-config"]');if(e){return S(e.content)}else{return null}}function Jn(){const e=zn();if(e){Q.config=ue(Q.config,e)}}_n(function(){Jn();$n();let e=ne().body;kt(e);const t=ne().querySelectorAll("[hx-trigger='restored'],[data-hx-trigger='restored']");e.addEventListener("htmx:abort",function(e){const t=e.target;const n=ie(t);if(n&&n.xhr){n.xhr.abort()}});const n=window.onpopstate?window.onpopstate.bind(window):null;window.onpopstate=function(e){if(e.state&&e.state.htmx){Yt();se(t,function(e){he(e,"htmx:restored",{document:ne(),triggerEvent:he})})}else{if(n){n(e)}}};E().setTimeout(function(){he(e,"htmx:load",{});e=null},0)});return Q}();
        )htmx";
        ///! handle events with inline scripts on elements
          template <const_string value = "None"> class $on : body_element {
          public:
            std::string &content;
            std::map<std::string, std::string> pairs;
            $on(std::string &content) : content(content) {
              content.insert(content.rfind(">"),
                             std::string(" hx-on-" +value+ "=\"") + value.c_str() + "\"");
            }
            operator std::string() const { return content; }
          };

          #define DEFINE_HTMX_ATTRIBUTE(NAME, ATTRIBUTE)                                                 \
            template <const_string value = "None"> class $##NAME : body_element {        \
            public:                                                                      \
              std::string &content;                                                      \
              std::map<std::string, std::string> pairs;                                  \
              $##NAME(std::string &content) : content(content) {                         \
                content.insert(content.rfind(">"),                                       \
                               std::string(ATTRIBUTE) + value.c_str() + "\"");     \
              }                                                                          \
              operator std::string() const { return content; }                           \
            };

          ///! issues a GET to the specified URL
          DEFINE_HTMX_ATTRIBUTE(get,"hx-get");
          ///! issues a POST to the specified URL
          DEFINE_HTMX_ATTRIBUTE(post,"hx-post");
          ///! push a URL into the browser location bar to create history
          DEFINE_HTMX_ATTRIBUTE(push_url,"hx-push-url");
          ///! select content to swap in from a response
          DEFINE_HTMX_ATTRIBUTE(select,"hx-select");
          ///! select content to swap in from a response, somewhere other than the target (out of band)
          DEFINE_HTMX_ATTRIBUTE(select_oob,"hx-select-oob");
          ///! controls how content will swap in (outerHTML, beforeend, afterend, …)
          DEFINE_HTMX_ATTRIBUTE(swap,"hx-swap");
          ///! mark element to swap in from a response (out of band)
          DEFINE_HTMX_ATTRIBUTE(swap_oob,"hx-swap-oob");
          ///! specifies the target element to be swapped
          DEFINE_HTMX_ATTRIBUTE(target,"hx-target");
          ///! specifies the event that triggers the request
          DEFINE_HTMX_ATTRIBUTE(trigger,"hx-trigger");
          ///! add values to submit with the request (JSON format)
          DEFINE_HTMX_ATTRIBUTE(vals,"hx-vals");
          ///! add progressive enhancement for links and forms
          DEFINE_HTMX_ATTRIBUTE(boost,"hx-boost");
          ///! shows a confirm() dialog before issuing a request
          DEFINE_HTMX_ATTRIBUTE(confirm,"hx-confirm");
          ///! issues a DELETE to the specified URL
          DEFINE_HTMX_ATTRIBUTE(delete,"hx-delete");
          ///! disables htmx processing for the given node and any children nodes
          DEFINE_HTMX_ATTRIBUTE(disable,"hx-disable");
          ///! adds the disabled attribute to the specified elements while a request is in flight
          DEFINE_HTMX_ATTRIBUTE(disabled_elt,"hx-disabled-elt");
          ///! control and disable automatic attribute inheritance for child nodes
          DEFINE_HTMX_ATTRIBUTE(disinherit,"hx-disinherit");
          ///! changes the request encoding type
          DEFINE_HTMX_ATTRIBUTE(encoding,"hx-encoding");
          ///! extensions to use for this element
          DEFINE_HTMX_ATTRIBUTE(ext,"hx-ext");
          ///! adds to the headers that will be submitted with the request
          DEFINE_HTMX_ATTRIBUTE(headers,"hx-headers");
          ///! prevent sensitive data being saved to the history cache
          DEFINE_HTMX_ATTRIBUTE(history,"hx-history");
          ///! the element to snapshot and restore during history navigation
          DEFINE_HTMX_ATTRIBUTE(history_elt,"hx-history-elt");
          ///! include additional data in requests
          DEFINE_HTMX_ATTRIBUTE(include,"hx-include");
          ///! the element to put the htmx-request class on during the request
          DEFINE_HTMX_ATTRIBUTE(indicator,"hx-indicator");
          ///! control and enable automatic attribute inheritance for child nodes if it has been disabled by default
          DEFINE_HTMX_ATTRIBUTE(inherit,"hx-inherit");
          ///! filters the parameters that will be submitted with a request
          DEFINE_HTMX_ATTRIBUTE(params,"hx-params");
          ///! issues a PATCH to the specified URL
          DEFINE_HTMX_ATTRIBUTE(patch,"hx-patch");
          ///! specifies elements to keep unchanged between requests
          DEFINE_HTMX_ATTRIBUTE(preserve,"hx-preserve");
          ///! shows a prompt() before submitting a request
          DEFINE_HTMX_ATTRIBUTE(prompt,"hx-prompt");
          ///! issues a PUT to the specified URL
          DEFINE_HTMX_ATTRIBUTE(put,"hx-put");
          ///! replace the URL in the browser location bar
          DEFINE_HTMX_ATTRIBUTE(replace_url,"hx-replace-url");
          ///! configures various aspects of the request
          DEFINE_HTMX_ATTRIBUTE(request,"hx-request");
          ///! control how requests made by different elements are synchronized
          DEFINE_HTMX_ATTRIBUTE(sync,"hx-sync");
          ///! force elements to validate themselves before a request
          DEFINE_HTMX_ATTRIBUTE(validate,"hx-validate");
          ///! adds values dynamically to the parameters to submit with the request (deprecated, please use hx-vals)
          DEFINE_HTMX_ATTRIBUTE(vars,"hx-vars");
}
    static js::builder htmx2(hx::htmx_2_0_1_str);
} // namespace hyper
