/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

#include "misc.hh"

/** Convert a string consisting of numbers and time-suffixes to an
 ** interger of that timespan in seconds.
 ** ** **/
unsigned int tgrey::convert_timespan(const std::string& value) {
  std::istringstream iss(value);
  unsigned int ret = 0;
  while(iss) {
    unsigned int num = 0;
    char mod = 's';
    iss >> num >> mod;
    switch(tolower(mod)) {
      case 'y':  ret += num * 31536000;  break;
      case 'w':  ret += num * 604800;    break;
      case 'd':  ret += num * 86400;     break;
      case 'h':  ret += num * 3600;      break;
      case 'm':  ret += num * 60;        break;
      case 's':  ret += num;             break;
    }
  }
  return ret;
}

/** Return the lowercase representation of a string.
 ** ** **/
std::string tgrey::lowercase(const std::string& str) {
  std::string res(str);
  std::transform(res.begin(), res.end(), res.begin(), tolower);
  return res;
}


/**
 ** ** **/
inline void assert_char(std::istream& is, const char& c) {
  if(c != is.get())
    throw std::runtime_error("Invalid field delimiter.");
}

template<typename T> void fetch_field(std::istream& is, T& val) {
  is >> val;
}

template<> void fetch_field(std::istream& is, bool& val) {
  is >> std::boolalpha >> val >> std::noboolalpha;
}

template<typename T> void write_field(std::ostream& os, const T& val) {
  os << val;
}

template<> void write_field(std::ostream& os, const bool& val) {
  os << std::boolalpha << val << std::noboolalpha;
}

void tgrey::fetch_fields(const std::string& data,
                         int64_t& lastseen, bool& cleared) {
  std::istringstream iss(data);
  fetch_field(iss, lastseen);
  assert_char(iss, field_separator);
  fetch_field(iss, cleared);
}

const std::string tgrey::join_fields(const int64_t& lastseen,
                                     const bool& cleared) {
  std::ostringstream oss;
  write_field(oss, lastseen);
  write_field(oss, field_separator);
  write_field(oss, cleared);
  return oss.str();
}

bool tgrey::older_than(const unsigned int& val, const int64_t& lastseen) {
  return lastseen < ::time(0) - val;
}
