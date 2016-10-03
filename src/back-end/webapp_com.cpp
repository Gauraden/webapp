/*
 * webapp_com.cpp
 *
 *  Created on: 31 авг. 2016 г.
 *      Author: denis
 */

#include "webapp_com.hpp"
#include <iostream>

namespace webapp {

const char Com::Attribute::kType[]   = "type";
const char Com::Attribute::kName[]   = "name";
const char Com::Attribute::kLabel[]  = "label";
const char Com::Attribute::kAction[] = "action";
const char Com::Attribute::kOption[] = "option";
const char Com::Attribute::kAsync[]  = "async";
// Com::Input ------------------------------------------------------------------
Com::Input::Input() {
}

Com::Input::~Input() {
}
// Com::Output -----------------------------------------------------------------
Com::Output::Output() {
}

Com::Output::~Output() {
}

USize Com::Output::Extract(Protocol::ArrayOfBytes&) {
  return 0;
}

void Com::Output::Include(const std::string    &name,
                                Com            &parent_com,
                                IncludeHandler  handler) {
  if (handler == 0) {
    return;
  }
  boost::scoped_ptr<Output> out(Create());
  if (not out) {
    return;
  }
  handler(*out, parent_com);
  Append(name, *out);
}
// Com::Group ------------------------------------------------------------------
Com::Group::Group(const std::string &name): Com(name, "group") {
}

Com::Group::~Group() {
}

Com::Ptr Com::Group::FindCom(const std::string &path) {
  Com::Ptr res;
  if (path.size() == 0) {
    return res;
  }
  const size_t      kSlashOff = path.find_first_of('/');
  const std::string kNodeName = path.substr(0, kSlashOff);
  auto com_it = _components.find(kNodeName);
  if (com_it == _components.end()) {
    return res;
  }
  if (kSlashOff != std::string::npos) {
    return com_it->second->FindCom(path.substr(kSlashOff+1));
  }
  return com_it->second;
}

bool Com::Group::PublishMembers(Output *out) {
  if (out == 0) {
    return false;
  }
  for (auto comp = _components.begin(); comp != _components.end(); comp++) {
    comp->second->Publish(out);
  }
  return true;
}
// Com::Manager ----------------------------------------------------------------
Com::Manager::Manager(): Com::Group("manager") {
}

Com::Manager::~Manager() {
}

bool Com::Manager::HandleInput(const std::string &com_path,
                               const Input       &in,
                               Output            *out) {
  if (com_path.size() == 0) {
    return false;
  }
  Com::Ptr com_ptr = FindCom(com_path);
  if (not com_ptr) {
    return false;
  }
  if (not com_ptr->HandleData(in)) {
    return false;
  }
  return com_ptr->Publish(out);
}
// Com::Process ----------------------------------------------------------------
Com::Process::Process(): status(kFinished), progress(0), need_to_notify(false) {
}

Com::Process::Process(Status _status, Com::Integer _progress)
    : status(_status),
      progress(_progress),
      need_to_notify(true) {
}

Com::Process::Process(Status             _status,
                      const std::string &_message,
                      Com::Integer       _progress)
    : status(_status),
      progress(_progress),
      message(_message),
      need_to_notify(true) {
}

bool Com::Process::Publish(Output *out) {
  static std::string kStatusName[] = {
    "in work",
    "finished",
    "error"
  };
  if (progress > 100) {
    progress = 100;
  }
  if (progress > 0) {
    out->Put("progress", progress);
  }
  out->Put("status",   kStatusName[status]);
  out->Put("message",  message);
  return true;
}
// Com -------------------------------------------------------------------------
Com::Com(const std::string &name, const std::string &type)
    : _name(name),
      _type(type) {
}

Com::~Com() {
}

const std::string& Com::get_name() const {
  return _name;
}

Com& Com::RegisterAction(const std::string &name, Action::Handler handler) {
  auto act_it = _actions.find(name);
  if (act_it == _actions.end()) {
    _actions.insert(Action::Pair(name, handler));
  }
  return *this;
}

bool Com::HandleData(const Input &in) {
  const std::string kAction(in.GetString("action"));
  if (kAction.size() == 0) {
    return true;
  }
  return HandleAction(kAction, in);
}

bool Com::HandleAction(const std::string &name, const Input &in) {
  auto act_it = _actions.find(name);
  if (act_it == _actions.end()) {
    // Вызываем последнее запомненное действие "action", в случае если оно
    // установило состояние InWork. Предполагается что действие занято
    // выполнением длительного процесса.
    if (_process_state.status == Process::kInWork &&
        _process_state.action != _actions.end()) {
      act_it = _process_state.action;
      _process_state = _process_state.action->second(in, *this);
      _process_state.action = act_it;
      return true;
    }
    return (name == "sync");
  }
  _process_state        = act_it->second(in, *this);
  _process_state.action = act_it;
  return true;
}

bool Com::PublishAttributes(Output *out) {
  out->Put(Attribute::kType, _type);
  out->Put(Attribute::kName, _name);
  return true;
}

bool Com::PublishMembers(Output*) {
  return true;
}

bool Com::PublishProcessState(Output *out) {
  Output::Scoped notice(out->Create());
  const bool kPubRes = _process_state.Publish(notice.get());
  out->Append("notification", *notice);
  return kPubRes;
}

bool Com::Publish(Output *out) {
  if (out == 0) {
    return false;
  }
  bool pub_res = Com::PublishAttributes(out) &&
                 Com::PublishMembers(out);
  if (_process_state.need_to_notify) {
    _process_state.need_to_notify = false;
    pub_res = pub_res && Com::PublishProcessState(out);
    // в случае когда идёт выполнение длительного процесса, компонент "COM" не
    // публикует свои данные
    if (_process_state.status == Process::kInWork) {
      return pub_res;
    }
  }
  return pub_res && PublishAttributes(out) &&
                    PublishMembers(out);
}

Com::Ptr Com::FindCom(const std::string&) {
  return Com::Ptr();
}

} // namespace webapp
