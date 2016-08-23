/*
 * enum_serializer.hpp
 *
 *  Created on: 12 авг. 2016 г.
 *      Author: denis
 */

#ifndef BACK_END_ENUM_SERIALIZER_HPP_
#define BACK_END_ENUM_SERIALIZER_HPP_

#include <string>
#include <map>

namespace enum_serializer {

template <typename BinaryType>
struct Collection {
  typedef std::map<std::string, BinaryType>  Map;
  typedef std::pair<std::string, BinaryType> Rec;
  typedef void (*Founder)(Collection<BinaryType>&);

  Collection(Founder handler) {
    handler(*this);
  }

  void Add(const std::string &name, BinaryType value) {
    map.insert(Rec(name, value));
  }

  bool Find(const std::string &name, BinaryType *out) const {
    if (out == 0) {
      return false;
    }
    typename Map::const_iterator it = map.find(name);
    if (it != map.end()) {
      *out = it->second;
      return true;
    }
    return false;
  }

  std::string Find(BinaryType out) const {
    typename Map::const_iterator it = map.begin();
    for (; it != map.end(); it++) {
      if (it->second == out) {
        return it->first;
      }
    }
    static const std::string kEmptyStr;
    return kEmptyStr;
  }

  Map map;
};

} // enum_serializer

#endif /* BACK_END_ENUM_SERIALIZER_HPP_ */
