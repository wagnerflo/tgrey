/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include <unistd.h>
#include <iostream>

#include "ext/slo.hh"
#include "ext/propa.hh"

#include "misc.hh"
#include "database.hh"
#include "logging.hh"
#include "policy.hh"

slo::logger tgrey::log;

void usage(std::ostream& os, const propa::spec& spec, const char* argv[]) {
  spec.usage(os, argv);

  os << std::endl
     << "An implementation of greylisting for the Postfix access policy "
     << "protocol" << std::endl << "using the trivial database system (tdb) "
     << "as back-end." << std::endl
     << std::endl;

  spec.options(os);

  os << std::endl
     << "This binary represents version " << PACKAGE_VERSION << " of the "
     << "package. Copyright (c) 2014," << std::endl << "Florian Wagner. "
     << "Feel free to contact me at florian@wagner-flo.net with" << std::endl
     << "comments and bug reports." << std::endl
     << std::endl;
}

int main(int argc, const char* argv[]) {
  // see if stderr is connected to a terminal; if this is not the case we
  // set the default log destination to syslog
  bool with_term = isatty(STDERR_FILENO);

  // variables with default values for the commandline options
  std::string   database   = CONFIG_TGREY_DB;
  unsigned int  delay      = tgrey::convert_timespan("5m");
  unsigned int  timeout    = tgrey::convert_timespan("7d");
  unsigned int  lifetime   = tgrey::convert_timespan("90d");
  unsigned int  v4mask     = 32;
  unsigned int  v6mask     = 128;
  bool          help       = false;
  bool          log2stderr = with_term;

  propa::spec spec;
  spec.opt("database", 'D', database)
    .help("Path to use as the database for storing greylisting triplets. "
          "The user this process is run under needs read and write "
          "permissions and if it not already exists needs to be allowed "
          "to create it.");
  spec.opt("delay", 'd', delay)
    .converter(&tgrey::convert_timespan)
    .help("Delta between the time a triplet is first recorded and mail "
          "for it rejected and the time the first retry message for it "
          "is allowed through.");
  spec.opt("timeout", 't', timeout)
    .converter(&tgrey::convert_timespan)
    .help("Any delivery made for triplets, which are older than this "
          "value but at the same time not cleared for delivery yet "
          "is rejected and the triplet reset.");
  spec.opt("lifetime", 'l', lifetime)
    .converter(&tgrey::convert_timespan)
    .help("For any delivery where no matching mail has been seen for "
          "this long, reject and reset the triplet in any case.");
  spec.opt("v4mask", '4', v4mask)
    .help("Prefix size for masking any IPv4 addresses used for "
          "building the triplet. This will group together all delivery "
          "agents coming from the subnet.");
  spec.opt("v6mask", '6', v6mask)
    .help("Same as --v4mask but for IPv6 addresses.");
  spec.flag("log-to-stderr", 'e', log2stderr)
    .help("Force log output to go to standard error even if that is not "
          "connected to a controlling terminal.");
  spec.flag("help", 'h', help)
    .help("Display this text and exit.");

  // parse the commandline and handle any parse errors
  try {
    spec.parse(argc, argv);
  }
  catch(...) {
    // at this point we do not know if the log-to-stderr flag has even
    // been parsed at all so we use with_term to select the destination
    tgrey::log.add_pipe(with_term ? slo::stderr : tgrey::syslog_stage);
    tgrey::log << slo::crit << "Error parsing commandline.";
    return 1;
  }

  if(help) {
    usage(std::cout, spec, argv);
    return 0;
  }

  // set up logging
  tgrey::log.msg_level(slo::info);
  tgrey::log.add_pipe(
      slo::min_level(slo::info) |
      (log2stderr ? slo::stderr : tgrey::syslog_stage));

  // create a database object; this will not try to open it
  tgrey::database db(database);

  // run in an infinite loop until either stdin gets closed or an signal
  while(std::cin) {
    try {
      // try to parse the request
      const tgrey::policy_request req(std::cin);

      // this is an noop if the database is already open, otherwise it
      // tries to open it; might throw
      db.open();

      bool exists, cleared;
      std::string key, val;
      int64_t lastseen;

      // try to get data associated with triplet from database
      key = req.to_key(v4mask, v6mask);
      exists = db.fetch(key, val);

      // parse database entry
      if(exists)
        tgrey::fetch_fields(val, lastseen, cleared);

      // create a fresh database entry if:
      //  - either there is none yet
      //  - or if the existing one is expired, meaning that it is:
      //    + either older than lifetime
      //    + or older than timeout and has not yet been cleared
      if(   (!exists)
         || (tgrey::older_than(lifetime, lastseen))
         || (tgrey::older_than(timeout, lastseen) && !cleared)) {
        db.store(key, tgrey::join_fields(::time(0), false));
        tgrey::log << "new: " << req.to_key(" / ", v4mask, v6mask);
        std::cout << tgrey::policy_response::service_unavailable;
      }

      // set database entry to cleared and update lastseen if:
      //  - either entry is cleared already
      //  - or the last delivery attempt was longer than delay ago
      else if(   cleared
              || tgrey::older_than(delay, lastseen)) {
        // db.store(key, tgrey::client_info(true));
        tgrey::log << "ok: " << req.to_key(" / ", v4mask, v6mask);
        std::cout << tgrey::policy_response::dunno;
      }

      // do not allow to pass and don't change database otherwise
      else {
        tgrey::log << "wait: " << req.to_key(" / ", v4mask, v6mask);
        std::cout << tgrey::policy_response::service_unavailable;
      }
    }
    // if there was any kind of unexpected error, make sure this does
    // not impact mail delivery by answering with dunno
    catch(const std::exception& err) {
      tgrey::log << slo::error << err.what();
      std::cout << tgrey::policy_response::dunno;
    }
    catch(...) {
      tgrey::log << slo::error << "Unknown error.";
      std::cout << tgrey::policy_response::dunno;
    }
  }

  return 0;
}
