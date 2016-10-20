/*
 * webapp_com.hpp
 *
 *  Created on: 31 авг. 2016 г.
 *      Author: denis
 */

#ifndef BACK_END_WEBAPP_COM_HPP_
#define BACK_END_WEBAPP_COM_HPP_

#include "webapp_proto.hpp"
#include <map>

namespace webapp {

class Com {
  public:
    typedef boost::shared_ptr<Com>      Ptr;
    typedef std::map<std::string, Ptr>  MapOfPtr;
    typedef std::pair<std::string, Ptr> Pair;

    typedef double      Real;
    typedef int64_t     Integer;
    typedef bool        Boolean;
    typedef std::string String;

    struct Attribute {
      static const char kType[];
      static const char kName[];
      static const char kLabel[];
      static const char kAction[];
      static const char kOption[];
      static const char kAsync[];
    };

    class Group;
    class Manager;

    class Input {
      public:
        typedef boost::shared_ptr<Input> Ptr;
        Input();
        virtual ~Input();

        virtual bool    HasCell   (const std::string &name) const = 0;
        virtual Real    GetReal   (const std::string &name) const = 0;
        virtual Integer GetInteger(const std::string &name) const = 0;
        virtual Boolean GetBoolean(const std::string &name) const = 0;
        virtual String  GetString (const std::string &name) const = 0;
    };

    class Output {
      public:
        typedef boost::scoped_ptr<Output> Scoped;
        typedef boost::shared_ptr<Output> Ptr;
        typedef void (*IncludeHandler)(Output &out, Com &com);

        Output();
        virtual ~Output();

        virtual void Put(const std::string &name, const Real    &val) = 0;
        virtual void Put(const std::string &name, const Integer &val) = 0;
        virtual void Put(const std::string &name, const Boolean &val) = 0;
        virtual void Put(const std::string &name, const String  &val) = 0;
        virtual void Append(const std::string &name, const Output &src) = 0;
        virtual Output* Create() = 0;
        virtual USize   Extract(Protocol::ArrayOfBytes &to);

        void Include(const std::string    &name,
                           Com            &parent_com,
                           IncludeHandler  handler);
    };

    struct Process;

    struct Action {
      typedef Process (*Handler)(const Input&, Com&);
      typedef std::map<std::string, Handler>  Map;
      typedef std::pair<std::string, Handler> Pair;
    };

    struct Process {
      static const Com::Integer kUnknownProgress = -1;

      enum Status {
        kInWork   = 0,
        kFinished,
        kError
      };

      Process();
      Process(Status _status, Com::Integer _progress = kUnknownProgress);
      Process(Status             _status,
              const std::string &_message,
              Com::Integer       _progress = kUnknownProgress);
      bool Publish(Output *out);

      Status                status;
      Com::Integer          progress;
      std::string           message;
      Action::Map::iterator action;
      bool                  need_to_notify;
    };

    Com(const std::string &name, const std::string &type);
    virtual ~Com();

    const std::string& get_name() const;
    Com& RegisterAction(const std::string &name, Action::Handler handler);
  protected:
    friend class Manager;

    virtual bool HandleAction(const std::string &act, const Input &in);
    virtual bool PublishAttributes(Output *out);
    virtual bool PublishMembers(Output *out);
    virtual Com::Ptr FindCom(const std::string &path);

    bool HandleData(const Input &in);
    bool Publish(Output *out);
    bool PublishProcessState(Output *out);
  private:
    Com();
    Com(const Com&);
    void operator= (const Com&);

    const std::string _name;
    const std::string _type;
    Action::Map       _actions;
    Process           _process_state;
};

class Com::Group : public Com {
  public:
    Group(const std::string &name);
    virtual ~Group();

    template <typename ComType>
    ComType& AddCom(const std::string &name) {
      return static_cast<ComType&>(*(
          _components.insert(Pair(name, Com::Ptr(new ComType(name))))
        ).first->second
      );
    }

    virtual Com::Ptr FindCom(const std::string &path);
  private:
    virtual bool PublishMembers(Output *out);

    MapOfPtr _components;
};

class Com::Manager : public Group {
  public:
    Manager();
    virtual ~Manager();

    bool HandleInput(const std::string &com_path, const Input &in, Output *out);
};

} // namespace webapp

#endif /* BACK_END_WEBAPP_COM_HPP_ */
