/*
 * com_ctl_test.cpp
 *
 *  Created on: 31 авг. 2016 г.
 *      Author: denis
 */

#include <iostream>
#include <sstream>
#include <boost/test/unit_test.hpp>
#include "webapp_com_ctl.hpp"

typedef webapp::Com com;

struct ComCtlTestFixture {

};

class DemoInput : public com::Input {
  public:
    DemoInput(): com::Input() {
    }

    virtual bool HasCell (const std::string &) const {
      return false;
    }

    virtual com::Real GetReal (const std::string &) const {
      return false;
    }

    virtual com::Integer GetInteger(const std::string &) const {
      return false;
    }

    virtual com::Boolean GetBoolean(const std::string &) const {
      return false;
    }

    virtual com::String GetString (const std::string &name) const {
      if (name == "fields") {
        return fields;
      }
      return conditions;
    }

    std::string fields;
    std::string conditions;
};
// -----------------------------------------------------------------------------
// Инициализация набора тестов
BOOST_FIXTURE_TEST_SUITE(ComCtlTestSuite, ComCtlTestFixture)

BOOST_AUTO_TEST_CASE(TestTableRequestCondition) {
  using namespace webapp::ctl;

  typedef Table::Request::Condition Cond;

  Cond con_0("field_0=10");
  Cond con_1("field_1<11");
  Cond con_2("field_2>12");
  Cond con_3("field_3<=13");
  Cond con_4("field_4>=14");

  BOOST_CHECK(con_0.field == "field_0");
  BOOST_CHECK(con_0.value == "10");
  BOOST_CHECK(con_0.logic == Cond::kEq);

  BOOST_CHECK(con_1.field == "field_1");
  BOOST_CHECK(con_1.value == "11");
  BOOST_CHECK(con_1.logic == Cond::kLt);

  BOOST_CHECK(con_2.field == "field_2");
  BOOST_CHECK(con_2.value == "12");
  BOOST_CHECK(con_2.logic == Cond::kGt);

  BOOST_CHECK(con_3.field == "field_3");
  BOOST_CHECK(con_3.value == "13");
  BOOST_CHECK(con_3.logic == Cond::kEqOrLt);

  BOOST_CHECK(con_4.field == "field_4");
  BOOST_CHECK(con_4.value == "14");
  BOOST_CHECK(con_4.logic == Cond::kEqOrGt);
}

BOOST_AUTO_TEST_CASE(TestTableRequestSelect) {
  using namespace webapp::ctl;
  typedef Table::Request Req;

  const std::string kFields[] = {
    "field_0",
    "field_1",
    "field_2"
  };

  DemoInput in;

  for (auto fid = 0; fid < 3; fid++) {
    in.fields += (fid == 0 ? "" : ",") + kFields[fid];
  }

  Req req(in);
  for (auto fid = 0; fid < 3; fid++) {
    BOOST_CHECK(req.NeedToSelectThis(kFields[fid]));
  }
}

BOOST_AUTO_TEST_CASE(TestTableRequestWhere) {
  using namespace webapp::ctl;
  typedef Table::Request Req;

  const std::string kWhere[] = {
    "field_0==9",
    "field_1<8",
    "field_2>7",
    "field_3<=6",
    "field_4>=5"
  };

  DemoInput            in;
  Req::Condition::List conds;

  for (auto fid = 0; fid < 3; fid++) {
    in.conditions += (fid == 0 ? "" : "&&") + kWhere[fid];
    conds.emplace_back(kWhere[fid]);
  }

  Req req(in);
  auto req_where = req.SelectWhere();
  auto cond_it   = conds.begin();
  for (auto it = req_where.begin();
       it != req_where.end(), cond_it != conds.end();
       it++, cond_it++) {
    BOOST_CHECK(it->field == cond_it->field);
    BOOST_CHECK(it->value == cond_it->value);
    BOOST_CHECK(it->logic == cond_it->logic);
  }
}

BOOST_AUTO_TEST_SUITE_END()
