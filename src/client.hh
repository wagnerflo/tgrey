/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#ifndef TGREY_CLIENT_HH
#define TGREY_CLIENT_HH

#include "policy.hh"
#include <string>
#include <stdint.h>

namespace tgrey
{
  class triplet {
    public:
      triplet(const std::string&, const std::string&, const std::string&);
      triplet(const tgrey::request&, unsigned int, unsigned int);

      operator std::string() const;

    protected:
      std::string sender;
      std::string recipient;
      std::string address;
  };

  class client_info {
    public:
      client_info();
      client_info(bool);
      client_info(const std::string&);

      bool older_than(unsigned int) const;
      operator std::string() const;

    protected:
      int64_t last_seen;

    public:
      bool passed;
  };
}

#endif /* TGREY_CLIENT_HH */
