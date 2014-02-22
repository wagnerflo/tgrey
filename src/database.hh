/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#ifndef TGREY_DATABASE_HH
#define TGREY_DATABASE_HH

#include <string>
#include <vector>
#include <sys/types.h>
#include <tdb.h>

namespace tgrey
{
  class database {
    public:
      database(const std::string&);
      ~database();

      void open();
      const std::string fetch (const std::string&) const;
      const void store(const std::string&, const std::string&);

    protected:
      const std::string filename;
      ::tdb_context* ctx;
  };
}

#endif /* TGREY_DATABASE_HH */
