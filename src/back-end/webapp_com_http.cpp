/*
 * webapp_com_http.cpp
 *
 *  Created on: 8 сент. 2016 г.
 *      Author: denis
 */

#include "webapp_com_http.hpp"
#include <cmath>
#include <cstddef>
#include <iostream>
#include <boost/regex.hpp>

namespace webapp {
namespace http   {
// Input -----------------------------------------------------------------------
Input::Input(const ProtocolHTTP::Request *http_req): _http_req(http_req) {
}

Input::~Input() {
}

bool Input::HasCell(const std::string &name) const {
  if (_http_req == 0) {
    return false;
  }
  return (_http_req->Get(name).size() > 0 || _http_req->Post(name) != 0);
}

template <typename ResType>
static
ResType ReadValFrom(const std::string           &name,
                    const ProtocolHTTP::Request *req,
                          ResType                def_val) {
  if (req == 0) {
    return def_val;
  }
  auto *field = req->Post(name);
  if (field != 0) {
    ResType res;
    field->get_value(&res);
    return res;
  }
  return req->Get(name, def_val);
}

Com::Real Input::GetReal(const std::string &name) const {
  static const Com::Real kNan = std::nan("");
  return ReadValFrom(name, _http_req, kNan);
}

Com::Integer Input::GetInteger(const std::string &name) const {
  return ReadValFrom(name, _http_req, std::numeric_limits<Com::Integer>::max());
}

Com::Boolean Input::GetBoolean(const std::string &name) const {
  return ReadValFrom(name, _http_req, false);
}

Com::String Input::GetString(const std::string &name) const {
  static const std::string kEmptyStr;
  return ReadValFrom(name, _http_req, kEmptyStr);
}
// Output ----------------------------------------------------------------------
Output::Output(): _use_comma(false) {
  _data << "{";
}

Output::~Output() {
}

const std::string& Output::Comma() {
  static const std::string kComma(",");
  static const std::string kVoid;
  return (_use_comma ? kComma : kVoid);
}

void Output::PutName(const std::string &name) {
  _data << Comma() << "\"" << name << "\":";
  _use_comma = true;
}

void Output::Put(const std::string &name, const Com::Real &val) {
  PutName(name);
  _data << std::fixed << val;
}

void Output::Put(const std::string &name, const Com::Integer &val) {
  PutName(name);
  _data << (long long)val;
}

void Output::Put(const std::string &name, const Com::Boolean &val) {
  PutName(name);
  _data << (val ? "true" : "false");
}

void Output::Put(const std::string &name, const Com::String &val) {
  PutName(name);
  using namespace boost;
  _data << "\"" << regex_replace(val, regex("\""), "\\\\\"") << "\"";
}

void Output::Append(const std::string &name, const Com::Output &src) {
  const Output &t_src = static_cast<const Output&>(src);
  PutName(name);
  _data << t_src._data.str() << "}";
}

Com::Output* Output::Create() {
  return new Output();
}

bool Output::MakeResponse(ProtocolHTTP::Response *out) {
  if (out == 0) {
    return false;
  }
  out->SetHeader(ProtocolHTTP::k200, ProtocolHTTP::Response::GetHeaderForJSON());
  _data << "}";//(_use_comma ? "}" : " ");
  out->SetBody(&_data);
  return true;
}
// Manager ---------------------------------------------------------------------
Manager::Manager() {
}

Manager::~Manager() {
}

Manager::Ptr Manager::Create() {
  return Manager::Ptr(new Manager());
}

ProtocolHTTP::Router::Ptr Manager::GetRouterFor(
          Manager::Ptr  man_ptr,
    const std::string  &root_path) {

  struct Handler : public ProtocolHTTP::Router::Functor {
    Handler(http::Manager::Ptr man_ptr)
        : ProtocolHTTP::Router::Functor(0),
          manager(man_ptr) {}
    virtual ~Handler() {}

    virtual bool operator()(const ProtocolHTTP::Uri::Path &path,
                            const ProtocolHTTP::Request   &request,
                                  ProtocolHTTP::Response  *response) {
      if (response == 0) {
        return false;
      }
      http::Input  in(&request);
      http::Output out;
      if (manager) {
        static const std::string kVoid;
        std::string com_path;
        for (auto path_node: path) {
          com_path += (com_path.size() > 0 ? "/" : kVoid) + path_node;
        }
        manager->HandleInput(com_path, in, &out);
      }
      return out.MakeResponse(response);
    }

    http::Manager::Ptr manager;
  };

  ProtocolHTTP::Router::Ptr          router(new ProtocolHTTP::Router(root_path));
  ProtocolHTTP::Router::Functor::Ptr handler_ptr(new Handler(man_ptr));
  router->AddHandlerFor("/webui", handler_ptr);
  return router;
}

}} // namespace webapp::http
