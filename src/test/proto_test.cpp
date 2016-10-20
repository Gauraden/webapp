
#include <boost/test/unit_test.hpp>
#include <sstream>
#include <iostream>
#include "webapp_lib.hpp"

static const unsigned char kSp     = 0x20;
static const unsigned char kCrLf[] = "\r\n";//{0x0D, 0x0A};
static const unsigned char kOWS    = kSp;

typedef webapp::ProtocolHTTP::ListOfString ListOfString;

static const std::string kUserAgent("curl/7.16.3 libcurl/7.16.3 OpenSSL/0.9.7l zlib/1.2.3");
static const std::string kHost("www.example.com");
static const std::string kAcceptLanguage("en, mi, ru");
static const std::string kAcceptCharset("iso-8859-5, unicode-1-1;q=0.8");
static const std::string kAcceptEncoding("gzip;q=1.0, identity; q=0.5, *;q=0");
static const std::string kBoundary("gc0pJq0M:08jU534c0p");
static const std::string kContentType("multipart/mixed; boundary=\"" + kBoundary
                                      + "\";charset=ISO-8859-4");
static const std::string kContentEncoding("gzip");
static const std::string kContentLanguage("mi, en");
static const std::string kContentLocation("/hello.txt");
//static const std::string kContentLength("3495");
static const std::string kMessageTitle("Привет мир!\nHello world!");
static const std::string kDestAddress("brutal-vasya@example.com");
static const std::string kAttachedFile1("the image must be here");
// Помощник.
// Данная структура будет передана каждому тесту!
// Конструктор и деструктор обязательны!
struct ProtocolTestFixture {
  struct Packet {
    Packet& AddStartLine(const std::string &method,
                      const std::string &target,
                      const std::string &version) {
      buff.str(std::string());
      body_buff.str(std::string());
      buff << method << kSp << target << kSp << version << kCrLf;
      return *this;
    }
    Packet& AddField(const std::string &name, const std::string &value) {
      buff << name << ":" << kOWS << value << kOWS << kCrLf;
      return *this;
    }
    Packet& AddField(const std::string &name, const size_t &value) {
      buff << name << ":" << kOWS << value << kOWS << kCrLf;
      return *this;
    }
    Packet& AddBody(const std::string &body) {
      body_buff << kCrLf << body;
      return *this;
    }
    Packet& AddMultipartBody(const std::string &boundary,
                          const std::string &disposition,
                          const std::string &type,
                          const std::string &body) {
      body_buff << kCrLf
           << "--" << boundary << kCrLf;
      if (disposition.size() > 0) {
        body_buff << "Content-Disposition: " << disposition << kCrLf;
      }
      if (type.size() > 0) {
        body_buff << "Content-Type: " << type << kCrLf;
      }
      body_buff << kCrLf;
      if (body.size() > 0) {
        body_buff << body;
      }
      return *this;
    }
    Packet& Finalize() {
      body_buff.seekp(0, std::ios_base::end);
      const ssize_t kBodySize = body_buff.tellp();
      char *data = new char[kBodySize];
      // -2 для исключения из размера, пустой строки, отделяющей заголовок от
      // тела. Данная строка присутствует всегда, даже когда в пакете нет тела,
      // по этому она не учитывается
      if (kBodySize > 0) {
        AddField("Content-Length", kBodySize - 2);
        body_buff.read(data, kBodySize);
        buff.write(data, kBodySize);
      }
      delete[] data;
      return *this;
    }
    size_t ConvertToArray(webapp::Protocol::Byte *out, size_t max_size) {
      buff.seekg(0, std::ios_base::beg);
      return buff.readsome((char*)out, max_size);
    }
    size_t ConvertToArrayAsBroken(webapp::Protocol::Byte *out, size_t max_size) {
      // characters not allowed in a token (x22 and "(),/:;<=>?@[\]{}")
      const size_t kActualSize = buff.readsome((char*)out, max_size);
      for (size_t off = 0; off < kActualSize; off++) {
        if (out[off] == ':') {
          out[off] = 0x0D;
        }
      }
      return kActualSize;
    }
    Packet& Clear() {
      buff.str(std::string());
      body_buff.str(std::string());
      return *this;
    }
    std::stringstream buff;
    std::stringstream body_buff;
  };

  class StorageTest: public webapp::ProtocolHTTP::Request::Field::Storage {
    public:
      static const std::string& GetStaticName() {
        static const std::string kName("StorageTest");
        return kName;
      }

      StorageTest(): webapp::ProtocolHTTP::Request::Field::Storage(StorageTest::GetStaticName()) {
      }
      virtual ~StorageTest() {
      }
      virtual uint32_t Size() const {
        return _data.size();
      }
      virtual void Clear() {
        _data.clear();
      }
      virtual bool Append(const webapp::Protocol::Byte * data, uint32_t size) {
        _data.assign(reinterpret_cast<const char*>(data), size);
        return true;
      }
      virtual bool operator()(std::ostream *out) const {
        if (out == 0) {
          return false;
        }
        out->write(_data.c_str(), _data.size());
        return true;
      }
      virtual bool operator()(std::string *out) const {
        if (out == 0) {
          return false;
        }
        (*out) = _data;
        return true;
      }
    private:
      std::string _data;
  };

  ProtocolTestFixture() {}
  ~ProtocolTestFixture() {}
};

static webapp::ProtocolHTTP::Request::Field::Storage::Ptr StorageGenerator(
    const webapp::ProtocolHTTP::Content::Type &type) {
  if (type.name == webapp::ProtocolHTTP::Content::Type::kApplication) {
    return webapp::ProtocolHTTP::Request::Field::Storage::Ptr(new ProtocolTestFixture::StorageTest());
  }
  return webapp::ProtocolHTTP::Request::Field::Storage::Ptr();
}

static void CheckListOfStrings(const ListOfString &list,
                               const char         div,
                               const std::string  &templ) {
  ListOfString::const_iterator lang_it = list.begin();
  size_t off = 0;
  do {
    const size_t kPrevOff = templ.find_first_not_of(' ', (off == 0 ? 0 : off + 1));
    off = templ.find_first_of(div, kPrevOff);
    // DEBUG --------------------
//    std::cout << "DEBUG: " << __FUNCTION__ << ": " << __LINE__ << ": "
//              << "lang: |" << *lang_it << "| == |"
//                           << templ.substr(kPrevOff, off - kPrevOff) << "| "
//              << std::endl;
    // --------------------------
    BOOST_CHECK(templ.substr(kPrevOff, off - kPrevOff) == *lang_it);
    lang_it++;
  } while (off != std::string::npos);
}

static void CreateGetRequest(const std::string             &path,
                             webapp::ProtocolHTTP::Request *req) {
  webapp::ProtocolHTTP        proto(webapp::ProtocolHTTP::Router::Create());
  ProtocolTestFixture::Packet pkt;

  static const size_t kBuffSz = 1024;
  boost::scoped_array<webapp::Protocol::Byte> buff(
      new webapp::Protocol::Byte[kBuffSz]
  );
  proto.ParseRequest(buff.get(),
    pkt.AddStartLine("GET", path, "HTTP/1.1").Finalize()
       .ConvertToArray(buff.get(), kBuffSz),
    req
  );
}

static size_t GeneratePacket(webapp::ProtocolHTTP::Byte *out, size_t max_size) {
  ProtocolTestFixture::Packet pkt;
  pkt.AddStartLine("GET", "/hello.txt", "HTTP/1.1");
  pkt.AddField("User-Agent", kUserAgent);
  pkt.AddField("Host", kHost);
  pkt.AddField("Accept-Language", kAcceptLanguage);
  pkt.AddField("Accept-Charset",  kAcceptCharset);
  pkt.AddField("Accept-Encoding", kAcceptEncoding);
  pkt.AddField("Content-Type",     kContentType);
  pkt.AddField("Content-Encoding", kContentEncoding);
  pkt.AddField("Content-Language", kContentLanguage);
  pkt.AddField("Content-Location", kContentLocation);
  pkt.AddMultipartBody(kBoundary,
    "form-data; name=\"MessageTitle\"",
    "",
    kMessageTitle);
  pkt.AddMultipartBody(kBoundary,
    "form-data; name=\"DestAddress\"",
    "",
    kDestAddress);
  pkt.AddMultipartBody(kBoundary,
    "form-data; name=\"AttachedFile1\"; filename=\"horror-photo-1.jpg\"",
    "image/jpeg",
    kAttachedFile1);
  pkt.AddMultipartBody(kBoundary,
    "form-data; name=\"AttachedFile2\"; filename=\"test.txt\"",
    "text/plain",
    "Hello from \"test.txt\" file!");
  pkt.AddMultipartBody(kBoundary, "", "", "");
  pkt.Finalize();
  return pkt.ConvertToArray(out, max_size);
}

static void CheckRequestHeader(const webapp::ProtocolHTTP::Header &head) {
  // общие поля
  BOOST_CHECK(head.line.method  == webapp::ProtocolHTTP::kGet);
  BOOST_CHECK(head.line.target.get_path().at(0) == "hello.txt");
  BOOST_CHECK(head.line.version == "HTTP/1.1");
  BOOST_CHECK(head.user_agent == kUserAgent);
  BOOST_CHECK(head.host       == kHost);
  // группа Accept
  CheckListOfStrings(head.accept.language, ',', kAcceptLanguage);
  CheckListOfStrings(head.accept.charset,  ',', kAcceptCharset);
  CheckListOfStrings(head.accept.encoding, ',', kAcceptEncoding);
  // группа Content
  BOOST_CHECK(head.content.type.name == webapp::ProtocolHTTP::Content::Type::kMultipart);
  BOOST_CHECK(head.content.type.sub_type == "mixed");
  BOOST_CHECK(head.content.type.boundary == kBoundary);
  BOOST_CHECK(head.content.type.charset == "ISO-8859-4");
  BOOST_CHECK(head.content.location.get_path().at(0) == "hello.txt");
  BOOST_CHECK(head.content.length == 556);
  CheckListOfStrings(head.content.language, ',', kContentLanguage);
  CheckListOfStrings(head.content.encoding, ',', kContentEncoding);
}

static void CheckPostRequest(webapp::ProtocolHTTP::Request &req) {
  std::string field_str;
  const webapp::ProtocolHTTP::Request::Field *field = req.Post("MessageTitle");
  if (field != 0) {
    field->get_value(&field_str);
  }
  BOOST_CHECK(kMessageTitle == field_str);
  req.Post("DestAddress")->get_value(&field_str);
  BOOST_CHECK(kDestAddress == field_str);
  req.Post("AttachedFile1")->get_value(&field_str);
  BOOST_CHECK(kAttachedFile1 == field_str);
}
// -----------------------------------------------------------------------------
// Инициализация набора тестов
BOOST_FIXTURE_TEST_SUITE(ProtocolTestSuite, ProtocolTestFixture)

BOOST_AUTO_TEST_CASE(AbsoluteUriTest) {
  webapp::AbsoluteUri uri;
  BOOST_CHECK(uri.ParseVal("http:bla bla/trolo-lo-lo"));
  BOOST_CHECK(uri.get_scheme() == "http");
  BOOST_CHECK(uri.ParseVal("HTTp:/azazaza "));
  BOOST_CHECK(uri.get_scheme() == "http");
  BOOST_CHECK(not uri.ParseVal("http=//example.com"));
  BOOST_CHECK(uri.get_scheme() == "");
  BOOST_CHECK(not uri.ParseVal("http=://example.com"));
  BOOST_CHECK(uri.get_scheme() == "");
}

BOOST_AUTO_TEST_CASE(HttpUriAuthorityTest) {
  webapp::ProtocolHTTP::Uri uri;
  BOOST_CHECK(not uri.ParseVal("http:bla bla/trolo-lo-lo"));
  BOOST_CHECK(uri.get_scheme() == "http");

  BOOST_CHECK(not uri.ParseVal("http#fail://example.com"));
  BOOST_CHECK(uri.get_scheme()         == "");
  BOOST_CHECK(uri.get_authority().host == "");

  BOOST_CHECK(uri.ParseVal("smtp://user@google.com"));
  BOOST_CHECK(uri.get_scheme()             == "smtp");
  BOOST_CHECK(uri.get_authority().userinfo == "user");
  BOOST_CHECK(uri.get_authority().host     == "google.com");

  BOOST_CHECK(uri.ParseVal("https://user11_55@google.com?bla-bla"));
  BOOST_CHECK(uri.get_scheme()             == "https");
  BOOST_CHECK(uri.get_authority().userinfo == "user11_55");
  BOOST_CHECK(uri.get_authority().host     == "google.com");

  BOOST_CHECK(uri.ParseVal("ftp://user&+$buzzy@google.com/zigota"));
  BOOST_CHECK(uri.get_scheme()             == "ftp");
  BOOST_CHECK(uri.get_authority().userinfo == "user&+$buzzy");
  BOOST_CHECK(uri.get_authority().host     == "google.com");
}

BOOST_AUTO_TEST_CASE(HttpUriPathTest) {
  webapp::ProtocolHTTP::Uri uri;
  BOOST_CHECK(uri.ParseVal("http://google.com/"));
  BOOST_CHECK(uri.get_path().size() == 0);

  BOOST_CHECK(not uri.ParseVal("http://google.com/node0 /node1/node2"));
  BOOST_CHECK(uri.get_path().size() == 0);

  BOOST_CHECK(uri.ParseVal("http://google.com/node0/node1/node2"));
  BOOST_CHECK(uri.get_path().size() == 3);
  BOOST_CHECK(uri.get_path().at(1) == "node1");

  BOOST_CHECK(uri.ParseVal("http://google.com/node0/node1?"));
  BOOST_CHECK(uri.get_path().size() == 2);
  BOOST_CHECK(uri.get_path().at(1) == "node1");

  BOOST_CHECK(uri.ParseVal("http://google.com/node0#"));
  BOOST_CHECK(uri.get_path().size() == 1);
  BOOST_CHECK(uri.get_path().at(0) == "node0");

  BOOST_CHECK(uri.ParseVal("http://google.com/%D0%BF%D1%80%D0%BE%D0%B2%2F%D0%B5%D1%80%D0%BA%D0%B0/node"));
  BOOST_CHECK(uri.get_path().size() == 2);
  BOOST_CHECK(uri.get_path().at(0) == "пров/ерка");
}

BOOST_AUTO_TEST_CASE(HttpUriQueryTest) {
  webapp::ProtocolHTTP::Uri uri;
  BOOST_CHECK(uri.ParseVal("http://google.com/node0?var_0=val_0&var_1=val_1"));
  BOOST_CHECK(uri.get_query().size() == 2);
  BOOST_CHECK(uri.get_query().find("var_0")->second == "val_0");
  BOOST_CHECK(uri.get_query().find("var_1")->second == "val_1");

  BOOST_CHECK(not uri.ParseVal("http://google.com/node0?var_0=val_0&var_1= val_1"));
  BOOST_CHECK(uri.get_query().size() == 1);
  BOOST_CHECK(uri.get_query().find("var_0")->second == "val_0");

  BOOST_CHECK(uri.ParseVal("http://google.com/node0?%D1%82%D0%B5%D1%81%D1%82%3D%D1%80%D0%B0%D0%B1%D0%BE%D1%82%D1%8B=%D0%BF%D1%80%D0%BE%D0%B9%D0%B4%D0%B5%D0%BD"));
  BOOST_CHECK(uri.get_query().size() == 1);
  BOOST_CHECK(uri.get_query().find("тест=работы")->second == "пройден");

  BOOST_CHECK(uri.ParseVal("http://google.com/node0?var_0=&=val_1"));
  BOOST_CHECK(uri.get_query().size() == 2);
  BOOST_CHECK(uri.get_query().find("var_0")->second == "");
  BOOST_CHECK(uri.get_query().find("")->second == "val_1");

  const std::string kUrl("http://192.168.7.223/action/update/firmware?from=");
  const std::string kFromVal("ftp://jenny/firmwares/F1772/cortex_a8.regigraf.1772.53.UNIVERSAL-last.rbf");
  BOOST_CHECK(uri.ParseVal(kUrl + kFromVal));
  BOOST_CHECK(uri.get_query().size() == 1);
  BOOST_CHECK(uri.get_query().find("from")->second == kFromVal);
}

BOOST_AUTO_TEST_CASE(HttpUriFragmentTest) {
  const std::string kFragment_0("some_fragment...!!!0-9");
  const std::string kFragment_1("some_fragment lol");
  const std::string kFragment_2("%D1%84%D1%80%D0%B0%D0%B3%23%D0%BC%D0%B5%D0%BD%D1%82");
  webapp::ProtocolHTTP::Uri uri;
  BOOST_CHECK(uri.ParseVal("http://google.com/node0#" + kFragment_0));
  BOOST_CHECK(uri.get_fragment() == kFragment_0);
  BOOST_CHECK(not uri.ParseVal("http://google.com/node0#" + kFragment_1));
  BOOST_CHECK(uri.get_fragment() == "");
  BOOST_CHECK(uri.ParseVal("http://google.com/node0#" + kFragment_2));
  BOOST_CHECK(uri.get_fragment() == "фраг#мент");
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPDecodeTest) {
  typedef std::map<std::string, std::string>  Tasks;
  typedef std::pair<std::string, std::string> Task;
  Tasks tasks;
  tasks.insert(Task("http://ABC.com/%7Esmith/home.html", "http://ABC.com/~smith/home.html"));
  tasks.insert(Task("http://ABC.com/%7esmith/home.html", "http://ABC.com/~smith/home.html"));
  tasks.insert(Task("https://translate.google.ru/?hl=ru&tab=mT#ru/en/%D0%BF%D1%80%D0%B8%D0%B2%D0%B5%D1%82",
                    "https://translate.google.ru/?hl=ru&tab=mT#ru/en/привет"));
  Tasks::iterator t_it = tasks.begin();
  std::string decoded_str;
  for (; t_it != tasks.end(); t_it++) {
    decoded_str.clear();
    webapp::ProtocolHTTP::DecodeString(t_it->first, '%', &decoded_str);
    BOOST_CHECK(decoded_str == t_it->second);
  }
}

BOOST_AUTO_TEST_CASE(ProtocolStorageInMemAppendTest) {
  webapp::ProtocolHTTP::Request::Field::StorageInMem mem;
  const std::string kStr0("hello world");
  const std::string kStr1("Мир, дружба, жвачка");
  const std::string kStr0and1(kStr0 + kStr1);
  BOOST_CHECK(mem.Append((const unsigned char*)kStr0.c_str(), kStr0.size()));
  std::string out_str;
  std::stringstream out_stream;
  BOOST_CHECK(mem(&out_str));
  BOOST_CHECK(mem(&out_stream));
  BOOST_CHECK(kStr0 == out_str);
  BOOST_CHECK(kStr0 == out_stream.str());

  out_str.clear();
  out_stream.str(out_str);
  BOOST_CHECK(mem.Append((const unsigned char*)kStr1.c_str(), kStr1.size()));
  BOOST_CHECK(mem(&out_str));
  BOOST_CHECK(mem(&out_stream));
  BOOST_CHECK(kStr0and1.size() == mem.Size());
  BOOST_CHECK(kStr0and1 == out_str);
  BOOST_CHECK(kStr0and1 == out_stream.str());
}

BOOST_AUTO_TEST_CASE(ProtocolStorageInMemClearTest) {
  webapp::ProtocolHTTP::Request::Field::StorageInMem mem;
  const std::string kStr0("Tro-lo-lo-lo-lo lo-lo-lo lo-lo-loooo aha-ha-ha-ha");
  BOOST_CHECK(mem.Append((const unsigned char*)kStr0.c_str(), kStr0.size()));
  std::string out_str;
  BOOST_CHECK(mem(&out_str));
  BOOST_CHECK(kStr0 == out_str);

  mem.Clear();
  BOOST_CHECK(mem(&out_str));
  BOOST_CHECK(out_str.size() == 0);

  out_str.clear();
  BOOST_CHECK(mem(&out_str));
  BOOST_CHECK(kStr0 != out_str);
  BOOST_CHECK(out_str.size() == 0);
}

BOOST_AUTO_TEST_CASE(ProtocolStorageInMemCopyingTest) {
  webapp::ProtocolHTTP::Request::Field::StorageInMem mem;
  const std::string kStr0("Tro-lo-lo-lo-lo lo-lo-lo lo-lo-loooo aha-ha-ha-ha");
  BOOST_CHECK(mem.Append((const unsigned char*)kStr0.c_str(), kStr0.size()));
  std::string out_str_0;
  BOOST_CHECK(mem(&out_str_0));
  BOOST_CHECK(kStr0 == out_str_0);
  // copy constructor
  {
    std::string out_str;
    webapp::ProtocolHTTP::Request::Field::StorageInMem tmp_mem(mem);
    BOOST_CHECK(tmp_mem(&out_str));
    BOOST_CHECK(kStr0 == out_str);
  }
  // copy operator
  {
    std::string out_str;
    webapp::ProtocolHTTP::Request::Field::StorageInMem tmp_mem;
    tmp_mem = mem;
    BOOST_CHECK(tmp_mem(&out_str));
    BOOST_CHECK(kStr0 == out_str);
  }

  std::string out_str_1;
  BOOST_CHECK(mem(&out_str_1));
  BOOST_CHECK(kStr0 == out_str_1);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPRequestFieldGeneratorTest) {
  const size_t kBuffMaxSize = 1024;
  webapp::ProtocolHTTP::ArrayOfBytes buff(new webapp::ProtocolHTTP::Byte[kBuffMaxSize]);
  webapp::ProtocolHTTP::Request      req;
  webapp::ProtocolHTTP               proto(webapp::ProtocolHTTP::Router::Create());
  const size_t kBuffActSize = GeneratePacket(buff.get(), kBuffMaxSize);
  req.UseStorageGenerator(StorageGenerator);
  BOOST_CHECK(proto.ParseRequest(buff.get(), kBuffActSize, &req));
  BOOST_CHECK(req.Completed());
  const webapp::ProtocolHTTP::Request::Field *field = req.Post("MessageTitle");
  BOOST_CHECK(field != 0);

  BOOST_CHECK(field->GetNameOfStorage() == StorageTest::GetStaticName());

  std::string field_str;
  field->get_value(&field_str);
  BOOST_CHECK(kMessageTitle == field_str);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPRequestFieldsTest) {
  const size_t kBuffMaxSize = 1024;
  webapp::ProtocolHTTP::ArrayOfBytes buff(new webapp::ProtocolHTTP::Byte[kBuffMaxSize]);
  webapp::ProtocolHTTP::Request      req;
  webapp::ProtocolHTTP               proto(webapp::ProtocolHTTP::Router::Create());
  const size_t kBuffActSize = GeneratePacket(buff.get(), kBuffMaxSize);

  BOOST_CHECK(proto.ParseRequest(buff.get(), kBuffActSize, &req));
  BOOST_CHECK(req.Completed());
  CheckRequestHeader(req.GetHeader());
  CheckPostRequest(req);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPRequestFieldCopyingTest) {
  const size_t kBuffMaxSize = 1024;
  webapp::ProtocolHTTP::ArrayOfBytes buff(new webapp::ProtocolHTTP::Byte[kBuffMaxSize]);
  webapp::ProtocolHTTP::Request      req;
  webapp::ProtocolHTTP               proto(webapp::ProtocolHTTP::Router::Create());
  const size_t kBuffActSize = GeneratePacket(buff.get(), kBuffMaxSize);

  BOOST_CHECK(proto.ParseRequest(buff.get(), kBuffActSize, &req));
  BOOST_CHECK(req.Completed());

  const webapp::ProtocolHTTP::Request::Field *field = req.Post("MessageTitle");
  std::string field_str_0;
  field->get_value(&field_str_0);
  BOOST_CHECK(field_str_0 == kMessageTitle);
  // copy constructor
  {
    const webapp::ProtocolHTTP::Request::Field t_field(*field);
    std::string field_str;
    t_field.get_value(&field_str);
    BOOST_CHECK(field_str == kMessageTitle);
  }
  // copy operator
  {
    webapp::ProtocolHTTP::Request::Field t_field = *field;
    std::string field_str;
    t_field.get_value(&field_str);
    BOOST_CHECK(field_str == kMessageTitle);
  }
  std::string field_str_1;
  field->get_value(&field_str_1);
  BOOST_CHECK(field_str_1 == kMessageTitle);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPPartialRequestTest) {
  const size_t kBuffMaxSize = 1024;
  webapp::ProtocolHTTP::ArrayOfBytes buff(new webapp::ProtocolHTTP::Byte[kBuffMaxSize]);
  webapp::ProtocolHTTP::Request      req;
  webapp::ProtocolHTTP               proto(webapp::ProtocolHTTP::Router::Create());
  const size_t kBuffActSize = GeneratePacket(buff.get(), kBuffMaxSize);
  const size_t kPartsAmount = 10;
  const size_t kStep = kBuffActSize / kPartsAmount;
  for (size_t part = 0; part < kBuffActSize; part += kStep) {
    const size_t kPartSize = (kBuffActSize - part) > kStep ? kStep : (kBuffActSize - part);
    BOOST_CHECK(proto.ParseRequest(&buff[part], kPartSize, &req));
    if (kPartSize == kStep) {
      BOOST_CHECK(not req.Completed());
    } else {
      BOOST_CHECK(req.Completed());
    }
  }
  CheckRequestHeader(req.GetHeader());
  CheckPostRequest(req);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPGetRequestTest) {
  const size_t kBuffMaxSize = 1024;
  webapp::ProtocolHTTP::ArrayOfBytes buff(new webapp::ProtocolHTTP::Byte[kBuffMaxSize]);
  ProtocolTestFixture::Packet pkt;
  pkt.AddStartLine("GET",
      "/hello.txt?var_0=val_0&var_1=val_1&%D0%BC%D0%B8%D1%80=%D1%80%D0%B8%D0%BC",
      "HTTP/1.1");
  pkt.AddField("User-Agent", kUserAgent);
  pkt.AddField("Host", kHost);
  pkt.Finalize();

  webapp::ProtocolHTTP::Request req;
  webapp::ProtocolHTTP          proto(webapp::ProtocolHTTP::Router::Create());
  const size_t kActSize = pkt.ConvertToArray(buff.get(), kBuffMaxSize);
  BOOST_CHECK(proto.ParseRequest(buff.get(), kActSize, &req));

  BOOST_CHECK(req.Get("var_0") == "val_0");
  BOOST_CHECK(req.Get("var_1") == "val_1");
  BOOST_CHECK(req.Get("мир")   == "рим");
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPGetRequestTypeCastTest) {
  const size_t kBuffMaxSize = 1024;
  webapp::ProtocolHTTP::ArrayOfBytes buff(new webapp::ProtocolHTTP::Byte[kBuffMaxSize]);
  ProtocolTestFixture::Packet pkt;
  pkt.AddStartLine("GET",
      "/hello.txt?var_0=777&var_1=666",
      "HTTP/1.1");
  pkt.AddField("User-Agent", kUserAgent);
  pkt.AddField("Host", kHost);
  pkt.Finalize();

  webapp::ProtocolHTTP::Request req;
  webapp::ProtocolHTTP          proto(webapp::ProtocolHTTP::Router::Create());
  const size_t kActSize = pkt.ConvertToArray(buff.get(), kBuffMaxSize);
  BOOST_CHECK(proto.ParseRequest(buff.get(), kActSize, &req));

  BOOST_CHECK(req.Get("var_0", (int)0) == 777);
  BOOST_CHECK(req.Get("var_1", (int)0) == 666);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPCopyingOfRequestTest) {
  const size_t kBuffMaxSize = 1024;
  webapp::ProtocolHTTP::ArrayOfBytes buff(new webapp::ProtocolHTTP::Byte[kBuffMaxSize]);
  const size_t kBuffActSize = GeneratePacket(buff.get(), kBuffMaxSize);
  webapp::ProtocolHTTP::Request req;
  webapp::ProtocolHTTP          proto(webapp::ProtocolHTTP::Router::Create());
  BOOST_CHECK(proto.ParseRequest(buff.get(), kBuffActSize, &req));
  CheckRequestHeader(req.GetHeader());
  // copy constructor
  {
    webapp::ProtocolHTTP::Request tmp_req(req);
    CheckRequestHeader(tmp_req.GetHeader());
  }
  // copy operator
  {
    webapp::ProtocolHTTP::Request tmp_req;
    tmp_req = req;
    CheckRequestHeader(tmp_req.GetHeader());
  }
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPExpireTest) {
  webapp::ProtocolHTTP::Expires exp("2016-08-09 18:00:00");
  BOOST_CHECK(exp.ToString() == "Tue, 09 Aug 2016 18:00:00 UTC");
  exp.Hour();
  BOOST_CHECK(exp.ToString() == "Tue, 09 Aug 2016 19:00:00 UTC");
  exp.Day();
  BOOST_CHECK(exp.ToString() == "Wed, 10 Aug 2016 19:00:00 UTC");
  exp.Week();
  BOOST_CHECK(exp.ToString() == "Wed, 17 Aug 2016 19:00:00 UTC");
  exp.Hour().Day();
  BOOST_CHECK(exp.ToString() == "Thu, 18 Aug 2016 20:00:00 UTC");
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPCacheControlTest) {
  webapp::ProtocolHTTP::CacheControl ctl;
  BOOST_CHECK(ctl.get_directive() == "");
  ctl.NoStore();
  BOOST_CHECK(ctl.get_directive() == "no-store");
  ctl.Reset().MaxAge(5);
  BOOST_CHECK(ctl.get_directive() == "max-age=5");
  ctl.Reset().NoCache().NoStore();
  BOOST_CHECK(ctl.get_directive() == "no-cache, no-store");
  ctl.Reset().MustRevalidate();
  BOOST_CHECK(ctl.get_directive() == "must-revalidate");
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPETagTest) {
  const std::string kETagVal("testing value");
  webapp::ProtocolHTTP::ETag etag;
  BOOST_CHECK(etag.get_directive() == "");
  etag.Predefined(kETagVal);
  BOOST_CHECK(etag.get_directive() == kETagVal);
  etag.Random();
  BOOST_CHECK(etag.get_directive() != kETagVal);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPResponseGetTextHeaderTest) {
  // заголовок по умолчанию
  const webapp::ProtocolHTTP::Header kDefHeader(
      webapp::ProtocolHTTP::Response::GetHeaderForText("", "")
  );
  const webapp::ProtocolHTTP::Content       kDefCont(kDefHeader.content);
  const webapp::ProtocolHTTP::Content::Type kDefType(kDefCont.type);
  BOOST_CHECK(kDefType.name     == webapp::ProtocolHTTP::Content::Type::kText);
  BOOST_CHECK(kDefType.sub_type == "html");
  BOOST_CHECK(kDefType.charset  == "utf-8");
  BOOST_CHECK(kDefHeader.expires.ToString() ==
      webapp::ProtocolHTTP::Expires().Now().Hour().ToString()
  );
  // заголовок с пользовательскими настройками
  static const std::string kSType("plain");
  static const std::string kCharset("cp-1251");
  const webapp::ProtocolHTTP::Header kHeader(
      webapp::ProtocolHTTP::Response::GetHeaderForText(kSType, kCharset)
  );
  const webapp::ProtocolHTTP::Content       kCont(kHeader.content);
  const webapp::ProtocolHTTP::Content::Type kType(kCont.type);
  BOOST_CHECK(kType.name     == webapp::ProtocolHTTP::Content::Type::kText);
  BOOST_CHECK(kType.sub_type == kSType);
  BOOST_CHECK(kType.charset  == kCharset);
  BOOST_CHECK(kHeader.expires.ToString() ==
      webapp::ProtocolHTTP::Expires().Now().Hour().ToString()
  );
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPResponseGetDefaultImageHeaderTest) {
  // заголовок по умолчанию
  const webapp::ProtocolHTTP::Header kDefHeader(
      webapp::ProtocolHTTP::Response::GetHeaderForImage("")
  );
  const webapp::ProtocolHTTP::Content       kDefCont(kDefHeader.content);
  const webapp::ProtocolHTTP::Content::Type kDefType(kDefCont.type);
  BOOST_CHECK(kDefType.name     == webapp::ProtocolHTTP::Content::Type::kApplication);
  BOOST_CHECK(kDefType.sub_type == "octet-stream");
  BOOST_CHECK(kDefHeader.expires.ToString() ==
      webapp::ProtocolHTTP::Expires().Now().Hour().ToString()
  );
  // заголовок с пользовательскими настройками
  static const std::string kSType("bmp");
  const webapp::ProtocolHTTP::Header kHeader(
      webapp::ProtocolHTTP::Response::GetHeaderForImage(kSType)
  );
  const webapp::ProtocolHTTP::Content       kCont(kHeader.content);
  const webapp::ProtocolHTTP::Content::Type kType(kCont.type);
  BOOST_CHECK(kType.name     == webapp::ProtocolHTTP::Content::Type::kImage);
  BOOST_CHECK(kType.sub_type == kSType);
  BOOST_CHECK(kHeader.expires.ToString() ==
      webapp::ProtocolHTTP::Expires().Now().Hour().ToString()
  );
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPResponseGetDefaultJSONHeaderTest) {
  const webapp::ProtocolHTTP::Header kDefHeader(
      webapp::ProtocolHTTP::Response::GetHeaderForJSON()
  );
  const webapp::ProtocolHTTP::Content       kDefCont(kDefHeader.content);
  const webapp::ProtocolHTTP::Content::Type kDefType(kDefCont.type);
  BOOST_CHECK(kDefType.name     == webapp::ProtocolHTTP::Content::Type::kApplication);
  BOOST_CHECK(kDefType.sub_type == "json");
  BOOST_CHECK(kDefHeader.expires.ToString() ==
      webapp::ProtocolHTTP::Expires().Now().ToString()
  );
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPResponseSourceFromArrayTest) {
  const std::string kSrcStr("hello from array resource");
  webapp::ProtocolHTTP::Response::SourceFromArray src_0(
    reinterpret_cast<const webapp::Protocol::Byte*>(kSrcStr.c_str()),
    kSrcStr.size()
  );
  BOOST_CHECK(src_0.IsAvailable());
  BOOST_CHECK(src_0.Size() == kSrcStr.size());

  const size_t kDataSz = 1024;
  boost::scoped_array<webapp::Protocol::Byte> data(new webapp::Protocol::Byte[kDataSz]);
  BOOST_CHECK(src_0.ReadSome(data.get(), kDataSz) == kSrcStr.size());
  BOOST_CHECK(not src_0.IsAvailable());

  const std::string kStrRes(reinterpret_cast<char*>(data.get()), src_0.Size());
  BOOST_CHECK(kStrRes == kSrcStr);

  BOOST_CHECK(src_0.ReadSome(data.get(), kDataSz) == 0);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPResponseSourceFromFileTest) {
  webapp::ProtocolHTTP::Response::SourceFromFile f_src_0("/etc/hosts");
  webapp::ProtocolHTTP::Response::SourceFromFile f_src_1("fail.txt");
  BOOST_CHECK(f_src_0.IsAvailable());
  BOOST_CHECK(f_src_0.Size() > 0);

  const size_t kDataSz = f_src_0.Size();
  boost::scoped_array<webapp::Protocol::Byte> data(new webapp::Protocol::Byte[kDataSz]);
  BOOST_CHECK(f_src_0.ReadSome(data.get(), kDataSz) == f_src_0.Size());
  BOOST_CHECK(not f_src_0.IsAvailable());

  BOOST_CHECK(not f_src_1.IsAvailable());
  BOOST_CHECK(f_src_1.Size() == 0);
  BOOST_CHECK(f_src_1.ReadSome(data.get(), kDataSz) == 0);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPResponseSourceFromStreamTest) {
  const std::string kStr1("Hello world");
  const std::string kStr2("Born to kill");
  std::stringstream s_str_0;
  std::stringstream s_str_1;

  s_str_1 << kStr1;

  webapp::ProtocolHTTP::Response::SourceFromStream s_src_0(&s_str_0);
  webapp::ProtocolHTTP::Response::SourceFromStream s_src_1(&s_str_1);
  // проверка данных из пустого потока
  BOOST_CHECK(not s_src_0.IsAvailable());
  BOOST_CHECK(s_src_0.Size() == 0);
  // проверка данных из непустого потока
  BOOST_CHECK(s_src_1.IsAvailable());
  BOOST_CHECK(s_src_1.Size() == kStr1.size());

  const size_t kDataSz = 1024;
  boost::scoped_array<webapp::Protocol::Byte> data(new webapp::Protocol::Byte[kDataSz]);
  BOOST_CHECK(s_src_1.ReadSome(data.get(), kDataSz) == s_src_1.Size());
  BOOST_CHECK(not s_src_1.IsAvailable());
  const std::string kStrRes(reinterpret_cast<char*>(data.get()), s_src_1.Size());
  BOOST_CHECK(kStrRes == kStr1);
  // проверка данных полученных из строки
  webapp::ProtocolHTTP::Response::SourceFromStream s_src_2(kStr2);
  BOOST_CHECK(s_src_2.IsAvailable());
  BOOST_CHECK(s_src_2.Size() == kStr2.size());
  BOOST_CHECK(s_src_2.ReadSome(data.get(), kDataSz) == s_src_2.Size());
  BOOST_CHECK(not s_src_2.IsAvailable());
  const std::string kStrRes2(reinterpret_cast<char*>(data.get()), s_src_2.Size());
  BOOST_CHECK(kStrRes2 == kStr2);
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPGetResponseTest) {
  webapp::ProtocolHTTP::Response resp;
  webapp::ProtocolHTTP           proto(webapp::ProtocolHTTP::Router::Create());
  static const size_t kBuffSz = 1024;
  boost::scoped_array<webapp::Protocol::Byte> buff(new webapp::Protocol::Byte[kBuffSz]);
  const std::string kJsonBody("{'key0': 'val0', 'key1': 'val1'}");
  const webapp::ProtocolHTTP::Header kHttpHeader = webapp::ProtocolHTTP::Response::GetHeaderForJSON();
  resp.SetHeader(webapp::ProtocolHTTP::k200, kHttpHeader);
  resp.SetBody(kJsonBody);
  std::string resp_text;
  // получение данных для отправки в качестве ответа
  do {
    const webapp::USize kRespSz = proto.GetResponse(buff.get(), kBuffSz, &resp);
    if (kRespSz == 0) {
      break;
    }
    const std::string kRespStr(reinterpret_cast<char*>(buff.get()), kRespSz);
    resp_text += kRespStr;
  } while (1);
  // проверка полученных данных
  typedef std::list<std::string> ListOfStrings;
  ListOfStrings resp_rows;
  resp_rows.push_back("HTTP/1.1 200 OK");
  resp_rows.push_back("Content-Type: application/json");
  resp_rows.push_back("Content-Length: 32");
  resp_rows.push_back("Expires: " + kHttpHeader.expires.ToString());
  resp_rows.push_back("{'key0': 'val0', 'key1': 'val1'}");
  ListOfStrings::const_iterator row_it = resp_rows.begin();
  size_t prev_off = 0;
  size_t off      = 0;
  while (off != std::string::npos) {
    prev_off = off + (off > 0 ? 2 : 0);
    off      = resp_text.find("\r\n", prev_off);
    const size_t kPartLen = off - prev_off;
    if (kPartLen > 2) {
      const std::string kRespPart = resp_text.substr(prev_off, kPartLen);
      BOOST_CHECK(kRespPart == *row_it);
      row_it++;
    }
  }
}

static
bool EmptyRouterHandler(const webapp::ProtocolHTTP::Uri::Path &,
                        const webapp::ProtocolHTTP::Request   &,
                              webapp::ProtocolHTTP::Response  *) {
  return true;
}

static
bool FailRouterHandler(const webapp::ProtocolHTTP::Uri::Path &,
                       const webapp::ProtocolHTTP::Request   &,
                             webapp::ProtocolHTTP::Response  *) {
  return false;
}

static bool router_handler_0_active   = false;
static bool router_handler_0_1_active = false;

static
bool RouterHandler_0(const webapp::ProtocolHTTP::Uri::Path &,
                     const webapp::ProtocolHTTP::Request   &,
                           webapp::ProtocolHTTP::Response  *) {
  router_handler_0_active = true;
  return true;
}

static
bool RouterHandler_0_1(const webapp::ProtocolHTTP::Uri::Path &,
                       const webapp::ProtocolHTTP::Request   &,
                             webapp::ProtocolHTTP::Response  *) {
  router_handler_0_1_active = true;
  return true;
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPRouterAddHandlerTest) {
  webapp::ProtocolHTTP::Router rt;
  BOOST_CHECK(rt.AddHandlerFor("/node0/node1/node2.0",     EmptyRouterHandler));
  BOOST_CHECK(rt.AddHandlerFor("node0/node1/node2.1",      EmptyRouterHandler));
  BOOST_CHECK(not rt.AddHandlerFor("/node0/node1/node2.0", EmptyRouterHandler));
  BOOST_CHECK(not rt.AddHandlerFor("node0/node1/node2.1",  EmptyRouterHandler));

  BOOST_CHECK(rt.AddHandlerFor("node0/node1",     EmptyRouterHandler));
  BOOST_CHECK(not rt.AddHandlerFor("node0/node1", EmptyRouterHandler));
  BOOST_CHECK(rt.AddHandlerFor("/node0",          EmptyRouterHandler));
  BOOST_CHECK(not rt.AddHandlerFor("/node0",      EmptyRouterHandler));
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPRouterCallHandlerTest) {
  typedef boost::scoped_ptr<webapp::ProtocolHTTP::Request> ReqPtr;
  webapp::ProtocolHTTP::Router   rt;
  ReqPtr                         req;
  webapp::ProtocolHTTP::Response resp;
  BOOST_CHECK(rt.AddHandlerFor("/node0",            RouterHandler_0));
  BOOST_CHECK(rt.AddHandlerFor("/node0/node1",      RouterHandler_0_1));
  BOOST_CHECK(rt.AddHandlerFor("/node0/null",       0));
  BOOST_CHECK(rt.AddHandlerFor("/node0/node1/fail", FailRouterHandler));
  router_handler_0_active   = false;
  router_handler_0_1_active = false;

  req.reset(new webapp::ProtocolHTTP::Request());
  CreateGetRequest("/node0", req.get());
  BOOST_CHECK(rt.CallHandlerFor(*req, &resp));
  BOOST_CHECK(router_handler_0_active);

  req.reset(new webapp::ProtocolHTTP::Request());
  CreateGetRequest("/node0/node1", req.get());
  BOOST_CHECK(rt.CallHandlerFor(*req, &resp));
  BOOST_CHECK(router_handler_0_1_active);

  req.reset(new webapp::ProtocolHTTP::Request());
  CreateGetRequest("/node0/null", req.get());
  BOOST_CHECK(not rt.CallHandlerFor(*req, &resp));

  req.reset(new webapp::ProtocolHTTP::Request());
  CreateGetRequest("/node0/node1/fail", req.get());
  BOOST_CHECK(not rt.CallHandlerFor(*req, &resp));
}

BOOST_AUTO_TEST_CASE(ProtocolHTTPRouterCallHandlerForUrlTest) {
  typedef boost::scoped_ptr<webapp::ProtocolHTTP::Request> ReqPtr;
  webapp::ProtocolHTTP::Router   rt;
  ReqPtr                         req;
  webapp::ProtocolHTTP::Response resp;
  BOOST_CHECK(rt.AddHandlerFor("/action/update/firmware", RouterHandler_0));
  router_handler_0_active = false;

  const std::string kUrl("http://192.168.7.223/action/update/firmware?from=");
  const std::string kFromVal("ftp://jenny/firmwares/F1772/cortex_a8.regigraf.1772.53.UNIVERSAL-last.rbf");

  req.reset(new webapp::ProtocolHTTP::Request());
  CreateGetRequest(kUrl + kFromVal, req.get());
  BOOST_CHECK(rt.CallHandlerFor(*req, &resp));
  BOOST_CHECK(router_handler_0_active);
}

BOOST_AUTO_TEST_SUITE_END()
