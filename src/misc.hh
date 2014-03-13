/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#ifndef TGREY_MISC_HH
#define TGREY_MISC_HH

#include <string>

namespace tgrey
{
  const char field_separator = 31;

  unsigned int convert_timespan(const std::string&);
  std::string lowercase(const std::string&);
  void fetch_fields(const std::string&, int64_t&, bool&);
  const std::string join_fields(const int64_t&, const bool&);
  bool older_than(const unsigned int&, const int64_t&);
}

#endif /* TGREY_MISC_HH */
