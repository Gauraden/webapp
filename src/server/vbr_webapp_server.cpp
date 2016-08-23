/*
 * vbr_webapp_server.cpp
 *
 *  Created on: 12 июля 2016 г.
 *      Author: denis
 */

#include "vbr_webapp_lib.hpp"
#include <iostream>

bool TestHandler(const webapp::ProtocolHTTP::Uri::Path &path,
                 const webapp::ProtocolHTTP::Request   &request,
                       webapp::ProtocolHTTP::Response  *response) {
  response->SetHeader(webapp::ProtocolHTTP::k200,
                      webapp::ProtocolHTTP::Response::GetHeaderForText("plain", ""));
  response->SetBody("Hello from vbr web app!");
  return true;
}

bool TestPupokHandler(const webapp::ProtocolHTTP::Uri::Path &path,
                      const webapp::ProtocolHTTP::Request   &request,
                            webapp::ProtocolHTTP::Response  *response) {
  response->SetHeader(webapp::ProtocolHTTP::k200,
                      webapp::ProtocolHTTP::Response::GetHeaderForText("plain", ""));
  response->SetBody("Hello from vbr web app! /test/pupok !");
  return true;
}

int main(int argc, char *argv[]) {
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
