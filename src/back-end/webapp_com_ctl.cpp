/*
 * webapp_com_ctl.cpp
 *
 *  Created on: 8 сент. 2016 г.
 *      Author: denis
 */

#include "webapp_com_ctl.hpp"
#include <iostream>

namespace webapp {
namespace ctl    {

// FileDialog ------------------------------------------------------------------
const std::string FileDialog::Action::kGoTo("goto");
const std::string FileDialog::Action::kOpen("open");

FileDialog::FileDialog(const std::string &name)
    : Com(name, "file_dialog"),
      _open_handler(0) {
  _cur_dir  = boost::filesystem::current_path();
  _root_dir = _cur_dir;
  _file_mask.assign(".+");

  RegisterAction(Action::kGoTo, [](const Com::Input &in, Com &obj)->Process {
    FileDialog &dia = static_cast<FileDialog&>(obj);
    return dia.GoTo(in.GetString("file"));
  });
  RegisterAction(Action::kOpen, [](const Com::Input &in, Com &obj)->Process {
    FileDialog &dia = static_cast<FileDialog&>(obj);
    return dia.Open(in.GetString("file"));
  });
}

static
std::string GetPathRelativeTo(boost::filesystem::path &root,
                              boost::filesystem::path &src) {
  auto src_it = src.begin();
  for (auto root_it: root) {
    src_it++;
  }
  std::string res;
  for (; src_it != src.end(); src_it++) {
    res += "/" + src_it->string();
  }
  return res;
}

bool FileDialog::PublishAttributes(Output *out) {
  namespace fs = boost::filesystem;
  Output::Scoped file(out->Create());
  Output::Scoped list_of_files(out->Create());
  fs::directory_iterator dir_end;

  if (_cur_dir.has_parent_path() && _cur_dir != _root_dir) {
    file->Put("is_directory", true);
    file->Put("is_regular",   false);
    list_of_files->Append("..", *file);
  }
  try {
    std::string filename;
    for (fs::directory_iterator dir_it(_cur_dir); dir_it != dir_end; ++dir_it) {
      const bool kIsDir = fs::is_directory(dir_it->status());
      filename = dir_it->path().filename().string();
      if (not kIsDir && not boost::regex_match(filename, _file_mask)) {
        continue;
      }
      file.reset(out->Create());
      file->Put("is_directory", kIsDir);
      file->Put("is_regular",   fs::is_regular_file(dir_it->status()));
      if (not kIsDir) {
        file->Put("size", (Com::Integer)file_size(*dir_it));
      }
      list_of_files->Append(filename, *file);
    }
  } catch (boost::filesystem::filesystem_error &ex) {
    _error_log << "Ошибка: " << ex.what();
  }
  out->Put("cur_dir", GetPathRelativeTo(_root_dir, _cur_dir));
  out->Put("error", _error_log.str());
  out->Append("files", *list_of_files);
  _error_log.str("");
  return true;
}

bool FileDialog::CheckName(const std::string &name) {
  namespace fs = boost::filesystem;
  if (name.empty()) {
    _error_log << "Ошибка: не указано имя файла";
    return false;
  }
  if (not fs::exists(_cur_dir)) {
    _error_log << "Ошибка: неверный путь к файлу: " << _cur_dir.string();
    return false;
  }
  return true;
}

FileDialog& FileDialog::SetRoot(const std::string &path) {
  _cur_dir  = boost::filesystem::path(path);
  _root_dir = _cur_dir;
  return *this;
}

FileDialog& FileDialog::SetMask(const std::string &expr) {
  _file_mask.assign(expr);
  return *this;
}

FileDialog& FileDialog::SetOpenHandler(Handler hndl) {
  _open_handler = hndl;
  return *this;
}

Com::Process FileDialog::GoTo(const std::string &name) {
  if (not CheckName(name)) {
    const Process kProcRes(Process::kError, _error_log.str());
    _error_log.str(std::string());
    return kProcRes;
  }
  if (name == "..") {
    if (_cur_dir != _root_dir) {
      _cur_dir = _cur_dir.parent_path();
    }
  } else {
    _cur_dir /= name;
  }
  return Process();
}

Com::Process FileDialog::Open(const std::string &name) {
  if (name.size() > 0) {
    _opening_file = name;
  }
  if (not CheckName(_opening_file)) {
    return Process();
  }
  const std::string kPath(_cur_dir.string() + _opening_file);
  const Process kHandlerRes = _open_handler != 0 ?
                              _open_handler(kPath) :
                              Process(Process::kError, "отсутствует обработчик");
  switch (kHandlerRes.status) {
    case Process::kInWork:
      break;
    case Process::kError:
      _error_log << "Ошибка: не удалось открыть файл: " << kPath << "; "
                                                        << kHandlerRes.message;
    case Process::kFinished:
    default:
      _opening_file = "";
      break;
  }
  return kHandlerRes;
}
// DataIFace -------------------------------------------------------------------
DataIFace::DataIFace() {
}

DataIFace::~DataIFace() {
}
// Table -----------------------------------------------------------------------
Table::Table(const std::string &name): Com(name, "table") {
}

void Table::SetDataInterface(DataIFace::Ptr data_if) {
  _data_if = data_if;
}

bool Table::PublishAttributes(Output *out) {
  if (not _data_if) {
    return false;
  }
  Output::Scoped data_out(out->Create());
  Output::Scoped meta_out(out->Create());
  _data_if->PublishData(data_out.get());
  _data_if->PublishMetaData(meta_out.get());
  out->Append("meta", *meta_out);
  out->Append("data", *data_out);
  return true;
}

}} // namespace webapp::ctl
