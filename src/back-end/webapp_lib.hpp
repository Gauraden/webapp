/*
 * webapp_lib.hpp
 *
 *  Created on: 12 июля 2016 г.
 *      Author: denis
 */

#ifndef BACK_END_WEBAPP_LIB_HPP_
#define BACK_END_WEBAPP_LIB_HPP_

#include <stdint.h>
#include <list>
#include <vector>
#include <map>
#include <fstream>
#include "boost/system/error_code.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/scoped_array.hpp"
#include "boost/date_time/posix_time/posix_time_types.hpp"

namespace webapp {

typedef uint32_t                 USize;
typedef boost::posix_time::ptime PTime;

// https://tools.ietf.org/html/rfc3986#section-3.1
class AbsoluteUri {
  public:
    AbsoluteUri();
    virtual ~AbsoluteUri();
    virtual bool ParseVal(const std::string &val);

    const std::string& get_scheme() const;
  protected:
    void   set_offset(size_t val);
    size_t get_offset() const;
  private:
    std::string _scheme;
    size_t      _offset;
};

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

class ProtocolHTTP : public Protocol {
  public:
    typedef std::list<std::string> ListOfString;
    // https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.1.1
    enum Method {
      kUnknown,
      kOptions,
      kGet,
      kHead,
      kPost,
      kPut,
      kDelete,
      kTrace,
      kConnect
    };
    // https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
    enum Code {
      k200 = 200, // OK
      k202 = 202, // Accepted

      k301 = 301, // Moved Permanently
      k304 = 304, // Not Modified

      k400 = 400, // Bad Request
      k403 = 403, // Forbidden
      k404 = 404, // Not Found

      k500 = 500, // Internal Server Error
      k501 = 501, // Not Implemented
      k505 = 505, // HTTP Version Not Supported
    }; // enum Code

    class Uri : public AbsoluteUri {
      public:
        // https://tools.ietf.org/html/rfc3986#section-3.3
        typedef std::vector<std::string>            Path;
        // https://tools.ietf.org/html/rfc3986#section-3.4
        typedef std::map<std::string, std::string>  Query;
        typedef std::pair<std::string, std::string> QueryParam;

        // https://tools.ietf.org/html/rfc3986#section-3.2
        struct Authority {
          std::string userinfo;
          std::string host;
          std::string port;
        };

        Uri();
        virtual ~Uri();
        virtual bool ParseVal(const std::string &val);

        const Authority&   get_authority() const;
        const Path&        get_path() const;
        const Query&       get_query() const;
        const std::string& get_fragment() const;
      private:
        bool CheckAuthority(const std::string &val);
        bool CheckPath(const std::string &val);
        bool CheckQuery(const std::string &val);
        bool CheckFragment(const std::string &val);

        Authority   _authority;
        Path        _path;
        Query       _query;
        std::string _fragment;
    }; // class Uri

    struct Line {
      Line(): method(kUnknown) {}
      Method      method;
      Uri         target;
      std::string version;
    };

    struct Content {
      // https://tools.ietf.org/html/rfc7231#section-3.1.1.1
      struct Type {
        // https://tools.ietf.org/html/rfc2046 -> "3.  Overview Of The Initial Top-Level Media Types"
        enum Name {
          kText,
          kImage,
          kAudio,
          kVideo,
          kApplication,
          kMultipart,
          kMessage
        };
        Type(): name(kApplication), sub_type("octet-stream") {}
        Type(Name n, const std::string &t): name(n), sub_type(t) {}
        Name        name;
        std::string sub_type;
        std::string boundary;
        std::string charset;
      };
      // https://tools.ietf.org/html/rfc1806#section-2
      struct Disposition {
        std::string type;
        std::string name;
        std::string filename;
      };

      Content(): length(0) {}
      Type         type;
      ListOfString encoding; // https://tools.ietf.org/html/rfc7231#section-3.1.2.2
      ListOfString language; // https://tools.ietf.org/html/rfc7231#section-3.1.3.2
      Uri          location; // https://tools.ietf.org/html/rfc7231#section-3.1.4.2
      USize        length;   // https://tools.ietf.org/html/rfc7230#section-3.3.2
      Disposition  disposition;
    }; // struct Content

    struct Accept {
      ListOfString language;
      ListOfString charset;
      ListOfString encoding;
    };
    // Cache-Control: https://tools.ietf.org/html/rfc7234#section-5.2
    class CacheControl {
      public:
        CacheControl& Reset();
        CacheControl& MaxAge(unsigned seconds);
        CacheControl& NoStore();
        CacheControl& NoCache();
        CacheControl& MustRevalidate();
        const std::string& get_directive() const;
      private:
        std::string _directive;
    };
    // https://tools.ietf.org/html/rfc7234#section-5.3
    struct Expires {
      Expires();
      Expires(const std::string &time_value);
      Expires& Now();
      Expires& Hour(USize value = 1);
      Expires& Day (USize value = 1);
      Expires& Week(USize value = 1);
      std::string ToString() const;

      PTime time;
    };

    struct Header {
      Header(): age(0), complete(false) {}
      Line         line;
      std::string  user_agent;
      std::string  host;
      Accept       accept;
      USize        age; // https://tools.ietf.org/html/rfc7234#section-5.1
      Content      content;
      Expires      expires;
      CacheControl cache_control;
      bool         complete;
    };

    // https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.3
    class Request {
      public:
        struct State;

        class Field {
          public:
            typedef std::map<std::string, Field> Map;

            class Storage {
              public:
                typedef boost::shared_ptr<Storage> Ptr;
                typedef Ptr (*Generator)(const Content::Type&);

                static const std::string& GetStaticName();

                Storage(const std::string &name);
                virtual ~Storage();
                virtual USize Size() const = 0;
                virtual void  Clear() = 0;
                virtual bool  Append(const Byte *data, USize size) = 0;
                virtual bool  operator()(std::ostream *out) const = 0;
                virtual bool  operator()(std::string *out) const = 0;
                const std::string& get_name() const;
              private:
                const std::string _name;
            };

            class StorageInMem: public Storage {
              public:
                static const std::string& GetStaticName();

                StorageInMem();
                virtual ~StorageInMem();
                StorageInMem(const StorageInMem&);
                void operator= (const StorageInMem&);

                virtual USize Size() const;
                virtual void  Clear();
                virtual bool  Append(const Byte *data, USize size);
                virtual bool  operator()(std::ostream *out) const;
                virtual bool  operator()(std::string *out) const;
              private:
                struct Container;
                Container *_container;
            };

            Field();
            ~Field();
            Field(const Field&);
            void operator= (const Field&);

            template <typename Type>
            bool                 get_value(Type *out) const;
            const Content::Type& get_type() const;

            bool               IsNull() const;
            USize              Size() const;
            void               Clear();
            bool               UseStorage(Storage *storage);
            bool               UseStorage(Storage::Ptr storage);
            const std::string& GetNameOfStorage() const;
            bool               Append(const Byte *data, USize size);
          private:
            struct Data;
            Data *_data;
        }; // class Field

        Request();
        ~Request();
        Request(const Request&);
        void operator= (const Request&);

        const Header& GetHeader() const;
        void UseStorageGenerator(Field::Storage::Generator generator);
        bool Completed() const;
        const std::string& Get(const std::string &name) const;
        template <typename GetType>
        GetType Get(const std::string &name, const GetType &def_val) const;
        const Field* const Post(const std::string &name) const;
      private:
        friend class ProtocolHTTP;

        void ResetState();

        State *_state;
    }; // class Request

    class Response {
      public:
        struct State;

        class Source {
          public:
            typedef boost::scoped_ptr<Source> Ptr;

            virtual bool  IsAvailable() const = 0;
            virtual USize Size() const = 0;
            virtual USize ReadSome(Byte *out, USize max_size) = 0;
        };

        class SourceFromFile : public Source {
          public:
            SourceFromFile(const std::string &file_name);
            virtual ~SourceFromFile();
            virtual bool  IsAvailable() const;
            virtual USize Size() const;
            virtual USize ReadSome(Byte *out, USize max_size);
          private:
            std::fstream _file;
            USize        _file_size;
        };

        class SourceFromStream : public Source {
          public:
            SourceFromStream(const std::string       &src);
            SourceFromStream(const std::stringstream &src);
            virtual ~SourceFromStream();
            virtual bool  IsAvailable() const;
            virtual USize Size() const;
            virtual USize ReadSome(Byte *out, USize max_size);
          private:
            std::stringstream _buff;
            USize             _buff_size;
        };

        class SourceFromArray : public Source {
          public:
            SourceFromArray(const Byte *data, USize size);
            virtual ~SourceFromArray();
            virtual bool  IsAvailable() const;
            virtual USize Size() const;
            virtual USize ReadSome(Byte *out, USize max_size);
          private:
            const Byte *_data;
            USize       _size;
            USize       _offset;
        };

        Response();
        ~Response();

        static Header GetHeaderForText(const std::string &format,
                                       const std::string &charset);
        static Header GetHeaderForImage(const std::string &format);
        static Header GetHeaderForJSON();

        CacheControl& UseCacheControl();

        void SetHeader(Code status_id);
        void SetHeader(Code status_id, const Header &header);
        void SetHeader(const std::string &type);
        void SetBody(const std::string &src);
        void SetBody(const Byte *data, USize size);
        bool UseFile(const Content::Type &type, const std::string &path);
      private:
        friend class ProtocolHTTP;

        Response(const Response&);
        void operator= (const Response&);
        void ResetState();

        State *_state;
    }; // class Response

    class Router {
      public:
        typedef boost::shared_ptr<Router> Ptr;

        class Functor {
          public:
            typedef boost::shared_ptr<Functor> Ptr;
            typedef bool (*Callback)(const Uri::Path &path,
                                     const Request   &request,
                                     Response        *response);

            Functor(Callback cb);
            virtual ~Functor();
            virtual bool operator()(const Uri::Path &path,
                                    const Request   &request,
                                          Response  *response);
          private:
            Callback _callback;
        };

        static Router::Ptr Create();

        Router();
        ~Router();
        bool CallHandlerFor(const Request &request, Response *response);
        bool AddHandlerFor(const std::string &url, const Functor &functor);
        bool AddHandlerFor(const std::string &url, Functor::Ptr &functor);
        bool AddDirectoryFor(const std::string &url, const std::string &dir);
      private:
        struct State;

        Router(const Router&);
        void operator= (const Router&);

        State *_state;
    }; // class Router

    static void DecodeString(const std::string &in, char label, std::string *out);

    ProtocolHTTP(Router::Ptr router);
    virtual ~ProtocolHTTP();

    bool  ParseRequest(Byte *req_bytes, size_t size, Request *out);
    USize GetResponse(Byte *out_resp_bytes, size_t out_size, Response *resp);

    virtual bool  HandleRequest(Byte *data, USize size);
    virtual USize PrepareResponse(Byte *data, USize size);
    virtual bool  NeedToCloseSession() const;
  private:
    struct State;

    ProtocolHTTP(const ProtocolHTTP&);
    void operator= (const ProtocolHTTP&);

    State *_state;
}; // class ProtocolHTTP

class Server {
  public:
    typedef uint16_t Port;
    typedef USize    Address;

    static const Address kUndefinedAddress = 0xFFFFFFFF;
    static const Address kLoopbackAddress  = 0x7F000001;
    static const Port    kUndefinedPort    = 0;

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

class ServerHttp : public Server {
  public:
    ServerHttp(ProtocolHTTP::Router::Ptr router);
    virtual ~ServerHttp();
  protected:
    virtual Protocol* InitProtocol();

    ProtocolHTTP::Router::Ptr _router;
}; // class ServerHttp

} // namespace webapp

#endif /* BACK_END_WEBAPP_LIB_HPP_ */
