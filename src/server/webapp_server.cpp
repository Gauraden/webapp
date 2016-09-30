/*
 * webapp_server.cpp
 *
 *  Created on: 12 июля 2016 г.
 *      Author: denis
 */

#include <iostream>
#include "../back-end/webapp_com_http.hpp"
#include "../back-end/webapp_com_ctl.hpp"

class TestDataIf : public webapp::ctl::DataIFace {
  public:
    typedef void (*Handler)(webapp::Com::Output*);

    TestDataIf(Handler meta, Handler data)
        : webapp::ctl::DataIFace(),
          _meta_hndl(meta),
          _data_hndl(data) {
    }
    virtual ~TestDataIf() {}

    virtual void PublishData(webapp::Com::Output *out) {
      if (_data_hndl != 0) {
        _data_hndl(out);
      }
    }

    virtual void PublishMetaData(webapp::Com::Output *out) {
      if (_meta_hndl != 0) {
        _meta_hndl(out);
      }
    }

    static TestDataIf::Ptr Create(Handler meta, Handler data) {
      return TestDataIf::Ptr(new TestDataIf(meta, data));
    }
  private:
    Handler _meta_hndl;
    Handler _data_hndl;
};

static
const std::string& Num2Str(unsigned num) {
  static std::string res;
  static const char kSym[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
  res.clear();
  do {
    res += kSym[num % 10];
    num = num / 10;
  } while (num > 0);
  return res;
}

static
void FillInTable(webapp::Com::Output *table, unsigned rows, unsigned cols) {
  webapp::Com::Output::Scoped row;
  for (unsigned row_i = 0; row_i < rows; row_i++) {
    row.reset(table->Create());
    for (unsigned col_i = 0; col_i < cols; col_i++) {
      row->Put("col_" + Num2Str(col_i), (webapp::Com::Integer)((row_i * 100) + col_i));
    }
    table->Append("row_" + Num2Str(row_i), *row);
  }
}

static
void InitDialogs(const std::string &root_dir, webapp::http::Manager &manager) {
  typedef webapp::Com Com;
  using namespace webapp::ctl;

  manager.AddCom<FileDialog>("demo_file_dialog")
         .SetRoot(root_dir)
         .SetMask(".+\\.csv")
         .SetOpenHandler([](const std::string &filename)->Com::Process {
    static uint8_t req_num = 0;
    req_num++;
    if (req_num == 11) {
      req_num = 1;
    }
    // DEBUG --------------------
    std::cout << "DEBUG: " << __FUNCTION__ << ": " << __LINE__ << ": "
              << "file: "    << filename << "; "
              << "req_num: " << (unsigned)req_num << "; "
              << std::endl;
    // --------------------------
    return Com::Process(req_num < 10 ? Com::Process::kInWork :
                                       Com::Process::kFinished);
  });

  manager.AddCom<Table>("demo_table_0").SetDataInterface(TestDataIf::Create(
    // метаданные
    [](Com::Output *out) {
      Com::Output::Scoped col_0(out->Create());
      Com::Output::Scoped col_1(out->Create());

      col_0->Put("name", Com::String("Column #0"));
      col_1->Put("name", Com::String("Column #1"));

      out->Append("col_0", *col_0);
      out->Append("col_1", *col_1);
    },
    // данные
    [](Com::Output *out) {
      FillInTable(out, 10, 2);
    }
  ));
  manager.AddCom<Table>("demo_table_1").SetDataInterface(TestDataIf::Create(
    // метаданные
    [](Com::Output *out) {
      Com::Output::Scoped col_0(out->Create());
      Com::Output::Scoped col_1(out->Create());
      Com::Output::Scoped col_2(out->Create());

      col_0->Put("name", Com::String("Data #0"));
      col_1->Put("name", Com::String("Data #1"));
      col_2->Put("name", Com::String("Data #2"));

      out->Append("col_0", *col_0);
      out->Append("col_1", *col_1);
      out->Append("col_2", *col_2);
    },
    // данные
    [](Com::Output *out) {
      FillInTable(out, 20, 3);
    }
  ));
}

int main(int /*argc*/, char *argv[]) {
  namespace fs = boost::filesystem;
  static const std::string kExecPath(fs::system_complete(argv[0]).string());
  static const std::string kRootDir = fs::system_complete(argv[0]).parent_path().string();
  auto com_manager = webapp::http::Manager::Create();
  auto router      = webapp::http::Manager::GetRouterFor(com_manager,
                       "/static/demo_webapp.html");
  webapp::ServerHttp srv(router);
  router->AddDirectoryFor("/static", kRootDir + "/static");
  srv.BindTo(webapp::Server::kLoopbackAddress, 8080, webapp::Inspector::Create());
  InitDialogs(kRootDir, *com_manager);
  do {
    srv.Run();
  } while (1);
  return 0;
}
