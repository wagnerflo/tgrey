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

#include "client.hh"
#include "cmdline.hh"
#include "errors.hh"
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
  unsigned int  delay      = 1440;
  unsigned int  timeout    = 10080;
  unsigned int  lifetime   = 21900;
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
    tgrey::log.msg_level(slo::crit);
    tgrey::log << "Error parsing commandline.";
    return 1;
  }

  if(help) {
    usage(std::cout, spec, argv);
    return 0;
  }

  // set up logging
  tgrey::log.add_pipe(slo::min_level(slo::info) | tgrey::syslog_stage);

  // create a database object; this will not try to open it
  tgrey::database db(database);

  // run in an infinite loop until either stdin gets closed or an signal
  while(std::cin) {
    try {
      // try to parse the request
      const tgrey::request req(std::cin);

      // verify that we got an request of appropriate type
      try {
        if(req["request"] != "smtpd_access_policy")
          throw std::runtime_error("request != smtpd_access_policy");
      }
      // if there is not even a request attribute do not bother to answer
      // since our reply would probably not be understood
      catch(const tgrey::key_error& err) {
        continue;
      }

      // turn request into triplet
      const tgrey::triplet tri(req, v4mask, v6mask);

      // this is an noop if the database is already open, otherwise it
      // tries to open it; might throw
      db.open();

      // turn the triplet into a string to be used as a key for db access
      std::string key(tri);

      // try to get data associated with triplet from database
      try {
        const tgrey::client_info info(db.fetch(key));

        // if the entry has expired reset it by doing as if it didn't
        // exist; an entry is expired if either it is older than lifetime
        // or otherwise if it is older than timeout and at the same time
        // has not been allowed to pass
        if(info.older_than(lifetime) ||
           (info.older_than(timeout) && !info.passed))
          throw tgrey::key_error(key);

        // if allowed to pass mail or last try was longer than delay ago,
        // then update last_seen in database and allow mail through
        if(info.passed || info.older_than(delay)) {
          db.store(key, tgrey::client_info(true));
          std::cout << tgrey::response::dunno;
        }
        // not yet allowed to pass mails; do not update last_seen
        else {
          std::cout << tgrey::response::service_unavailable;
        }
      }
      // no entry in the database: make a new one and return the
      // service unavailable message
      catch(const tgrey::key_error& err) {
        db.store(key, tgrey::client_info());
        std::cout << tgrey::response::service_unavailable;
      }
    }
    // if there was any kind of unexpected error, make sure this does
    // not impact mail delivery by answering with dunno
    catch(const std::exception& err) {
      std::cerr << err.what() << std::endl;
      std::cout << tgrey::response::dunno;
    }
    catch(...) {
      std::cout << tgrey::response::dunno;
    }
  }

  return 0;
}
