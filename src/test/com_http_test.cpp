/*
 * com_http_test.cpp
 *
 *  Created on: 9 сент. 2016 г.
 *      Author: denis
 */
#define empty() __empty()
#include <iostream>
#include <sstream>
#include <boost/test/unit_test.hpp>
#include <boost/regex.hpp>
#include "webapp_com_http.hpp"

typedef webapp::Com          Com;
typedef webapp::ProtocolHTTP Proto;
namespace http = webapp::http;

static const unsigned char kSp     = 0x20;
static const unsigned char kCrLf[] = "\r\n";//{0x0D, 0x0A};
static const unsigned char kOWS    = kSp;

struct ComHttpTestFixture {
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
    Packet& Clear() {
      buff.str(std::string());
      body_buff.str(std::string());
      return *this;
    }
    std::stringstream buff;
    std::stringstream body_buff;
  };
};

static const std::string kBoundary("gc0pJq0M:08jU534c0p");
static const std::string kContentType("multipart/mixed; boundary=\"" + kBoundary
                                      + "\";charset=ISO-8859-4");
static const std::string kGetString("Lees_texto_de_programa_y_reflexionas_en_esto.");
static const std::string kPostString("Hasta_la_vista!_Comprendas?");

static const Com::Real    kGetReal    = 340.29;
static const Com::Integer kGetInteger = 299792458;
static const Com::Boolean kGetBoolean = true;

static size_t GeneratePacket(webapp::ProtocolHTTP::Byte *out, size_t max_size) {
  ComHttpTestFixture::Packet pkt;

  std::stringstream get_args;
  get_args << "getString=" << kGetString << "&"
           << "getReal=" << std::fixed << kGetReal << "&"
           << "getInteger=" << (unsigned long)kGetInteger << "&"
           << "getBoolean=" << (kGetBoolean ? "true" : "false");

  pkt.AddStartLine("GET", "/?" + get_args.str(), "HTTP/1.1");
  pkt.AddField("Content-Type", kContentType);
  pkt.AddMultipartBody(kBoundary,
    "form-data; name=\"postString\"",
    "",
    kPostString);
  pkt.AddMultipartBody(kBoundary, "", "", "");
  pkt.Finalize();
  return pkt.ConvertToArray(out, max_size);
}

static void GenerateRequest(Proto::Request &req) {
  const size_t kBuffMaxSize = 1024;
  Proto::ArrayOfBytes buff(new Proto::Byte[kBuffMaxSize]);
  Proto               proto(Proto::Router::Create());
  const size_t kBuffActSize = GeneratePacket(buff.get(), kBuffMaxSize);
  BOOST_CHECK(proto.ParseRequest(buff.get(), kBuffActSize, &req));
  BOOST_CHECK(req.Completed());
}

static void GetResponse(Proto::Response &resp, std::string &resp_text) {
  static const size_t kBuffSz = 1024;
  Proto proto(Proto::Router::Create());
  boost::scoped_array<Proto::Byte> buff(new Proto::Byte[kBuffSz]);
  resp_text.clear();
  // получение данных для отправки в качестве ответа
  do {
    const webapp::USize kRespSz = proto.GetResponse(buff.get(), kBuffSz, &resp);
    if (kRespSz == 0) {
      break;
    }
    const std::string kRespStr(reinterpret_cast<char*>(buff.get()), kRespSz);
    resp_text += kRespStr;
  } while (1);
}
// -----------------------------------------------------------------------------
// Инициализация набора тестов
BOOST_FIXTURE_TEST_SUITE(ComHttpTestSuite, ComHttpTestFixture)

BOOST_AUTO_TEST_CASE(TestComHttpInputHasCell) {
  Proto::Request req;
  GenerateRequest(req);
  http::Input in(&req);

  BOOST_CHECK(in.HasCell("getString"));
  BOOST_CHECK(in.HasCell("getReal"));
  BOOST_CHECK(in.HasCell("getInteger"));
  BOOST_CHECK(in.HasCell("getBoolean"));
  BOOST_CHECK(in.HasCell("postString"));
  BOOST_CHECK(not in.HasCell("get"));
}

BOOST_AUTO_TEST_CASE(TestComHttpInputGet) {
  Proto::Request req;
  GenerateRequest(req);
  http::Input in(&req);

  BOOST_CHECK(in.GetString("getString")   == kGetString);
  BOOST_CHECK(in.GetReal("getReal")       == kGetReal);
  BOOST_CHECK(in.GetInteger("getInteger") == kGetInteger);
  BOOST_CHECK(in.GetBoolean("getBoolean") == kGetBoolean);
  BOOST_CHECK(in.GetString("postString")  == kPostString);
}

BOOST_AUTO_TEST_CASE(TestComHttpOutputPut) {
  const std::string kQString("\"There is no substitute for hard work.\""
                             "\"What you are will show in what you do.\"");
  Proto::Response resp;
  http::Output out;

  out.Put("real",    kGetReal);
  out.Put("integer", kGetInteger);
  out.Put("boolean", kGetBoolean);
  out.Put("string",  kQString);
  BOOST_CHECK(out.MakeResponse(&resp));

  std::string resp_text;
  GetResponse(resp, resp_text);

  using namespace boost;
  std::stringstream json_data;
  json_data << "{\"real\":" << std::fixed << kGetReal    << ","
               "\"integer\":"              << kGetInteger << ","
               "\"boolean\":"              << (kGetBoolean ? "true" : "false") << ","
               "\"string\":\""             << regex_replace(kQString, regex("\""), "\\\\\"") << "\"}";
  const std::string kJsonData = json_data.str();
  std::stringstream json_resp;
  json_resp << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: application/json\r\n"
            << "Content-Length: " << kJsonData.size() << "\r\n"
            << "Expires: " << resp.GetHeader().expires.ToString() << "\r\n"
            << "\r\n"
            << kJsonData;
  BOOST_CHECK(json_resp.str() == resp_text);
}

BOOST_AUTO_TEST_CASE(TestComHttpOutputAppend) {
  const std::string kMsg0("Message 0");
  const std::string kMsg1("Message 1");

  Proto::Response resp;
  http::Output out;
  http::Output sub_out;

  sub_out.Put("kMsg0", kMsg0);
  sub_out.Put("kMsg1", kMsg1);
  out.Put("kMsg0", kMsg0);
  out.Append("sub_out", sub_out);
  BOOST_CHECK(out.MakeResponse(&resp));

  std::string resp_text;
  GetResponse(resp, resp_text);

  using namespace boost;
  std::stringstream json_data;
  json_data << "{\"kMsg0\":\"" << kMsg0 << "\","
               "\"sub_out\":{"
            << "\"kMsg0\":\"" << kMsg0 << "\","
               "\"kMsg1\":\"" << kMsg1 << "\""
            << "}}";
  const std::string kJsonData = json_data.str();
  std::stringstream json_resp;
  json_resp << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: application/json\r\n"
            << "Content-Length: " << kJsonData.size() << "\r\n"
            << "Expires: " << resp.GetHeader().expires.ToString() << "\r\n"
            << "\r\n"
            << kJsonData;
  BOOST_CHECK(json_resp.str() == resp_text);
}

BOOST_AUTO_TEST_SUITE_END()
