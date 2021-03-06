/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include <iostream>
#include "policy.hh"

int main() {
  std::cout
    << tgrey::policy_request(std::cin).to_key(24, 66)
    << std::endl;
  return 0;
}
