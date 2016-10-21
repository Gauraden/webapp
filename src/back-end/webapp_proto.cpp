/*
 * webapp_proto.cpp
 *
 *  Created on: 30 авг. 2016 г.
 *      Author: denis
 */

#include "webapp_proto.hpp"

#include <list>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace webapp {

namespace asio = ::boost::asio;

// Inspector -------------------------------------------------------------------
Inspector::Inspector() {
}

Inspector::~Inspector() {
}

void Inspector::RegisterError(const std::string &msg, const Error &error) {
  std::cout << msg << ": " << error << "; "
            << std::endl;
}

void Inspector::RegisterMessage(const std::string &msg) {
  std::cout << msg
            << std::endl;
}

Inspector::Ptr Inspector::Create() {
  return Inspector::Ptr(new Inspector());
}
// Server ----------------------------------------------------------------------
const Server::Address Server::kUndefinedAddress = 0xFFFFFFFF;
const Server::Address Server::kLoopbackAddress  = 0x7F000001;
const Server::Address Server::kAllIPv4Addresses = 0x00000000;
const Server::Port    Server::kUndefinedPort    = 0;

class Server::Resources {
  public:
    /**
     * Сессия сетевого подключения
     */
    class Session {
      public:
        typedef void (*OnConnection)(Server::Resources *srv_res);
        typedef asio::ip::tcp::socket      Socket;
        typedef boost::scoped_ptr<Socket>  SocketPtr;
        typedef asio::ip::tcp::endpoint    EndPoint;
        typedef boost::shared_ptr<Session> Ptr;
        typedef std::list<Ptr>             ListOfPtr;

        static const USize kDefBuffSize = 1514; // MTU

        Session(Socket         *socket_ptr,
                Protocol       *protocol_ptr,
                Inspector::Ptr  inspector_ptr)
            : socket(socket_ptr),
              protocol(protocol_ptr),
              inspector(inspector_ptr),
              need_to_send(0),
              was_sended(0) {
          buff.reset(new Protocol::Byte[kDefBuffSize]);
        }

        void WaitForRequest() {
          if (socket == 0) {
            return;
          }
          need_to_send = 0;
          was_sended   = 0;
          socket->async_read_some(boost::asio::buffer(buff.get(), kDefBuffSize),
            boost::bind(&Session::ReadHandler,
              this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
        }

        void SendResponse() {
          if (not protocol) {
            inspector->RegisterError("Ошибка, протокол не был инициализирован",
                                     Inspector::Error());
            return;
          }
          const USize kWasLost = need_to_send - was_sended;
          need_to_send = kWasLost;
          if (kWasLost == 0) {
            need_to_send = protocol->PrepareResponse(buff.get(), kDefBuffSize);
            was_sended   = 0;
          }
          if (need_to_send == 0) {
            if (protocol->NeedToCloseSession()) {
              socket->close();
              return;
            }
            WaitForRequest();
            return;
          }
          socket->async_write_some(asio::buffer(buff.get() + was_sended,
                                   need_to_send),
            boost::bind(&Session::WriteHandler,
              this,
              asio::placeholders::error,
              asio::placeholders::bytes_transferred));
        }

        void ReadHandler(const Inspector::Error &error, size_t amount) {
          if (error) {
            inspector->RegisterError("Ошибка чтения данных", error);
            return;
          }
          if (amount == 0) {
            return;
          }
          if (not protocol) {
            inspector->RegisterError("Ошибка, протокол не был инициализирован",
                                     Inspector::Error());
            return;
          }
          // чтение данных до тех пор, пока реализация протокола не скажет хватит
          if (protocol->HandleRequest(buff.get(), amount)) {
            WaitForRequest();
            return;
          }
          SendResponse();
        }

        void WriteHandler(const Inspector::Error &error, size_t amount) {
          if (error) {
            inspector->RegisterError("Ошибка отправки данных", error);
            return;
          }
          was_sended = amount;
          SendResponse();
        }

        bool IsAlive() const {
          return (socket != 0 && socket->is_open());
        }

        SocketPtr              socket;
        Protocol::ArrayOfBytes buff;
        Protocol::Ptr          protocol;
        Inspector::Ptr         inspector;
        USize                  need_to_send;
        USize                  was_sended;
    };

    Resources(Address         addr,
              Port            port,
              Server         *server_ptr,
              Inspector::Ptr  inspector_ptr)
        : server(server_ptr),
          inspector(inspector_ptr),
          acceptor(service, asio::ip::tcp::endpoint(asio::ip::address_v4(addr),
                                                    port)),
          sess_socket(0) {
      eth_addr = addr;
      eth_port = port;
      if (not inspector) {
        inspector.reset(new Inspector());
      }
      WaitForConnection();
    }

    void CloseBrokenSessions() {
      Session::ListOfPtr::iterator it = sessions.begin();
      for (; it != sessions.end();) {
        Session::Ptr sess = *it;
        if (not sess || not sess->IsAlive()) {
          it = sessions.erase(it);
          inspector->RegisterMessage("Удаление сессии...");
          continue;
        }
        it++;
      }
    }

    void WaitForConnection() {
      sess_socket = new Session::Socket(service);
      acceptor.async_accept(
        *sess_socket,
        boost::bind(&Resources::HandleAccept,
                    this,
                    asio::placeholders::error));
    }

    void HandleAccept(const Inspector::Error &error) {
      if (error) {
        inspector->RegisterError("Ошибка подключения", error);
        return;
      }
      inspector->RegisterMessage("Обнаружено подключение! Создание сессии...");
      Protocol *protocol = server->InitProtocol();
      if (sess_socket == 0 || protocol == 0) {
        return;
      }
      /*
       * [19 авг. 2016 г.] denis: в сессию передаются 2 указателя,
       * на сокет (sess_socket) и на реализацию протокола (protocol). Отныне,
       * именно сессия управляет этими объектами!
       */
      Session::Ptr sess(new Session(sess_socket, protocol, inspector));
      sessions.push_back(sess);
      sess_socket = 0;
      WaitForConnection();
      sess->WaitForRequest();
    }

    Address                  eth_addr;
    Port                     eth_port;
    Server                  *server;
    Inspector::Ptr           inspector;
    Session::ListOfPtr       sessions;
    boost::asio::io_service  service;
    asio::ip::tcp::acceptor  acceptor;
    Session::Socket         *sess_socket;
};

Server::Server(): _res(0) {
}

Server::~Server() {
  Unbind();
}

bool Server::BindTo(Address addr, Port port, Inspector::Ptr inspector) {
  if (addr == kUndefinedAddress || port == kUndefinedPort) {
    return false;
  }
  if (_res != 0) {
    Unbind();
  }
  _res = new Resources(addr, port, this, inspector);
  return true;
}

bool Server::Unbind() {
  delete _res;
  _res = 0;
  return true;
}

void Server::Run() {
  _res->service.run_one();
  _res->CloseBrokenSessions();
}

} // namespace webapp
