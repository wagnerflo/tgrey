/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include <sstream>
#include <string>
#include "cmdline.hh"

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
