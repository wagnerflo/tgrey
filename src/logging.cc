/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include "logging.hh"

slo::stage::ptr tgrey::syslog_stage() {
  return slo::syslog("tgreylist", LOG_DAEMON);
}
