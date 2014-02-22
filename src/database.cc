/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>

#include "errors.hh"
#include "database.hh"

TDB_DATA from_string(const std::string& data) {
  TDB_DATA ret = { 0, 0 };

  if(data.length()) {
    ret.dptr = (unsigned char*) data.c_str();
    ret.dsize = data.length();
  }

  return ret;
}

tgrey::database::database(const std::string& f) : filename(f), ctx(0) {
  /* empty */
}

tgrey::database::~database() {
  if(ctx)
    tdb_close(ctx);
}

void tgrey::database::open() {
  if(ctx)
    return;

  ctx = tdb_open(
     filename.c_str(), 0, TDB_DEFAULT, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

  if(!ctx)
    throw std::runtime_error("");
}

const std::string tgrey::database::fetch (const std::string& key) const {
  if(!ctx)
    throw std::runtime_error("");

  TDB_DATA value = tdb_fetch(ctx, from_string(key));

  if(!value.dptr)
    throw tgrey::key_error(key);

  std::string ret(value.dptr, value.dptr + value.dsize);
  delete value.dptr;
  return ret;
}

const void tgrey::database::store(
                         const std::string& key, const std::string& value) {
  if(!ctx)
    throw std::runtime_error("");

  if(tdb_store(ctx, from_string(key), from_string(value), TDB_REPLACE))
    throw std::runtime_error("");
}
