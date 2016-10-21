/*
 * webapp_proto_http.cpp
 *
 *  Created on: 30 авг. 2016 г.
 *      Author: denis
 */

#include "webapp_proto_http.hpp"
#include "enum_serializer.hpp"

#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

namespace webapp {

typedef ProtocolHTTP::Method                    HttpMethod;
typedef enum_serializer::Collection<HttpMethod> CollectionOfHttpMethods;

static CollectionOfHttpMethods http_methods([](CollectionOfHttpMethods &list) {
  list.Add("OPTION",  ProtocolHTTP::kOptions);
  list.Add("GET",     ProtocolHTTP::kGet);
  list.Add("HEAD",    ProtocolHTTP::kHead);
  list.Add("POST",    ProtocolHTTP::kPost);
  list.Add("PUT",     ProtocolHTTP::kPut);
  list.Add("DELETE",  ProtocolHTTP::kDelete);
  list.Add("TRACE",   ProtocolHTTP::kTrace);
  list.Add("CONNECT", ProtocolHTTP::kConnect);
});

typedef ProtocolHTTP::Content::Type::Name     MimeType;
typedef enum_serializer::Collection<MimeType> CollectionOfMimeTypes;

static CollectionOfMimeTypes mime_types([](CollectionOfMimeTypes &list) {
  list.Add("text",        MimeType::kText);
  list.Add("image",       MimeType::kImage);
  list.Add("audio",       MimeType::kAudio);
  list.Add("video",       MimeType::kVideo);
  list.Add("application", MimeType::kApplication);
  list.Add("multipart",   MimeType::kMultipart);
  list.Add("message",     MimeType::kMessage);
});

typedef ProtocolHTTP::Content::Type           FileType;
typedef enum_serializer::Collection<FileType> CollectionOfFileTypes;

static CollectionOfFileTypes file_types([](CollectionOfFileTypes &list) {
  // Список:
  // https://ru.wikipedia.org/wiki/%D0%A1%D0%BF%D0%B8%D1%81%D0%BE%D0%BA_MIME-%D1%82%D0%B8%D0%BF%D0%BE%D0%B2
  // text
  list.Add("txt",   FileType(MimeType::kText,        "plain"));
  list.Add("html",  FileType(MimeType::kText,        "html"));
  list.Add("htm",   FileType(MimeType::kText,        "html"));
  list.Add("css",   FileType(MimeType::kText,        "css"));
  list.Add("js",    FileType(MimeType::kText,        "javascript"));
  list.Add("json",  FileType(MimeType::kApplication, "json"));
  // images
  list.Add("bmp",   FileType(MimeType::kImage, "bmp"));
  list.Add("jpeg",  FileType(MimeType::kImage, "jpeg"));
  list.Add("jpg",   FileType(MimeType::kImage, "jpeg"));
  list.Add("png",   FileType(MimeType::kImage, "png"));
  list.Add("svg",   FileType(MimeType::kImage, "svg+xml"));
});

class Validator {
  public:
    typedef bool (*Handler)(Validator&);

    Validator(const std::string &str, Handler handler): _is_valid(false) {
      if (handler == 0) {
        return;
      }
      _sym_it = str.begin();
      for (; _sym_it != str.end(); _sym_it++) {
        if (not handler(*this)) {
          // DEBUG --------------------
//          std::cout << "DEBUG: " << __FUNCTION__ << ": " << __LINE__ << ": "
//                    << "sym: " << (*_sym_it) << " in " << str
//                    << std::endl;
          // --------------------------
          return;
        }
      }
      _is_valid = true;
    };

    bool Result() const {
      return _is_valid;
    }

    bool Sym(unsigned char sym) const {
      return (*_sym_it) == sym;
    }
    // Описание шаблонов:
    // http://www.ietf.org/rfc/rfc2396.txt -> "A. Collected BNF for URI"
    bool LowAlpha() const {
      return (*_sym_it) >= 'a' && (*_sym_it) <= 'z';
    }

    bool UpAlpha() const {
      return (*_sym_it) >= 'A' && (*_sym_it) <= 'Z';
    }

    bool Digit() const {
      return (*_sym_it) >= '0' && (*_sym_it) <= '9';
    }

    bool Alpha() const {
      return LowAlpha() || UpAlpha();
    }

    bool AlphaNum() const {
      return Alpha() || Digit();
    }

    bool Mark() const {
      const char kSym = *_sym_it;
      return kSym == '-' || kSym == '_' || kSym == '.'  || kSym == '!' ||
             kSym == '~' || kSym == '*' || kSym == 0x27 || kSym == '(' ||
             kSym == ')';
    }

    bool Reserved() const {
      const char kSym = *_sym_it;
      return kSym == ';' || kSym == '/' || kSym == '?' || kSym == ':' ||
             kSym == '@' || kSym == '&' || kSym == '=' || kSym == '+' ||
             kSym == '$' || kSym == ',';
    }

    bool Unreserved() const {
      return AlphaNum() || Mark();
    }

    bool Escaped() const {
      const char kSym = *_sym_it;
      return (kSym >= 'a' && kSym <= 'f') || kSym == '%' ||
             (kSym >= 'A' && kSym <= 'F') || Digit();
    }

    bool PChar() const {
      const char kSym = *_sym_it;
      return Unreserved() || Escaped() ||
             kSym == ':' || kSym == '@' || kSym == '&' || kSym == '=' ||
             kSym == '+' || kSym == '$' || kSym == ',';
    }

    bool PathSegments() const {
      return PChar() || Sym(';') || Sym('/');
    }
  private:
    std::string::const_iterator _sym_it;
    bool                        _is_valid;
}; // namespace webapp::validator

static
void UpperSymbolsToLower(std::string *in_out) {
  if (in_out == 0) {
    return;
  }
  std::string::iterator char_it = in_out->begin();
  static const unsigned char kRange = 'A' - 'a';
  for (; char_it != in_out->end(); char_it++) {
    if (*char_it >= 'A' && *char_it <= 'Z') {
      *char_it -= kRange;
    }
  }
}
// AbsoluteUri -----------------------------------------------------------------
void AbsoluteUri::set_offset(size_t val) {
  _offset = val;
}

size_t AbsoluteUri::get_offset() const {
  return _offset;
}

AbsoluteUri::AbsoluteUri(): _offset(0) {
}

AbsoluteUri::~AbsoluteUri() {
}

bool AbsoluteUri::ParseVal(const std::string &val) {
// The URI syntax is dependent upon the scheme.  In general, absolute
// URI are written as follows:
//    <scheme>:<scheme-specific-part>
// Разделитель '/' добавлен для случаев, когда указан относительный URI,
// т.е. scheme будет пустой:
//   /node0/node1/node2
// полный вариант:
//   http://node0/node1/node2
// В случае когда scheme не указана, мы не можем считать URI некорректным!
  const std::size_t kDelimOff = val.find_first_of(":/");
  _scheme = val.substr(0, kDelimOff);
  set_offset(kDelimOff);
  UpperSymbolsToLower(&_scheme);
  // scheme = alpha | digit | "+" | "-" | "."
  if (not Validator(_scheme, [](Validator &v) -> bool {
      return v.AlphaNum() || v.Sym('+') || v.Sym('-') || v.Sym('.');
    }).Result()) {
    _scheme.clear();
    return false;
  }
  return true;
}

const std::string& AbsoluteUri::get_scheme() const {
  return _scheme;
}
// ProtocolHTTP::Uri -----------------------------------------------------------
ProtocolHTTP::Uri::Uri(): AbsoluteUri() {
}

ProtocolHTTP::Uri::~Uri() {
}

bool ProtocolHTTP::Uri::CheckAuthority(const std::string &val) {
// The authority component is preceded by a double slash "//" and is
// terminated by the next slash "/", question-mark "?", or by the end of
// the URI.
// Общий вид для HTTP схемы: <scheme>://<userinfo>@<host>:<port>/<path>?<query>
  _authority = Authority();
  const size_t      kSchemeOff = get_scheme().size() > 0 ? 1 : 0;
  const std::string kPathPref(val.substr(get_offset() + kSchemeOff, 2));
  if (kPathPref != "//") {
    // если указан относительный путь
    if (kPathPref.at(0) == '/') {
      return true;
    }
    return false;
  }
  // ищем признак авторизации пользователя
  set_offset(get_offset() + 2 + kSchemeOff);
  const size_t kUInfoEndOff = val.find_first_of('@', get_offset());
  if (kUInfoEndOff != std::string::npos) {
    _authority.userinfo = val.substr(get_offset(), kUInfoEndOff - get_offset());
    // userinfo = unreserved | escaped | ";" | ":" | "&" | "=" | "+" | "$" | ","
    if (not Validator(_authority.userinfo, [](Validator &v) -> bool {
        return v.Unreserved() || v.Escaped() ||
               v.Sym(';') || v.Sym(':') || v.Sym('&') || v.Sym('=') ||
               v.Sym('+') || v.Sym('$') || v.Sym(',');
      }).Result()) {
      return false;
    }
    set_offset(kUInfoEndOff + 1);
  }
  // ищем адрес/имя хоста и номер его порта
  const size_t kEndOff  = val.find_first_of("/?", get_offset());
  if (kEndOff != std::string::npos) {
    const std::string kHostPort(val.substr(get_offset(), kEndOff - get_offset()));
    const size_t      kPortOff = kHostPort.find_first_of(':');
    _authority.host = kHostPort.substr(0, kPortOff);
    _authority.port = kPortOff != std::string::npos ? kHostPort.substr(kPortOff)
                                                    : "80";
    if (not Validator(_authority.port, [](Validator &v) -> bool {
        return v.Digit();
      }).Result()) {
      return false;
    }
  } else {
    _authority.host = val.substr(get_offset(), kEndOff - get_offset());
  }
  set_offset(kEndOff);
  // проверяем адрес/имя хоста
  return Validator(_authority.host, [](Validator &v) -> bool {
    return v.AlphaNum() || v.Sym('.') || v.Sym('-');
  }).Result();
}

bool ProtocolHTTP::Uri::CheckPath(const std::string &val) {
// The path may consist of a sequence of path segments separated by a
// single slash "/" character.  Within a path segment, the characters
// "/", ";", "=", and "?" are reserved.
  _path.clear();
  _path.reserve(10);
  if (get_offset() == std::string::npos || val.at(get_offset()) != '/') {
    return true;
  }
  size_t off = get_offset();
// Ищем разделитель '#', на нём заканчивается URI и начинается Fragment:
// URI-reference = [ absoluteURI | relativeURI ] [ "#" fragment ]
  const size_t kQuestionMark = val.find_first_of('?', off + 1);
  const size_t kCrosshatch   = val.find_first_of('#', off + 1);
  // чтение пути и разбиение его на сегменты
  std::string decoded_str;
  std::string encoded_str;
  while (off != kCrosshatch && off != kQuestionMark) {
    off = val.find_first_of("/", off + 1);
    if (kQuestionMark != std::string::npos && off > kQuestionMark) {
      off = kQuestionMark;
    }
    if (kCrosshatch != std::string::npos && off > kCrosshatch) {
      off = kCrosshatch;
    }
    encoded_str = val.substr(get_offset() + 1, off - get_offset() - 1);
    if (encoded_str.size() == 0) {
      set_offset(off);
      continue;
    }
    if (not Validator(encoded_str, [](Validator &v) -> bool {
        return v.PathSegments();
      }).Result()) {
      return false;
    }
    ProtocolHTTP::DecodeString(encoded_str, '%', &decoded_str);
    _path.push_back(decoded_str);
    set_offset(off);
  }
  return true;
}

bool ProtocolHTTP::Uri::CheckQuery(const std::string &val) {
// The query component is indicated by the first question
// mark ("?") character and terminated by a number sign ("#") character
// or by the end of the URI.
  _query.clear();
  if (get_offset() == std::string::npos || val.at(get_offset()) != '?') {
    return true;
  }
  size_t off = get_offset();
  const size_t kCrosshatchOff = val.find_first_of('#', off + 1);
  std::string encoded_str;
  std::string decoded_key;
  std::string decoded_val;
  while (off != kCrosshatchOff) {
    off = val.find_first_of("&", off + 1);
    if (kCrosshatchOff != std::string::npos && off > kCrosshatchOff) {
      off = kCrosshatchOff;
    }
    encoded_str = val.substr(get_offset() + 1, off - get_offset() - 1);
    if (not Validator(encoded_str, [](Validator &v) -> bool {
        return v.PChar() || v.Sym('/') || v.Sym('?');
      }).Result()) {
      return false;
    }
    const size_t kEqualSignOff = encoded_str.find_first_of('=');
    ProtocolHTTP::DecodeString(encoded_str.substr(0, kEqualSignOff), '%',
                                                  &decoded_key);
    ProtocolHTTP::DecodeString(encoded_str.substr(kEqualSignOff + 1), '%',
                                                  &decoded_val);
    _query.insert(QueryParam(decoded_key, decoded_val));
    set_offset(off);
  }
  return true;
}

bool ProtocolHTTP::Uri::CheckFragment(const std::string &val) {
  _fragment = "";
  if (get_offset() == std::string::npos || val.at(get_offset()) != '#') {
    return true;
  }
  const std::string kEncodedStr = val.substr(get_offset() + 1);
  // fragment = reserved | unreserved | escaped
  if (not Validator(kEncodedStr, [](Validator &v) -> bool {
      return v.Reserved() || v.Unreserved() || v.Escaped();
    }).Result()) {
    return false;
  }
  DecodeString(kEncodedStr, '%', &_fragment);
  return true;
}

bool ProtocolHTTP::Uri::ParseVal(const std::string &val) {
  return AbsoluteUri::ParseVal(val) &&
         CheckAuthority(val) &&
         CheckPath(val) &&
         CheckQuery(val) &&
         CheckFragment(val);
}

const ProtocolHTTP::Uri::Authority& ProtocolHTTP::Uri::get_authority() const {
  return _authority;
}

const ProtocolHTTP::Uri::Path& ProtocolHTTP::Uri::get_path() const {
  return _path;
}

const ProtocolHTTP::Uri::Query& ProtocolHTTP::Uri::get_query() const {
  return _query;
}

const std::string& ProtocolHTTP::Uri::get_fragment() const {
  return _fragment;
}
// ProtocolHTTP::Expires -------------------------------------------------------
ProtocolHTTP::Expires::Expires() {
  Now();
}

ProtocolHTTP::Expires::Expires(const std::string &time_value) {
  time = boost::posix_time::time_from_string(time_value);
}

ProtocolHTTP::Expires& ProtocolHTTP::Expires::Now() {
  // Можно использовать только время без часовых поясов!
  // https://tools.ietf.org/html/rfc7234#section-4.2
  // RFC:
  //  A cache recipient SHOULD consider a date with a zone abbreviation
  //  other than GMT or UTC to be invalid for calculating expiration.
  time = boost::posix_time::second_clock::universal_time();
  return *this;
}

ProtocolHTTP::Expires& ProtocolHTTP::Expires::Hour(USize    value) {
  time += boost::posix_time::hours(value);
  return *this;
}

ProtocolHTTP::Expires& ProtocolHTTP::Expires::Day (USize    value) {
  time += boost::posix_time::hours(value * 24);
  return *this;
}

ProtocolHTTP::Expires& ProtocolHTTP::Expires::Week(USize    value) {
  time += boost::posix_time::hours(value * 24 * 7);
  return *this;
}

std::string ProtocolHTTP::Expires::ToString() const {
  using namespace boost::posix_time;
  std::stringstream buff;
  // по поводу UTC см. выше, метод Now()
  time_facet *facet(new time_facet("%a, %d %b %Y %H:%M:%S UTC"));
  buff.imbue(std::locale(std::locale::classic(), facet));
  buff << time;
  return buff.str();
}
// CacheControl ----------------------------------------------------------------
static
void PushToStrList(const std::string &val, std::string *out) {
  if (out == 0 || val.size() == 0) {
    return;
  }
  if (out->size() > 0) {
    *out += ", ";
  }
  *out += val;
}

ProtocolHTTP::CacheControl& ProtocolHTTP::CacheControl::Reset() {
  _directive.clear();
  return *this;
}

ProtocolHTTP::CacheControl& ProtocolHTTP::CacheControl::MaxAge(unsigned seconds) {
  std::stringstream buff;
  buff << "max-age=" << seconds;
  PushToStrList(buff.str(), &_directive);
  return *this;
}

ProtocolHTTP::CacheControl& ProtocolHTTP::CacheControl::NoStore() {
  PushToStrList("no-store", &_directive);
  return *this;
}

ProtocolHTTP::CacheControl& ProtocolHTTP::CacheControl::NoCache() {
  PushToStrList("no-cache", &_directive);
  return *this;
}

ProtocolHTTP::CacheControl& ProtocolHTTP::CacheControl::MustRevalidate() {
  PushToStrList("must-revalidate", &_directive);
  return *this;
}

const std::string& ProtocolHTTP::CacheControl::get_directive() const {
  return _directive;
}
// ProtocolHTTP::ETag ----------------------------------------------------------
static std::string NumToStr(uint32_t num) {
  static const char kSymbols[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
  };
  std::string res;
  do {
    const uint32_t kTail = num % 10;
    res += kSymbols[kTail];
    num = num / 10;
  } while (num > 0);
  return res;
}

void ProtocolHTTP::ETag::Random() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  _value = "\"" + NumToStr(tv.tv_sec) + NumToStr(tv.tv_usec) + "\"";
}

void ProtocolHTTP::ETag::Predefined(const std::string &val) {
  if (val.size() > 0) {
    _value = "\"" + val + "\"";
  } else {
    _value = val;
  }
}

const std::string& ProtocolHTTP::ETag::get_directive() const {
  return _value;
}
// Protocol --------------------------------------------------------------------
Protocol::Protocol() {
}

Protocol::~Protocol() {
}
// ProtocolHTTP::Request::Field ------------------------------------------------
struct ProtocolHTTP::Request::Field::Data {
  Content::Type type;
  Storage::Ptr  value;
};

ProtocolHTTP::Request::Field::Field() {
  _data = new Data();
}

ProtocolHTTP::Request::Field::~Field() {
  delete _data;
}

ProtocolHTTP::Request::Field::Field(const Field &src) {
  _data = new Data();
  *_data = *src._data;
}

void ProtocolHTTP::Request::Field::operator= (const Field &src) {
  *_data = *src._data;
}

template <>
bool ProtocolHTTP::Request::Field::get_value(std::string *out) const {
  if (IsNull() || out == 0) {
    return false;
  }
  return _data->value->operator ()(out);
}

template <>
bool ProtocolHTTP::Request::Field::get_value(std::ostream *out) const {
  if (IsNull() || out == 0) {
    return false;
  }
  return _data->value->operator ()(out);
}

template <>
bool ProtocolHTTP::Request::Field::get_value(double *out) const {
  std::string out_str;
  if (out == 0 || not get_value(&out_str)) {
    return false;
  }
  *out = atof(out_str.c_str());
  return true;
}

template <>
bool ProtocolHTTP::Request::Field::get_value(int64_t *out) const {
  std::string out_str;
  if (out == 0 || not get_value(&out_str)) {
    return false;
  }
  *out = atol(out_str.c_str());
  return true;
}

static
bool ParseBoolValue(const std::string &val) {
  using namespace boost;
  return regex_match(val.begin(), val.end(), regex("true")) ? true : false;
}

template <>
bool ProtocolHTTP::Request::Field::get_value(bool *out) const {
  std::string out_str;
  if (out == 0 || not get_value(&out_str)) {
    return false;
  }
  *out = ParseBoolValue(out_str);
  return true;
}

const ProtocolHTTP::Content::Type& ProtocolHTTP::Request::Field::get_type() const {
  return _data->type;
}

bool ProtocolHTTP::Request::Field::IsNull() const {
  return not _data->value;
}

USize ProtocolHTTP::Request::Field::Size() const {
  if (IsNull()) {
    return 0;
  }
  return _data->value->Size();
}

void ProtocolHTTP::Request::Field::Clear() {
  if (IsNull()) {
    return;
  }
  _data->value->Clear();
}

bool ProtocolHTTP::Request::Field::UseStorage(Storage *storage) {
  if (storage == 0) {
    return false;
  }
  _data->value.reset(storage);
  return true;
}

bool ProtocolHTTP::Request::Field::UseStorage(Storage::Ptr storage) {
  if (not storage) {
    return false;
  }
  _data->value = storage;
  return true;
}

const std::string& ProtocolHTTP::Request::Field::GetNameOfStorage() const {
  if (IsNull()) {
    static std::string kEmptyStr;
    return kEmptyStr;
  }
  return _data->value->get_name();
}

bool ProtocolHTTP::Request::Field::Append(const Byte *data, USize    size) {
  if (IsNull()) {
    return false;
  }
  return _data->value->Append(data, size);
}
// ProtocolHTTP::Request::Field::Storage ---------------------------------------
const std::string& ProtocolHTTP::Request::Field::Storage::GetStaticName() {
  static const std::string kName("Storage");
  return kName;
}

ProtocolHTTP::Request::Field::Storage::Storage(const std::string &name)
    : _name(name) {
}

ProtocolHTTP::Request::Field::Storage::~Storage() {
}

const std::string& ProtocolHTTP::Request::Field::Storage::get_name() const {
  return _name;
}

struct ProtocolHTTP::Request::Field::StorageInMem::Container {
  class Segment {
    public:
      typedef std::list<Segment> List;
      Segment(const Byte *_data, USize _size)
          : size(_size),
            data(new Byte[_size]) {
        memcpy(data, _data, sizeof(Byte) * size);
      }
      ~Segment() {
        delete[] data;
      }
      USize     size;
      Byte     *data;
    private:
      Segment(const Segment&);
      void operator= (const Segment&);
  };

  void Clear() {
    size = 0;
    segments.clear();
  }

  void AppendData(const Byte *_data, USize _size) {
    segments.emplace_back(_data, _size);
    size += _size;
  }

  void Assign(const Container &src) {
    size = 0;
    segments.clear();
    Segment::List::const_iterator seg_it = src.segments.begin();
    for (; seg_it != src.segments.end(); seg_it++) {
      AppendData(seg_it->data, seg_it->size);
    }
  }

  USize         size;
  Segment::List segments;
};
// ProtocolHTTP::Request::Field::StorageInMem ----------------------------------
const std::string& ProtocolHTTP::Request::Field::StorageInMem::GetStaticName() {
  static const std::string kName("StorageInMem");
  return kName;
}

ProtocolHTTP::Request::Field::StorageInMem::StorageInMem()
    : Storage(StorageInMem::GetStaticName()) {
  _container = new Container();
}

ProtocolHTTP::Request::Field::StorageInMem::~StorageInMem() {
  delete _container;
}

ProtocolHTTP::Request::Field::StorageInMem::StorageInMem(
    const StorageInMem &src): Storage(StorageInMem::GetStaticName()) {
  _container = new Container();
  _container->Assign(*src._container);
}

void ProtocolHTTP::Request::Field::StorageInMem::operator= (
    const StorageInMem &src) {
  _container->Assign(*src._container);
}

USize ProtocolHTTP::Request::Field::StorageInMem::Size() const {
  return _container->size;
}

void ProtocolHTTP::Request::Field::StorageInMem::Clear() {
  _container->Clear();
}

bool ProtocolHTTP::Request::Field::StorageInMem::Append(const Byte *data,
                                                        USize       size) {
  _container->AppendData(data, size);
  return true;
}

bool ProtocolHTTP::Request::Field::StorageInMem::operator()(
    std::ostream *out) const {
  if (out == 0) {
    return false;
  }
  Container::Segment::List::const_iterator seg_it = _container->segments.begin();
  for (; seg_it != _container->segments.end(); seg_it++) {
    out->write((const char*)seg_it->data, seg_it->size);
  }
  return true;
}

bool ProtocolHTTP::Request::Field::StorageInMem::operator()(
    std::string *out) const {
  if (out == 0) {
    return false;
  }
  out->resize(_container->size);
  Container::Segment::List::const_iterator seg_it = _container->segments.begin();
  std::string::iterator out_it = out->begin();
  for (; seg_it != _container->segments.end(); seg_it++) {
    for (USize    off = 0; off < seg_it->size; off++) {
      *out_it = seg_it->data[off];
      out_it++;
    }
  }
  return true;
}
// ProtocolHTTP::Request -------------------------------------------------------
struct ProtocolHTTP::Request::State {
  State(): complete_body(false),
           boundary_was_found(false),
           body_size(0),
           body_last_match(0),
           storage_generator(0) {
  }

  bool                      complete_body;
  bool                      boundary_was_found;
  USize                     body_size;
  std::string               header_last_line;
  uint8_t                   body_last_match;
  Field::Map                fields_get;
  Field::Map                fields_post;
  Field::Storage::Generator storage_generator;
  Header                    header;
  Header                    post_header;
};

ProtocolHTTP::Request::Request() {
  _state = new State();
}

ProtocolHTTP::Request::~Request() {
  delete _state;
}

ProtocolHTTP::Request::Request(const Request &src) {
  _state = new State();
  *_state = *src._state;
}

void ProtocolHTTP::Request::operator= (const Request &src) {
  *_state = *src._state;
}

const ProtocolHTTP::Header& ProtocolHTTP::Request::GetHeader() const {
  return _state->header;
}

void ProtocolHTTP::Request::UseStorageGenerator(
    Field::Storage::Generator generator) {
  _state->storage_generator = generator;
}

bool ProtocolHTTP::Request::Completed() const {
  return _state->header.complete && _state->complete_body;
}

const std::string& ProtocolHTTP::Request::Get(const std::string &name) const {
  const Uri::Query &kQuery = _state->header.line.target.get_query();
  Uri::Query::const_iterator q_it = kQuery.find(name);
  if (q_it == kQuery.end()) {
    static const std::string kEmptyStr;
    return kEmptyStr;
  }
  return q_it->second;
}

template <>
std::string ProtocolHTTP::Request::Get(const std::string &name,
                                       const std::string &def_val) const {
  const std::string &kVal = Get(name);
  if (kVal.size() == 0) {
    return def_val;
  }
  return kVal;
}

template <>
int ProtocolHTTP::Request::Get(const std::string &name,
                               const int         &def_val) const {
  const std::string &kVal = Get(name);
  if (kVal.size() == 0) {
    return def_val;
  }
  return atol(kVal.c_str());
}

template <>
int64_t ProtocolHTTP::Request::Get(const std::string &name,
                                   const int64_t     &def_val) const {
  const std::string &kVal = Get(name);
  if (kVal.size() == 0) {
    return def_val;
  }
  return atoll(kVal.c_str());
}

template <>
double ProtocolHTTP::Request::Get(const std::string &name,
                                  const double      &def_val) const {
  const std::string &kVal = Get(name);
  if (kVal.size() == 0) {
    return def_val;
  }
  return atof(kVal.c_str());
}

template <>
bool ProtocolHTTP::Request::Get(const std::string &name,
                                const bool        &def_val) const {
  const std::string &kVal = Get(name);
  if (kVal.size() == 0) {
    return def_val;
  }
  return ParseBoolValue(kVal);
}

const ProtocolHTTP::Request::Field* ProtocolHTTP::Request::Post(
    const std::string &name) const {
  Field::Map::const_iterator f_it = _state->fields_post.find(name);
  if (f_it == _state->fields_post.end()) {
    return 0;
  }
  return &f_it->second;
}

void ProtocolHTTP::Request::ResetState() {
  delete _state;
  _state = new State();
}

static HttpMethod GetMethodFromStr(const std::string &name) {
  HttpMethod res;
  return http_methods.Find(name, &res) ? res : ProtocolHTTP::kUnknown;
}

static MimeType GetContentTypeFromStr(const std::string &name) {
  MimeType res;
  return mime_types.Find(name, &res) ? res : MimeType::kApplication;
}

static bool TruncateString(const std::string &in, std::string *out) {
  if (out == 0) {
    return false;
  }
  const size_t kFrom   = in.find_first_not_of(" \"");
  const size_t kAmount = in.find_last_not_of(" \"") + 1 - kFrom;
  // +1 необходим для учёта символа на котором остановился поиск, см. документацию:
  // ... The position of the first character that does not match. ...
  out->assign(in.substr(kFrom, kAmount));
  return true;
}

static std::string TruncateString(const std::string &in) {
  std::string res;
  TruncateString(in, &res);
  return res;
}

static bool SplitStringToList(const std::string          &str,
                              const std::string          &div,
                              ProtocolHTTP::ListOfString *out) {
  if (out == 0) {
    return false;
  }
  out->clear();
  size_t off = 0;
  std::string truncated_str;
  while (off != std::string::npos) {
    const size_t kPrevOff = (off == 0 ? 0 : off + 1);
    off = str.find_first_of(div, off + 1);
    TruncateString(str.substr(kPrevOff, off - kPrevOff), &truncated_str);
    out->push_back(truncated_str);
  }
  return true;
}

typedef void (*ParameterHandler)(const std::string &name,
                                 const std::string &value,
                                 void              *out);

static void DetectParametersForHeaderField(const std::string &field,
                                           size_t             off,
                                           void              *out,
                                           ParameterHandler   handler) {
  if (out == 0 || handler == 0) {
    return;
  }
  while (off != std::string::npos) {
    const size_t kPrevOff = off + 1;
    off = field.find_first_of(';', kPrevOff);
    const std::string kParam     = field.substr(kPrevOff, off - kPrevOff);
    const size_t      kEqualSign = kParam.find_first_of('=');
    const std::string kParamName = TruncateString(kParam.substr(0, kEqualSign));
    const std::string kParamVal  = TruncateString(kParam.substr(kEqualSign + 1));
    handler(kParamName, kParamVal, out);
  }
}

static bool DetectContentType(const std::string           &in,
                              ProtocolHTTP::Content::Type *out) {
  typedef ProtocolHTTP::Content::Type Type;
  if (out == 0) {
    return false;
  }
  const size_t kTypeSubNameOff = in.find_first_of('/');
  const size_t kSemicolonOff   = in.find_first_of(';', kTypeSubNameOff);
  std::string type_name = in.substr(0, kTypeSubNameOff);
  out->sub_type         = in.substr(kTypeSubNameOff + 1,
                                    kSemicolonOff - kTypeSubNameOff - 1);
  UpperSymbolsToLower(&type_name);
  UpperSymbolsToLower(&out->sub_type);
  out->name = GetContentTypeFromStr(type_name);
  DetectParametersForHeaderField(in, kSemicolonOff, out, [](
      const std::string &name,
      const std::string &value,
      void              *out) {
    Type *type = static_cast<Type*>(out);
    if (name == "boundary") {
      type->boundary = value;
      return;
    }
    if (name == "charset") {
      type->charset = value;
      return;
    }
  });
  return true;
}

static bool DetectContentDisposition(
    const std::string                  &in,
    ProtocolHTTP::Content::Disposition *out) {
  typedef ProtocolHTTP::Content::Disposition Disposition;
  if (out == 0) {
    return false;
  }
  const size_t kSemicolonOff = in.find_first_of(';');
  out->type = in.substr(0, kSemicolonOff);
  DetectParametersForHeaderField(in, kSemicolonOff, out, [](
      const std::string &name,
      const std::string &value,
      void              *out) {
    Disposition *disp = static_cast<Disposition*>(out);
    if (name == "name") {
      disp->name = value;
      return;
    }
    if (name == "filename") {
      disp->filename = value;
      return;
    }
  });
  return true;
}

static bool ParseHeaderField(const std::string    &line,
                             ProtocolHTTP::Header *out) {
  if (out == 0) {
    return false;
  }
  if (line.size() < 2) {
    return true;
  }
  // https://tools.ietf.org/html/rfc7230#section-3.2
  const size_t      kSpaceOff = line.find_first_of(' ');
  const std::string kField    = line.substr(0, kSpaceOff);
  const size_t      kColonOff = kField.find_first_of(':');
  // если не найден разделитель ':', тогда перед нами start-line
  if (kColonOff == std::string::npos) {
    out->line.method = GetMethodFromStr(kField);
    if (out->line.method == ProtocolHTTP::kUnknown) {
      return false;
    }
    const size_t kVerOff = line.find_first_of(' ', kSpaceOff + 1);
    if (not out->line.target.ParseVal(line.substr(kSpaceOff + 1,
                                                  kVerOff - kSpaceOff - 1))) {
      return false;
    }
    out->line.version = line.substr(kVerOff + 1);
  }
  // разбор полей
  const size_t      kNameOff   = line.find_first_not_of(' ');
  const std::string kFieldName = line.substr(kNameOff, kColonOff - kNameOff);
  const std::string kFieldVal  = TruncateString(line.substr(kColonOff + 1));
  if (kFieldName == "User-Agent") {
    out->user_agent = kFieldVal;
    return true;
  }
  if (kFieldName == "Host") {
    out->host = kFieldVal;
    return true;
  }
  if (kFieldName == "Accept-Language") {
    return SplitStringToList(kFieldVal, ",", &out->accept.language);
  }
  if (kFieldName == "Accept-Charset") {
    return SplitStringToList(kFieldVal, ",", &out->accept.charset);
  }
  if (kFieldName == "Accept-Encoding") {
    return SplitStringToList(kFieldVal, ",", &out->accept.encoding);
  }
  if (kFieldName == "Content-Type") {
    return DetectContentType(kFieldVal, &out->content.type);
  }
  if (kFieldName == "Content-Disposition") {
    return DetectContentDisposition(kFieldVal, &out->content.disposition);
  }
  if (kFieldName == "Content-Length") {
    out->content.length = strtoul(kFieldVal.c_str(), 0, 0);
    return true;
  }
  if (kFieldName == "Content-Encoding") {
    return SplitStringToList(kFieldVal, ",", &out->content.encoding);
  }
  if (kFieldName == "Content-Language") {
    return SplitStringToList(kFieldVal, ",", &out->content.language);
  }
  if (kFieldName == "Content-Location") {
    return out->content.location.ParseVal(kFieldVal);
  }
  return true;
}
// ProtocolHTTP::Response::Source ----------------------------------------------
ProtocolHTTP::Response::Source::~Source() {
}

ProtocolHTTP::Response::SourceFromFile::SourceFromFile(
    const std::string &file_name)
    : _file(file_name.c_str(), std::fstream::in),
      _file_size(0) {
  if (not IsAvailable()) {
    return;
  }
  _file.seekg(0, std::ios_base::end);
  _file_size = _file.tellg();
  _file.seekg(0, std::ios_base::beg);
  _file.clear();
}

ProtocolHTTP::Response::SourceFromFile::~SourceFromFile() {
  _file.close();
}

bool ProtocolHTTP::Response::SourceFromFile::IsAvailable() const {
  return _file.is_open() && _file.good();
}

USize ProtocolHTTP::Response::SourceFromFile::Size() const {
  return _file_size;
}

USize ProtocolHTTP::Response::SourceFromFile::ReadSome(
    Byte  *out,
    USize  max_size) {
  if (not IsAvailable()) {
    return 0;
  }
  const USize kRealSize = _file.readsome(reinterpret_cast<char*>(out), max_size);
  if (_file.tellg() == Size()) {
    _file.setstate(std::ios::eofbit);
  }
  return kRealSize;
}
// SourceFromStream
ProtocolHTTP::Response::SourceFromStream::SourceFromStream(const std::string &src)
    : _buff_size(0) {
  _buff.str(src);
  _buff_size = src.size();
}

ProtocolHTTP::Response::SourceFromStream::SourceFromStream(std::stringstream *src)
    : _buff_size(0) {
  if (src == 0) {
    return;
  }
  // TODO: gcc-4.9 не поддерживает это
  //_buff.swap(*src);
  //_buff = *src;
  _buff.str(src->str());
  _buff.seekg(0, std::ios_base::end);
  _buff_size = _buff.tellg();
  _buff.seekg(0, std::ios_base::beg);
  _buff.clear();
}

ProtocolHTTP::Response::SourceFromStream::~SourceFromStream() {
}

bool ProtocolHTTP::Response::SourceFromStream::IsAvailable() const {
  return _buff.good() && (Size() > 0);
}

USize ProtocolHTTP::Response::SourceFromStream::Size() const {
  return _buff_size;
}

USize ProtocolHTTP::Response::SourceFromStream::ReadSome(Byte  *out,
                                                         USize  max_size) {
  if (not IsAvailable()) {
    return 0;
  }
  const USize kRealSize = _buff.readsome(reinterpret_cast<char*>(out), max_size);
  if (_buff.tellg() == Size()) {
    _buff.setstate(std::ios::eofbit);
  }
  return kRealSize;
}
// SourceFromArray
ProtocolHTTP::Response::SourceFromArray::SourceFromArray(const Byte *data,
                                                         USize       size)
    : _data(data), _size(size), _offset(0) {
}

ProtocolHTTP::Response::SourceFromArray::~SourceFromArray() {
}

bool  ProtocolHTTP::Response::SourceFromArray::IsAvailable() const {
  return (_data != 0 && _offset < _size);
}

USize ProtocolHTTP::Response::SourceFromArray::Size() const {
  return _size;
}

USize ProtocolHTTP::Response::SourceFromArray::ReadSome(Byte *out, USize max_size) {
  if (out == 0 || _data == 0) {
    return 0;
  }
  const USize kAvailableSz = _size - _offset;
  const USize kSize        = kAvailableSz > max_size ? max_size : kAvailableSz;
  memcpy(out, &_data[_offset], kSize);
  _offset += kSize;
  return kSize;
}
// ProtocolHTTP::Response ------------------------------------------------------
struct ProtocolHTTP::Response::State {
  State(): status_id(k404) {}

  Code        status_id;
  Header      header;
  Source::Ptr src_header;
  Source::Ptr src_body;
};

ProtocolHTTP::Response::Response() {
  _state = new State();
}

ProtocolHTTP::Response::~Response() {
  delete _state;
}

ProtocolHTTP::Header ProtocolHTTP::Response::GetHeaderForText(
    const std::string &format,
    const std::string &charset) {
  Content::Type type;
  Header        header;

  type.name     = Content::Type::kText;
  type.sub_type = format.size() == 0 ? "html" : format;
  type.charset  = charset.size() == 0 ? "utf-8" : charset;
  header.content.type = type;
  header.expires.Now().Hour();
  return header;
}

ProtocolHTTP::Header ProtocolHTTP::Response::GetHeaderForImage(
    const std::string &format) {
  Content::Type type;
  Header        header;
  if (format.size() > 0) {
    type.name     = Content::Type::kImage;
    type.sub_type = format;
  } else {
    type.name     = Content::Type::kApplication;
    type.sub_type = "octet-stream";
  }
  header.content.type = type;
  header.expires.Now().Hour();
  return header;
}

ProtocolHTTP::Header ProtocolHTTP::Response::GetHeaderForJSON() {
  Content::Type type;
  Header        header;

  type.name     = Content::Type::kApplication;
  type.sub_type = "json";
  header.content.type = type;
  header.expires.Now();
  return header;
}

static std::string GetResponseStatus(ProtocolHTTP::Code status_id) {
  std::stringstream res;
  res << (unsigned)status_id << " ";
  switch (status_id) {
    // 200
    case ProtocolHTTP::k200:
      res << "OK";
      break;
    case ProtocolHTTP::k202:
      res << "Accepted";
      break;
    // 300
    case ProtocolHTTP::k301:
      res << "Moved Permanently";
      break;
    case ProtocolHTTP::k304:
      res << "Not Modified";
      break;
    // 400
    case ProtocolHTTP::k400:
      res << "Bad Request";
      break;
    case ProtocolHTTP::k403:
      res << "Forbidden";
      break;
    case ProtocolHTTP::k404:
      res << "Not Found";
      break;
    // 500
    case ProtocolHTTP::k500:
      res << "Internal Server Error";
      break;
    case ProtocolHTTP::k505:
      res << "HTTP Version Not Supported";
      break;
    case ProtocolHTTP::k501:
    default:
      res << "Not Implemented";
      break;
  };
  return res.str();
}

static bool SetupHeader(ProtocolHTTP::Response::State *state) {
  if (state == 0 || state->src_header) {
    return false;
  }
  static const char kCrLf[] = "\r\n";
  const std::string kCacheControl = state->header.cache_control.get_directive();
  const std::string kETag         = state->header.etag.get_directive();
  std::stringstream str_h;
  str_h << "HTTP/1.1 "        << GetResponseStatus(state->status_id)
        << kCrLf
        << "Content-Type: "   << mime_types.Find(state->header.content.type.name)
                              << "/" << state->header.content.type.sub_type
        << kCrLf
        << "Content-Length: " << state->header.content.length
        << kCrLf
        << "Expires: "        << state->header.expires.ToString()
        << kCrLf;
  if (kCacheControl.size() != 0) {
    str_h << "Cache-Control: " << kCacheControl
          << kCrLf;
    state->header.cache_control.Reset();
  }
  if (kETag.size() != 0) {
    str_h << "ETag: " << kETag
          << kCrLf;
  }
  str_h << kCrLf;
  state->src_header.reset(new ProtocolHTTP::Response::SourceFromStream(str_h.str()));
  return true;
}

ProtocolHTTP::CacheControl& ProtocolHTTP::Response::UseCacheControl() {
  return _state->header.cache_control;
}

ProtocolHTTP::ETag& ProtocolHTTP::Response::UseETag() {
  return _state->header.etag;
}

const ProtocolHTTP::Header& ProtocolHTTP::Response::GetHeader() const {
  return _state->header;
}

void ProtocolHTTP::Response::SetHeader(Code status_id) {
  _state->status_id = status_id;
  _state->header    = GetHeaderForText("", "");
}

void ProtocolHTTP::Response::SetHeader(Code status_id, const Header &header) {
  _state->status_id = status_id;
  _state->header    = header;
}

void ProtocolHTTP::Response::SetHeader(const std::string &type) {
  if (type.size() == 0) {
    return;
  }
  FileType ftype;
  Header   hd;
  if (file_types.Find(type, &hd.content.type)) {
    SetHeader(k200, hd);
  }
}

void ProtocolHTTP::Response::SetBody(const std::string &src) {
  if (_state->status_id == k404) {
    SetHeader(k200, GetHeaderForText("html", ""));
  }
  _state->src_body.reset(new SourceFromStream(src));
  _state->header.content.length = src.size();
  SetupHeader(_state);
}

void ProtocolHTTP::Response::SetBody(std::stringstream *src) {
  if (_state->status_id == k404) {
    SetHeader(k200, GetHeaderForText("html", ""));
  }
  if (src != 0) {
    _state->src_body.reset(new SourceFromStream(src));
    _state->header.content.length = _state->src_body->Size();
  }
  SetupHeader(_state);
}

void ProtocolHTTP::Response::SetBody(const Byte  *data,
                                           USize  size) {
  _state->src_body.reset(new SourceFromArray(data, size));
  _state->header.content.length = size;
  SetupHeader(_state);
}

bool ProtocolHTTP::Response::UseFile(const Content::Type &type,
                                     const std::string   &path) {
  _state->src_body.reset(new SourceFromFile(path));
  if (not _state->src_body->IsAvailable()) {
    return false;
  }
  Header header;
  header.expires.Week();
  header.content.type   = type;
  header.content.length = _state->src_body->Size();
  SetHeader(k200, header);
  return true;
}

void ProtocolHTTP::Response::ResetState() {
  delete _state;
  _state = new State();
}
// ProtocolHTTP::Router --------------------------------------------------------
ProtocolHTTP::Router::Functor::Functor(Callback cb)
    : _callback(cb) {
}

ProtocolHTTP::Router::Functor::~Functor() {
}

bool ProtocolHTTP::Router::Functor::operator()(const Uri::Path &path,
                                               const Request   &request,
                                                     Response  *response) {
  if (_callback != 0) {
    return _callback(path, request, response);
  }
  return false;
}

struct ProtocolHTTP::Router::State {
  struct Node {
    typedef std::map<std::string, Node>  Map;
    typedef std::pair<std::string, Node> Rec;

    Node() {}
    Node(const Node&) {}
    void operator= (const Node&) {}

    ProtocolHTTP::Router::Functor::Ptr handler;
    Map                                children;
  };

  Node      nodes;
  Uri::Path root_url;
};

ProtocolHTTP::Router::Router() {
  _state = new State();
}

ProtocolHTTP::Router::Router(const std::string &root_url) {
  Uri u;
  u.ParseVal(root_url);
  _state = new State();
  _state->root_url = u.get_path();
}

ProtocolHTTP::Router::~Router() {
  delete _state;
}

static
const ProtocolHTTP::Uri::Path& SelectPath(const ProtocolHTTP::Uri::Path &path_0,
                                          const ProtocolHTTP::Uri::Path &path_1) {
  return path_0.size() > 0 ? path_0 : path_1;
}

bool ProtocolHTTP::Router::CallHandlerFor(const Request &request,
                                          Response      *response) {
  const Uri::Path &kPath = SelectPath(request.GetHeader().line.target.get_path(),
                                      _state->root_url);
  Uri::Path::const_iterator path_it = kPath.begin();
  State::Node *node_it = &_state->nodes;
  Functor     *handler = 0;
  for (; path_it != kPath.end(); path_it++) {
    State::Node::Map::iterator it = node_it->children.find(*path_it);
    if (it == node_it->children.end()) {
      break;
    }
    handler = it->second.handler.get();
    node_it = &it->second;
  }
  return handler != 0 ? (*handler)(Uri::Path(path_it, kPath.end()),
                                   request,
                                   response) : false;
}

bool ProtocolHTTP::Router::AddHandlerFor(const std::string  &url,
                                               Functor::Ptr &functor) {
  State::Node *node_it   = &_state->nodes;
  size_t       prev_off  = 0;
  size_t       off       = 0;
  ssize_t      first_off = url.at(0) == '/' ? 0 : -1;
  do {
    prev_off  = off + 1 + first_off;
    off       = url.find_first_of('/', prev_off);
    first_off = 0;
    const size_t kPartSz = off - prev_off;
    if (kPartSz < 2) {
      continue;
    }
    const std::string kPart(url.substr(prev_off, kPartSz));
    State::Node::Map::iterator cur_node = node_it->children.insert(
        State::Node::Rec(kPart, State::Node())
    ).first;
    node_it = &cur_node->second;
  } while (off != std::string::npos);
  if (node_it->handler) {
    return false;
  }
  node_it->handler = functor;
  return true;
}

bool ProtocolHTTP::Router::AddHandlerFor(const std::string &url,
                                         const Functor     &functor) {
  Functor::Ptr func_ptr(new Functor(functor));
  return AddHandlerFor(url, func_ptr);
}

static ProtocolHTTP::Content::Type GetContentTypeFor(
    const boost::filesystem::path &path2file) {
  const std::string kName(path2file.filename().string());
  const size_t      kDotOff = kName.find_last_of('.');
  if (kDotOff == std::string::npos) {
    return ProtocolHTTP::Content::Type();
  }
  const std::string kExt(kName, kDotOff + 1);
  FileType ftype;
  if (file_types.Find(kExt, &ftype)) {
    return ftype;
  }
  return ProtocolHTTP::Content::Type();
}

bool ProtocolHTTP::Router::AddDirectoryFor(const std::string &url,
                                           const std::string &dir) {
  struct DirInfo : public Functor {
    DirInfo(const std::string &val): Functor(0), path_to_dir(val) {}
    virtual ~DirInfo() {}
    virtual bool operator()(const Uri::Path &path,
                            const Request   &,
                                  Response  *response) {
      std::string path_str(path_to_dir);
      Uri::Path::const_iterator it = path.begin();
      for (; it != path.end(); it++) {
        path_str += "/" + *it;
      }
      namespace fs = boost::filesystem;
      const fs::path kPath(path_str);
      if (not fs::exists(kPath)) {
        return false;
      }
      if (fs::is_directory(kPath)) {
        // TODO
        return false;
      }
      return response->UseFile(GetContentTypeFor(path_str), path_str);
    }
    const std::string path_to_dir;
  };

  DirInfo::Ptr func_ptr(new DirInfo(dir));
  return AddHandlerFor(url, func_ptr);
}

ProtocolHTTP::Router::Ptr ProtocolHTTP::Router::Create() {
  return ProtocolHTTP::Router::Ptr(new ProtocolHTTP::Router());
}
// ProtocolHTTP ----------------------------------------------------------------
struct ProtocolHTTP::State {
  State(): need_to_close(false) {}

  Router::Ptr router;
  Request     request;
  Response    response;
  bool        need_to_close;
};

ProtocolHTTP::ProtocolHTTP(Router::Ptr router): Protocol() {
  _state = new State();
  _state->router = router;
}

ProtocolHTTP::~ProtocolHTTP() {
  delete _state;
}

void ProtocolHTTP::DecodeString(const std::string &in,
                                char               label,
                                std::string       *out) {
  if (out == 0) {
    return;
  }
  out->clear();
  bool in_hex = false;
  std::string hex;
  std::string::const_iterator sym_it = in.begin();
  for (; sym_it != in.end(); sym_it++) {
    const char kSym = *sym_it;
    if (kSym == label) {
      in_hex = true;
      hex.clear();
      continue;
    }
    if (in_hex) {
      hex.push_back(kSym);
      if (hex.size() == 2) {
        in_hex = false;
        out->push_back((char)strtol(hex.c_str(), 0, 16));
      }
      continue;
    }
    out->push_back(kSym);
  }
}

static USize ParseRequestHeader(ProtocolHTTP::Byte           *req_bytes,
                                const size_t                  size,
                                ProtocolHTTP::Request::State *out_state) {
  static const size_t kLineMaxLen  = 512;
  static const size_t kLineAverLen = 256;
  std::string line(out_state->header_last_line);
  line.reserve(kLineAverLen);
  out_state->header.complete = false;
  size_t offs = 0;
  for (; offs < size; offs++) {
    const ProtocolHTTP::Byte kByte = req_bytes[offs];
    // пропускаем символ CR
    if (kByte == 0x0D) {
      continue;
    }
    // учитываем символ LF, и принимаем его как признак конца строки
    if (kByte == 0x0A) {
      // Пустая строка, признак окончания заголовка
      if (line.size() < 2) {
        out_state->header_last_line.clear();
        // Условие offs > 2, для того что бы алгоритм не находил признак окончания
        // заголовка в начале массива. Иначе мы получим пустой заголовок!
        if (offs > 2) {
          out_state->header.complete = true;
        }
        // +1 нужен для смещения на символ следующий, после обнаруженной пустой
        // строки. Если убрать +1 то смещение будет указывать на пустую строку.
        offs++;
        break;
      }
      if (not ParseHeaderField(line, &out_state->header)) {
        // TODO: регистрация ошибок
      }
      line.clear();
      continue;
    }
    line.push_back((char)kByte);
    if (line.size() > kLineMaxLen) {
      // TODO: регистрация ошибок
      break;
    }
  }
  if (not out_state->header.complete) {
    out_state->header_last_line = line;
  }
  return offs;
}

static size_t FindMultipartBoundary(const std::string            &boundary,
                                    ProtocolHTTP::Byte           *req_bytes,
                                    const size_t                  size,
                                    ProtocolHTTP::Request::State *out_state) {
  if (req_bytes == 0 || boundary.size() == 0) {
    return 0;
  }
  const std::string &kFieldName = out_state->post_header.content.disposition.name;
  ProtocolHTTP::Request::Field *field = 0;
  if (kFieldName.size() > 0) {
    field = &out_state->fields_post[kFieldName];
    if (field && field->IsNull()) {
      if (out_state->storage_generator != 0) {
        field->UseStorage(out_state->storage_generator(out_state->post_header.content.type));
      }
      if (field->IsNull()) {
        field->UseStorage(new ProtocolHTTP::Request::Field::StorageInMem());
      }
    }
  }
  const std::string kBoundary("--" + boundary + "\r\n");
  uint8_t match = out_state->body_last_match;
  size_t  off   = 0;
  out_state->boundary_was_found = false;
  for (; off < size; off++) {
    const ProtocolHTTP::Byte kByte = req_bytes[off];
    if (match < kBoundary.size() && kByte == kBoundary.at(match)) {
      match++;
    } else {
      match = 0;
    }
    // признак начала, части составного сообщения, найден
    if (match == kBoundary.size()) {
      if (field != 0 && not field->IsNull() && off > match) {
        field->Append(req_bytes, off - (match + 1));
      }
      out_state->body_last_match    = 0;
      out_state->boundary_was_found = true;
      // +1 нужен для последующего поиска, что бы он не начинался с последнего
      // символа последовательности "boundary"
      off++;
      break;
    }
  }
  out_state->body_last_match = match;
  if (not out_state->boundary_was_found && field != 0 && not field->IsNull()) {
    field->Append(req_bytes, size);
  }
  return off;
}
/**
 * Функция для разбора пакета состоящего из нескольких частей, содержащих
 * данные различных типов.
 * @param content   Этот аргумент должен передаваться через копирование, нельзя
 *                  использовать ссылки и указатели, это нарушит работу алгоритма!
 * @param req_bytes
 * @param size
 * @param out_state
 * @return
 */
static bool ParseMultipartRequestBody(const ProtocolHTTP::Content   content,
                                      ProtocolHTTP::Byte           *req_bytes,
                                      const size_t                  size,
                                      ProtocolHTTP::Request::State *out_state) {
  if (req_bytes == 0 || out_state == 0) {
    return false;
  }
  const ProtocolHTTP::Header kOutStateHeader = out_state->header;
  size_t off = 0;
  while (off < size) {
    if (out_state->post_header.complete || not out_state->boundary_was_found) {
      const size_t kPartOff = FindMultipartBoundary(content.type.boundary,
                                                    &req_bytes[off],
                                                    size - off,
                                                    out_state);
      // Необходимо сбросить последний найденый заголовок, потому что он уже был
      // использован, внутри FindMultipartBoundary, и последующее его использование
      // является некорректным. Делать это можно только в случае когда был обнаружен
      // полный boundary
      if (out_state->boundary_was_found) {
        out_state->post_header = ProtocolHTTP::Header();
      }
      off += kPartOff;
      if (off >= size || kPartOff == 0) {
        break;
      }
    }
    if (not out_state->post_header.complete) {
      out_state->header = out_state->post_header;
    }
    const USize kBodyOffs = ParseRequestHeader(&req_bytes[off],
                                               size - off,
                                               out_state);
    out_state->post_header = out_state->header;
    off += kBodyOffs;
  } // while
  out_state->body_size += off;
  out_state->header     = kOutStateHeader;
  if (out_state->body_size == out_state->header.content.length) {
    out_state->complete_body = true;
  }
  return true;
}

static bool ParseRequestBody(ProtocolHTTP::Byte           *req_bytes,
                             const size_t                  size,
                             ProtocolHTTP::Request::State *out_state) {
  if (req_bytes == 0 || out_state == 0) {
    return false;
  }
  if (out_state->header.content.type.name == ProtocolHTTP::Content::Type::kMultipart) {
    return ParseMultipartRequestBody(out_state->header.content,
                                     req_bytes,
                                     size,
                                     out_state);
  }
  return true;
}

bool ProtocolHTTP::ParseRequest(Byte         *req_bytes,
                                const size_t  size,
                                Request      *out) {
  if (req_bytes == 0 || out == 0 || size == 0) {
    return false;
  }
  USize body_off = 0;
  if (not out->_state->header.complete) {
    body_off = ParseRequestHeader(req_bytes, size, out->_state);
    if (not out->_state->header.complete) {
      return true;
    }
  }
  if (out->_state->header.content.length == 0) {
    out->_state->complete_body = true;
  }
  if (not out->_state->complete_body) {
    return ParseRequestBody(&req_bytes[body_off], size - body_off, out->_state);
  }
  return true;
}

USize ProtocolHTTP::GetResponse(Byte     *out_resp_bytes,
                                size_t    out_size,
                                Response *resp) {
  if (out_resp_bytes == 0 || resp == 0 || out_size == 0) {
    return 0;
  }
  if (not resp->_state->src_header && not SetupHeader(resp->_state)) {
    return 0;
  }
  if (resp->_state->src_header->IsAvailable()) {
    return resp->_state->src_header->ReadSome(out_resp_bytes, out_size);
  }
  if (not resp->_state->src_body || not resp->_state->src_body->IsAvailable()) {
    return 0;
  }
  return resp->_state->src_body->ReadSome(out_resp_bytes, out_size);
}

bool ProtocolHTTP::HandleRequest(Byte *data, USize size) {
  const bool kParseRes = ParseRequest(data, size, &_state->request);
  const bool kComplete = _state->request.Completed();
  if (kComplete) {
    if (not _state->router->CallHandlerFor(_state->request, &_state->response)) {
      _state->response.SetHeader(k404);
    }
  }
  return (kParseRes && not kComplete);
}

USize ProtocolHTTP::PrepareResponse(Byte *data, USize size) {
  const USize kSize = GetResponse(data, size, &_state->response);
  if (kSize == 0) {
    _state->response.ResetState();
    _state->request.ResetState();
    _state->need_to_close = true;
  }
  return kSize;
}

bool ProtocolHTTP::NeedToCloseSession() const {
  return _state->need_to_close;
}
// ServerHttp ------------------------------------------------------------------
ServerHttp::ServerHttp(ProtocolHTTP::Router::Ptr router)
    : Server(), _router(router) {
}

ServerHttp::~ServerHttp() {
}

Protocol* ServerHttp::InitProtocol() {
  return new ProtocolHTTP(_router);
}

} // namespace webapp
