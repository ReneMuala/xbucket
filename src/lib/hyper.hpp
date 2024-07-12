
#include <map>
#include <string>
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
  html(const std::map<std::string, std::string> & fields = {}) {
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

namespace hx {
#define CREATE_HTMX_ATTRIBUTE(NAME)                                                 \
  template <const_string value = "None"> class $##NAME : body_element {        \
  public:                                                                      \
    std::string &content;                                                      \
    std::map<std::string, std::string> pairs;                                  \
    $##NAME(std::string &content) : content(content) {                         \
      content.insert(content.rfind(">"),                                       \
                     std::string(" hx-" #NAME "=\"") + value.c_str() + "\"");     \
    }                                                                          \
    operator std::string() const { return content; }                           \
  };


CREATE_HTMX_ATTRIBUTE(target);
CREATE_HTMX_ATTRIBUTE(delete);
} // namespace hx

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


} // namespace hyper