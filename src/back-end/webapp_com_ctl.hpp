/*
 * webapp_com_ctl.hpp
 *
 *  Created on: 8 сент. 2016 г.
 *      Author: denis
 */

#ifndef BACK_END_WEBAPP_COM_CTL_HPP_
#define BACK_END_WEBAPP_COM_CTL_HPP_

#include "webapp_com.hpp"
#include "boost/filesystem.hpp"
#include "boost/regex.hpp"

namespace webapp {
namespace ctl    {

class FileDialog : public Com {
  public:
    typedef Process (*Handler)(const std::string &file);

    struct Action {
      static const std::string kGoTo;
      static const std::string kOpen;
    };

    FileDialog(const std::string &name);
    FileDialog& SetRoot(const std::string &path);
    FileDialog& SetMask(const std::string &expr);
    Process GoTo(const std::string &name);
    Process Open(const std::string &name);
    FileDialog& SetOpenHandler(Handler hndl);
  private:
    virtual bool PublishAttributes(Output *out);
    bool CheckName(const std::string &name);

    boost::filesystem::path _root_dir;
    boost::filesystem::path _cur_dir;
    boost::regex            _file_mask;
    std::stringstream       _error_log;
    Handler                 _open_handler;
    std::string             _opening_file;
};

class DataIFace {
  public:
    typedef boost::shared_ptr<DataIFace> Ptr;

    DataIFace();
    virtual ~DataIFace();
    virtual void PublishData(Com::Output *out) = 0;
    virtual void PublishMetaData(Com::Output *out) = 0;
};

class Table : public Com {
  public:
    Table(const std::string &name);
    void SetDataInterface(DataIFace::Ptr data_if);
  private:
    virtual bool PublishAttributes(Output *out);

    DataIFace::Ptr _data_if;
};

}} // namespace webapp::ctl

#endif /* BACK_END_WEBAPP_COM_CTL_HPP_ */
