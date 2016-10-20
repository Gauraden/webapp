/*
 * webapp_lib.hpp
 *
 *  Created on: 12 июля 2016 г.
 *      Author: denis
 */

#ifndef BACK_END_WEBAPP_LIB_HPP_
#define BACK_END_WEBAPP_LIB_HPP_

#define BOOST_THREAD_PROVIDES_FUTURE
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/utility/result_of.hpp>
#include "webapp_com_http.hpp"
#include "webapp_com_ctl.hpp"

namespace webapp {

template <class Func>
#if BOOST_VERSION > 104900
boost::future<typename boost::result_of<Func()>::type>
#else
boost::unique_future<typename boost::result_of<Func()>::type>
#endif
Async(Func f) {
  typedef typename boost::result_of<Func()>::type Result;
  boost::packaged_task<Result> pt(f);

//auto fut = pt.get_future();
#if BOOST_VERSION > 104900
  boost::future<Result> fut = pt.get_future();
#else
  boost::unique_future<Result> fut = pt.get_future();
#endif
  boost::thread( boost::move(pt) ).detach();
  boost::move(fut);
  return fut;
}

} // namespace webapp

#endif /* BACK_END_WEBAPP_LIB_HPP_ */
