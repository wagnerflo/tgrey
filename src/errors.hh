/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#ifndef TGREY_ERRORS_HH
#define TGREY_ERRORS_HH

#include <stdexcept>

namespace tgrey
{
  class key_error : public std::runtime_error {
    public:
      const std::string key;

      key_error(const std::string& k) throw()
        : std::runtime_error("Could not find '" + k + "'."), key(k) {
        /* empty */
      }

      key_error() throw()
        : std::runtime_error("Could not find some unspecified key.") {
        /* empty */
      }

      ~key_error() throw() {
        /* empty */
      }
  };

  class conversion_error : public std::runtime_error {
    public:
      conversion_error(const std::string& msg)
        : std::runtime_error(msg) {
        /* empty */
      }
  };
}

#endif /* TGREY_ERRORS_HH */
