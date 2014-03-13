/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#ifndef TGREY_POLICY_HH
#define TGREY_POLICY_HH

#include <istream>
#include <string>

namespace tgrey
{
  class policy_request {
    public:
      policy_request(std::istream&);
      const std::string to_key(const std::string&,
                               const unsigned int,
                               const unsigned int) const;
      const std::string to_key(const unsigned int,
                               const unsigned int) const;

    protected:
      std::string sender;
      std::string recipient;
      std::string client_name;
      std::string client_address;
  };

  class policy_response {
    public:
      static const policy_response dunno;
      static const policy_response service_unavailable;

      const std::string action;
      const std::string text;

      policy_response(const std::string&, const std::string&);
      policy_response(const std::string&);
  };

  std::ostream& operator<< (std::ostream&, const policy_response&);
}

#endif /* TGREY_POLICY_HH */
