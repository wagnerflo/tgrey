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

class cleanup_visitor : public tgrey::db_visitor {
  public:
    cleanup_visitor(unsigned int& l) : _lifetime(l), _num_removed(0) {
      /* empty */
    }

    virtual int visit(tgrey::database& db,
                      const std::string& key, const std::string& val) {
      bool cleared;
      int64_t lastseen;

      tgrey::fetch_fields(val, lastseen, cleared);

      if(tgrey::older_than(_lifetime, lastseen)) {
        db.remove(key);
        _num_removed++;
      }

      return 0;
    }

    const unsigned int& num_removed() const {
      return _num_removed;
    }

   protected:
    const unsigned int& _lifetime;
    unsigned int _num_removed;
};

int main(int argc, const char* argv[]) {
  // see if stderr is connected to a terminal; if this is not the case we
  // set the default log destination to syslog
  bool with_term = isatty(STDERR_FILENO);

  // variables with default values for the commandline options
  std::string   database   = CONFIG_TGREY_DB;
  unsigned int  lifetime   = tgrey::convert_timespan("90d");
  bool          help       = false;
  bool          log2stderr = with_term;

  propa::spec spec;
  spec.opt("database", 'D', database)
    .help("Path to use as the database for storing greylisting triplets. "
          "The user this process is run under needs read and write "
          "permissions and if it not already exists needs to be allowed "
          "to create it.");
  spec.opt("lifetime", 'l', lifetime)
    .converter(&tgrey::convert_timespan)
    .help("For any delivery where no matching mail has been seen for "
          "this long, reject and reset the triplet in any case.");
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
  cleanup_visitor vi(lifetime);

  db.open();
  db.traverse(vi);

  tgrey::log << "Cleanup removed "
             << vi.num_removed()
             << " database entries.";

  return 0;
}
