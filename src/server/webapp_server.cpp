/*
 * webapp_server.cpp
 *
 *  Created on: 12 июля 2016 г.
 *      Author: denis
 */

#include <iostream>
#include "../back-end/webapp_lib.hpp"

static
std::string AsyncDataSelect() {
  for (unsigned i = 0; i < 2; i++) {
    sleep(1);
  }
  return "hello from async data select";
}

class TestDataIf : public webapp::ctl::DataIFace {
  public:
    typedef std::string Data;
    typedef void (*Handler)(const Data &data, webapp::Com::Output*);

    TestDataIf(Handler meta, Handler data)
        : webapp::ctl::DataIFace(),
          _meta_hndl(meta),
          _data_hndl(data) {
    }
    virtual ~TestDataIf() {}

    virtual webapp::Com::Process HandleData(const Request &req) {
      using namespace boost;
      typedef webapp::Com::Process Proc;
      if (_data_future.get_state() == future_state::ready) {
        _data = _data_future.get();
        DataFuture tmp_f;
        _data_future.swap(tmp_f);
        return Proc(Proc::kFinished);
      }
      if (_data_future.get_state() == future_state::uninitialized) {
        auto where    = req.SelectWhere();
        auto where_it = where.begin();
        for (; where_it != where.end(); where_it++) {
          // DEBUG --------------------
          std::cout << "DEBUG: " << __FUNCTION__ << ": " << __LINE__ << ": "
                    << "cond: " << where_it->field << " AND "
                                << where_it->value
                    << std::endl;
          // --------------------------
        }
        _data_future = webapp::Async(AsyncDataSelect);
      }
      return Proc(Proc::kInWork);
    }

    virtual void PublishData(webapp::Com::Output *out) {
      if (_data_hndl != 0) {
        _data_hndl(_data, out);
      }
    }

    virtual void PublishMetaData(webapp::Com::Output *out) {
      if (_meta_hndl != 0) {
        _meta_hndl(_data, out);
      }
    }

    static TestDataIf::Ptr Create(Handler meta, Handler data) {
      return TestDataIf::Ptr(new TestDataIf(meta, data));
    }
  private:
    typedef boost::unique_future<std::string> DataFuture;

    Handler    _meta_hndl;
    Handler    _data_hndl;
    DataFuture _data_future;
    Data       _data;
};

class FileManager {
  public:
    FileManager() {
    }
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
void FillInTable(webapp::Com::Output *table,
                 unsigned rows,
                 unsigned cols,
                 const TestDataIf::Data &data) {
  webapp::Com::Output::Scoped row;
  for (unsigned row_i = 0; row_i < rows; row_i++) {
    row.reset(table->Create());
    for (unsigned col_i = 0; col_i < cols; col_i++) {
      //row->Put("col_" + Num2Str(col_i), (webapp::Com::Integer)((row_i * 100) + col_i));
      row->Put("col_" + Num2Str(col_i), data);
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
    return Com::Process();
//    static uint8_t req_num = 0;
//    req_num++;
//    if (req_num == 11) {
//      req_num = 1;
//    }
//    // DEBUG --------------------
//    std::cout << "DEBUG: " << __FUNCTION__ << ": " << __LINE__ << ": "
//              << "file: "    << filename << "; "
//              << "req_num: " << (unsigned)req_num << "; "
//              << std::endl;
//    // --------------------------
//    return Com::Process(req_num < 10 ? Com::Process::kInWork :
//                                       Com::Process::kFinished);
  });

  manager.AddCom<Table>("demo_table_0").SetDataInterface(TestDataIf::Create(
    // метаданные
    [](const TestDataIf::Data &, Com::Output *out) {
      Com::Output::Scoped col_0(out->Create());
      Com::Output::Scoped col_1(out->Create());

      col_0->Put("name", Com::String("Column #0"));
      col_1->Put("name", Com::String("Column #1"));

      out->Append("col_0", *col_0);
      out->Append("col_1", *col_1);
    },
    // данные
    [](const TestDataIf::Data &data, Com::Output *out) {
      FillInTable(out, 15, 2, data);
    }
  ));
  manager.AddCom<Table>("demo_table_1").SetDataInterface(TestDataIf::Create(
    // метаданные
    [](const TestDataIf::Data &, Com::Output *out) {
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
    [](const TestDataIf::Data &data, Com::Output *out) {
      FillInTable(out, 20, 3, data);
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
