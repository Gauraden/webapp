/*
 * webapp_server.cpp
 *
 *  Created on: 12 июля 2016 г.
 *      Author: denis
 */

#include <iostream>
#include "../back-end/webapp_lib.hpp"

static
bool TestHandler(const webapp::ProtocolHTTP::Uri::Path &,
                 const webapp::ProtocolHTTP::Request   &,
                       webapp::ProtocolHTTP::Response  *response) {
  response->SetBody("Hello from vbr web app!");
  return true;
}

static
bool TestPupokHandler(const webapp::ProtocolHTTP::Uri::Path &,
                      const webapp::ProtocolHTTP::Request   &,
                            webapp::ProtocolHTTP::Response  *response) {
  response->SetBody("Hello from vbr web app! /test/pupok !");
  return true;
}

int main(/*int argc, char *argv[]*/) {
  webapp::ProtocolHTTP::Router::Ptr router = webapp::ProtocolHTTP::Router::Create();
  webapp::ServerHttp srv(router);
  router->AddHandlerFor("/test",       TestHandler);
  router->AddHandlerFor("/test/pupok", TestPupokHandler);
  router->AddDirectoryFor("/docs",     "/home/denis/Документы");
  srv.BindTo(webapp::Server::kLoopbackAddress, 8080, webapp::Inspector::Create());
  do {
    srv.Run();
  } while (1);
  return 0;
}
