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

template <class Result>
struct Task {
#if BOOST_VERSION > 104900
  typedef boost::future<Result> Future;
#else
  typedef boost::unique_future<Result> Future;
#endif
  typedef Result (*Handler)();

  static
  Future Async(Handler handler) {
    boost::packaged_task<Result> pt(handler);
    Future fut = pt.get_future();
    boost::thread( boost::move(pt) ).detach();
    boost::move(fut);
    return fut;
  }
};

} // namespace webapp

#endif /* BACK_END_WEBAPP_LIB_HPP_ */
