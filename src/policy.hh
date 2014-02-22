/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#ifndef TGREY_POLICY_HH
#define TGREY_POLICY_HH

#include <string>
#include <map>
#include <istream>

namespace tgrey
{
  class request {
    public:
      request(std::istream&);
      const std::string& operator[] (const std::string&) const;

    protected:
      typedef std::map<const std::string,const std::string> attrmap;
      typedef std::pair<const std::string,const std::string> attrpair;

      attrmap attrs;
  };

  class response {
    public:
      static const response dunno;
      static const response service_unavailable;

      const std::string action;
      const std::string text;

      response(const std::string&, const std::string&);
      response(const std::string&);
  };

  std::ostream& operator<< (std::ostream&, const response&);
}

#endif /* TGREY_POLICY_HH */
