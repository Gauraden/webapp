/*
 * webapp_com_http.hpp
 *
 *  Created on: 8 сент. 2016 г.
 *      Author: denis
 */

#ifndef BACK_END_WEBAPP_COM_HTTP_HPP_
#define BACK_END_WEBAPP_COM_HTTP_HPP_

#include "webapp_com.hpp"
#include "webapp_proto_http.hpp"

namespace webapp {
namespace http   {

class Input : public Com::Input {
  public:
    Input(const ProtocolHTTP::Request *http_req);
    virtual ~Input();
    virtual bool         HasCell   (const std::string &name) const;
    virtual Com::Real    GetReal   (const std::string &name) const;
    virtual Com::Integer GetInteger(const std::string &name) const;
    virtual Com::Boolean GetBoolean(const std::string &name) const;
    virtual Com::String  GetString (const std::string &name) const;
  private:
    const ProtocolHTTP::Request *_http_req;
};

class Output : public Com::Output {
  public:
    Output();
    virtual ~Output();

    virtual void Put(const std::string &name, const Com::Real    &val);
    virtual void Put(const std::string &name, const Com::Integer &val);
    virtual void Put(const std::string &name, const Com::Boolean &val);
    virtual void Put(const std::string &name, const Com::String  &val);
    virtual void Append(const std::string &name, const Com::Output &src);
    virtual Com::Output* Create();
    bool MakeResponse(ProtocolHTTP::Response *out);
  private:
    const std::string& Comma();
    void PutName(const std::string &name);

    std::stringstream _data;
    bool              _use_comma;
};

class Manager : public Com::Manager, private boost::noncopyable {
  public:
    typedef boost::shared_ptr<Manager> Ptr;

    virtual ~Manager();
    static Ptr                       Create();
    static ProtocolHTTP::Router::Ptr GetRouterFor(Manager::Ptr       man_ptr,
                                                  const std::string &root_com_path);
  private:
    Manager();
};

}} // namespace webapp::http

#endif /* BACK_END_WEBAPP_COM_HTTP_HPP_ */
