/*
 * com_test.cpp
 *
 *  Created on: 31 авг. 2016 г.
 *      Author: denis
 */

#define empty() __empty()
#include <iostream>
#include <sstream>
#include <boost/test/unit_test.hpp>
#include "webapp_com.hpp"

struct ComTestFixture {

};

typedef webapp::Com com;

class TestInput : public com::Input {
  public:
    TestInput()
        : com::Input(),
          real(nanf("")),
          integer(0),
          boolean(false) {
    }

    virtual bool HasCell (const std::string &name) const {
      return (name == "real") ||
             (name == "integer") ||
             (name == "boolean") ||
             (name == "string");
    }

    virtual com::Real GetReal (const std::string &name) const {
      if (name == "real") {
        return real;
      }
      return nanf("");
    }

    virtual com::Integer GetInteger(const std::string &name) const {
      if (name == "integer") {
        return integer;
      }
      return 0;
    }

    virtual com::Boolean GetBoolean(const std::string &name) const {
      if (name == "boolean") {
        return boolean;
      }
      return false;
    }

    virtual com::String GetString (const std::string &name) const {
      if (name == "string") {
        return string;
      }
      if (name == "action") {
        return action;
      }
      return "";
    }

    com::Real    real;
    com::Integer integer;
    com::Boolean boolean;
    com::String  string;
    com::String  action;
};

class TestOutput : public com::Output {
  public:
    TestOutput(): com::Output() {
    }

    virtual void Put(const std::string &name, const com::Real &val) {
      data << name << "=" << std::fixed << val << " ";
    }

    virtual void Put(const std::string &name, const com::Integer &val) {
      data << name << "=" << val << " ";
    }

    virtual void Put(const std::string &name, const com::Boolean &val) {
      data << name << "=" << (val ? "true" : "false") << " ";
    }

    virtual void Put(const std::string &name, const com::String &val) {
      data << name << "='" << val << "' ";
    }

    virtual void Append(const std::string &name, const Output &src) {
      const TestOutput &t_src = static_cast<const TestOutput&>(src);
      data << name << "=[" << t_src.data.str() << "] ";
    }

    virtual Output* Create() {
      return new TestOutput();
    }

    virtual webapp::USize Extract(webapp::Protocol::ArrayOfBytes &to) {
      const std::string kOut = data.str();
      to.reset(new webapp::Protocol::Byte[kOut.size()]);
      memcpy(to.get(), kOut.c_str(), kOut.size());
      return kOut.size();
    }

    std::stringstream data;
};

class TestCom : public com {
  public:
    static const com::Real    kDefReal;
    static const com::Integer kDefInteger;

    static const char kActReset[];
    static const char kActUpdate[];

    TestCom(const std::string &name)
        : com(name, "test"),
          real(kDefReal),
          integer(kDefInteger) {
    }

    bool MakePublishing(Output *out) {
      return Publish(out);
    }

    bool MakeHandling(const Input &in) {
      return HandleData(in);
    }

    com::Real    real;
    com::Integer integer;
  protected:
    virtual bool PublishAttributes(Output *out) {
      out->Put("real",    real);
      out->Put("integer", integer);
      return true;
    }
};

const com::Real    TestCom::kDefReal     = 0.0;
const com::Integer TestCom::kDefInteger  = 0;
const char         TestCom::kActReset[]  = "reset";
const char         TestCom::kActUpdate[] = "update";
// -----------------------------------------------------------------------------
// Инициализация набора тестов
BOOST_FIXTURE_TEST_SUITE(ComTestSuite, ComTestFixture)

BOOST_AUTO_TEST_CASE(TestComInputGet) {
  TestInput       *t_in_ptr(new TestInput());
  com::Input::Ptr  in_ptr(t_in_ptr);

  const com::Real    kReal    = 666.777;
  const com::Integer kInteger = 1234567;
  const com::Boolean kBoolean = true;
  const com::String  kString("hello world");

  t_in_ptr->real    = kReal;
  t_in_ptr->integer = kInteger;
  t_in_ptr->boolean = kBoolean;
  t_in_ptr->string  = kString;

  BOOST_CHECK(in_ptr->GetReal("real")       == kReal);
  BOOST_CHECK(in_ptr->GetInteger("integer") == kInteger);
  BOOST_CHECK(in_ptr->GetBoolean("boolean") == kBoolean);
  BOOST_CHECK(in_ptr->GetString("string")   == kString);
}

BOOST_AUTO_TEST_CASE(TestComOutputPut) {
  TestOutput       *t_out_ptr(new TestOutput());
  com::Output::Ptr  out_ptr(t_out_ptr);
  com::Output::Ptr  sub_out_ptr(t_out_ptr->Create());

  const com::Real    kReal    = 666.777;
  const com::Integer kInteger = 1234567;
  const com::Boolean kBoolean = true;
  const com::String  kString("waaaaaarghhhh");
  std::stringstream model;
  model << "real="    << std::fixed << kReal    << " "
        << "integer="               << kInteger << " "
        << "boolean="               << (kBoolean ? "true" : "false") << " "
        << "string='"               << kString << "' "
        << "group=["
          << "integer=" << kInteger << " "
          << "string='"  << kString  << "' "
        << "] ";

  out_ptr->Put("real",    kReal);
  out_ptr->Put("integer", kInteger);
  out_ptr->Put("boolean", kBoolean);
  out_ptr->Put("string",  kString);

  sub_out_ptr->Put("integer", kInteger);
  sub_out_ptr->Put("string",  kString);

  out_ptr->Append("group", *sub_out_ptr);

  BOOST_CHECK(t_out_ptr->data.str() == model.str());

  webapp::Protocol::ArrayOfBytes bytes;
  const webapp::USize kBytesSz = out_ptr->Extract(bytes);
  BOOST_CHECK(memcmp(bytes.get(), model.str().c_str(), kBytesSz) == 0);
}

BOOST_AUTO_TEST_CASE(TestComGroupAddFind) {
  com::Group grp("grp");
  TestCom    &test_com_0   = grp.AddCom<TestCom>("test_com_0");
  TestCom    &test_com_1   = grp.AddCom<TestCom>("test_com_1");
  com::Group &grp_0        = grp.AddCom<com::Group>("grp_0");
  com::Group &grp_1        = grp.AddCom<com::Group>("grp_1");
  TestCom    &test_com_0_1 = grp_0.AddCom<TestCom>("test_com_0_1");

  BOOST_CHECK(grp.FindCom("test_com_0"));
  BOOST_CHECK(grp.FindCom("test_com_1"));
  BOOST_CHECK(not grp.FindCom("test_com_2"));

  BOOST_CHECK(grp.FindCom("grp_0"));
  BOOST_CHECK(grp.FindCom("grp_0/test_com_0_1"));
  BOOST_CHECK(not grp.FindCom("grp_0/test_com_0_2"));

  BOOST_CHECK(grp.FindCom("grp_1"));
  BOOST_CHECK(not grp.FindCom("grp_2"));

  BOOST_CHECK(grp.FindCom("grp_0/test_com_0_1")->get_name() == "test_com_0_1");
}

BOOST_AUTO_TEST_CASE(TestComPublish) {
  const std::string  kName("t_com");
  const com::Real    kReal    = 6789.9876;
  const com::Integer kInteger = 0xFF00FF00;
  TestOutput       *t_out_ptr(new TestOutput());
  com::Output::Ptr  out_ptr(t_out_ptr);

  TestCom t_com(kName);
  t_com.real    = kReal;
  t_com.integer = kInteger;
  BOOST_CHECK( not t_com.MakePublishing(0)  );
  BOOST_CHECK( t_com.MakePublishing(out_ptr.get()) );

  std::stringstream model;
  model << "type='test' "
        << "name='"                 << kName    << "' "
        << "real="    << std::fixed << kReal    << " "
        << "integer="               << kInteger << " ";

  BOOST_CHECK(t_out_ptr->data.str() == model.str());
}

BOOST_AUTO_TEST_CASE(TestComHandleData) {
  const com::Real    kReal    = 3.1415926535;
  const com::Integer kInteger = 0x01020408;

  TestInput t_in;
  TestCom   t_com("t_com");

  t_com.RegisterAction(TestCom::kActUpdate, [](const com::Input &in, com &obj)->com::Process{
    TestCom &t_obj = static_cast<TestCom&>(obj);
    t_obj.real    = in.GetReal("real");
    t_obj.integer = in.GetInteger("integer");
    return com::Process();
  });

  t_com.RegisterAction(TestCom::kActReset, [](const com::Input&, com &obj)->com::Process{
    TestCom &t_obj = static_cast<TestCom&>(obj);
    t_obj.real    = TestCom::kDefReal;
    t_obj.integer = TestCom::kDefInteger;
    return com::Process();
  });

  t_in.action  = TestCom::kActUpdate;
  t_in.real    = kReal;
  t_in.integer = kInteger;
  BOOST_CHECK(t_com.MakeHandling(t_in));
  BOOST_CHECK(t_com.real    == kReal);
  BOOST_CHECK(t_com.integer == kInteger);

  t_in.action  = TestCom::kActReset;
  BOOST_CHECK(t_com.MakeHandling(t_in));
  BOOST_CHECK(t_com.real    == TestCom::kDefReal);
  BOOST_CHECK(t_com.integer == TestCom::kDefInteger);

  t_in.action  = "unknown";
  BOOST_CHECK(not t_com.MakeHandling(t_in));
  BOOST_CHECK(t_com.real    == TestCom::kDefReal);
  BOOST_CHECK(t_com.integer == TestCom::kDefInteger);
}

BOOST_AUTO_TEST_CASE(TestComManagerHandleInput) {
  static const std::string  kTCom0("t_com_0");
  static const std::string  kTGrp0("t_grp_0");
  static const std::string  kTCom00("t_com_0_0");
  static const com::Real    kReal    = 6.022140857;
  static const com::Integer kInteger = 142000000;

  com::Manager t_man;

  TestInput  t_in;
  TestOutput t_out;

  t_man.AddCom<TestCom>(kTCom0).RegisterAction(
    TestCom::kActUpdate,
    [](const com::Input &in, com &obj)->com::Process {
      TestCom &t_obj = static_cast<TestCom&>(obj);
      t_obj.real    = in.GetReal("real");
      t_obj.integer = in.GetInteger("integer");
      return com::Process();
  }).RegisterAction(
    TestCom::kActReset,
    [](const com::Input&, com &obj)->com::Process {
      TestCom &t_obj = static_cast<TestCom&>(obj);
      t_obj.real    = TestCom::kDefReal;
      t_obj.integer = TestCom::kDefInteger;
      return com::Process();
  });
  t_man.AddCom<com::Group>(kTGrp0).AddCom<TestCom>(kTCom00).RegisterAction(
    TestCom::kActUpdate,
    [](const com::Input &in, com &obj)->com::Process {
      TestCom &t_obj = static_cast<TestCom&>(obj);
      t_obj.real    = in.GetReal("real");
      t_obj.integer = in.GetInteger("integer");
      return com::Process();
  });

  std::stringstream model;
  model << "type='test' name='" << kTCom0 << "' "
        << "real="    << std::fixed << kReal    << " "
        << "integer="               << kInteger << " ";

  t_in.action    = TestCom::kActUpdate;
  t_in.real      = kReal;
  t_in.integer   = kInteger;
  BOOST_CHECK(t_man.HandleInput(kTCom0, t_in, &t_out));
  BOOST_CHECK(t_out.data.str() == model.str());

  t_out.data.str("");
  model.str("");
  model << "type='test' name='" << kTCom00 << "' "
        << "real="    << std::fixed << kReal    << " "
        << "integer="               << kInteger << " ";

  BOOST_CHECK(t_man.HandleInput(kTGrp0 + "/" + kTCom00, t_in, &t_out));
  BOOST_CHECK(t_out.data.str() == model.str());

  t_out.data.str("");
  model.str("");
  model << "type='test' name='" << kTCom0 << "' "
        << "real=" << std::fixed << TestCom::kDefReal << " "
        << "integer=" << TestCom::kDefInteger << " ";
  t_in.action    = TestCom::kActReset;
  BOOST_CHECK(t_man.HandleInput(kTCom0, t_in, &t_out));
  BOOST_CHECK(t_out.data.str() == model.str());
}

BOOST_AUTO_TEST_SUITE_END()
