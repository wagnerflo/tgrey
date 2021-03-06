/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#ifndef TGREY_LOGGING_HH
#define TGREY_LOGGING_HH

#include "ext/slo.hh"

namespace tgrey
{
  extern slo::logger log;
  slo::stage::ptr syslog_stage();
}

#endif /* TGREY_LOGGING_HH */
