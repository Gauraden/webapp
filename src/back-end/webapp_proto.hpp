/*
 * webapp_proto.hpp
 *
 *  Created on: 30 авг. 2016 г.
 *      Author: denis
 */

#ifndef BACK_END_WEBAPP_PROTO_HPP_
#define BACK_END_WEBAPP_PROTO_HPP_

#include <stdint.h>
#include "boost/system/error_code.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/scoped_array.hpp"
#include "boost/date_time/posix_time/posix_time_types.hpp"
#include "boost/noncopyable.hpp"

namespace webapp {

typedef uint32_t                 USize;
typedef boost::posix_time::ptime PTime;

class Inspector {
  public:
    typedef boost::system::error_code    Error;
    typedef boost::shared_ptr<Inspector> Ptr;

    static Inspector::Ptr Create();

    Inspector();
    virtual ~Inspector();

    virtual void RegisterError(const std::string &msg, const Error &error);
    virtual void RegisterMessage(const std::string &msg);
};

class Protocol {
  public:
    typedef boost::shared_ptr<Protocol> Ptr;
    typedef uint8_t                     Byte;
    typedef boost::scoped_array<Byte>   ArrayOfBytes;

    Protocol();
    virtual ~Protocol();

    virtual bool  HandleRequest(Byte *data, USize size) = 0;
    virtual USize PrepareResponse(Byte *data, USize size) = 0;
    virtual bool  NeedToCloseSession() const = 0;
};

class Server {
  public:
    typedef uint16_t Port;
    typedef USize    Address;

    static const Address kUndefinedAddress;
    static const Address kLoopbackAddress;
    static const Address kAllIPv4Addresses;
    static const Port    kUndefinedPort;

    Server();
    virtual ~Server();

    bool BindTo(Address addr, Port port, Inspector::Ptr inspector);
    bool Unbind();
    void Run();
  protected:
    virtual Protocol* InitProtocol() = 0;
  private:
    class Resources;

    Server(const Server&);
    void operator= (const Server&);

    Resources *_res;
}; // class Server

} // namespace webapp

#endif /* BACK_END_WEBAPP_PROTO_HPP_ */
