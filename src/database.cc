/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tdb.h>

#include <stdexcept>

#include "database.hh"

struct tgrey::db_data {
    TDB_CONTEXT* ctx;
};

TDB_DATA from_string(const std::string& data) {
  TDB_DATA ret = { 0, 0 };

  if(data.length()) {
    ret.dptr = (unsigned char*) data.c_str();
    ret.dsize = data.length();
  }

  return ret;
}

tgrey::database::database(const std::string& f)
  : filename(f), data(new db_data) {
  data->ctx = 0;
}

tgrey::database::~database() {
  if(data->ctx)
    ::tdb_close(data->ctx);
}

void tgrey::database::open() {
  if(data->ctx)
    return;

  data->ctx = ::tdb_open(
     filename.c_str(), 0, TDB_DEFAULT, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

  if(!data->ctx) {
    int errnum = errno;
    throw std::runtime_error(std::string("Error opening TDB: ") +
                             std::string(strerror(errnum)));
  }
}

bool tgrey::database::fetch(const std::string& key, std::string& val) {
  if(!data->ctx)
    throw std::runtime_error("Trying to fetch from unopened TDB database.");

  TDB_DATA value = ::tdb_fetch(data->ctx, from_string(key));

  if(!value.dptr)
    return false;

  val = std::string(value.dptr, value.dptr + value.dsize);
  delete value.dptr;
  return true;
}

void tgrey::database::store(const std::string& key, const std::string& val) {
  if(!data->ctx)
    throw std::runtime_error("Trying to store to unopened TDB database.");

  if(::tdb_store(data->ctx,
                 from_string(key), from_string(val), TDB_REPLACE))
    throw std::runtime_error(std::string("Error storing to TDB: ") +
                             std::string(::tdb_errorstr(data->ctx)));
}

void tgrey::database::remove(const std::string& key) {
  if(!data->ctx)
    throw std::runtime_error("Trying to delete from unopened TDB database.");

  if(::tdb_delete(data->ctx, from_string(key)))
    throw std::runtime_error(std::string("Error deleting from TDB: ") +
                             std::string(::tdb_errorstr(data->ctx)));
}

struct traverse_callback {
    tgrey::database& db;
    tgrey::db_visitor& vi;
};

inline int
traverse_helper(TDB_CONTEXT* tdb, TDB_DATA key, TDB_DATA val, void* state) {
  traverse_callback* cb = static_cast<traverse_callback*>(state);
  return cb->vi.visit(cb->db,
                      std::string(key.dptr, key.dptr + key.dsize),
                      std::string(val.dptr, val.dptr + val.dsize));
  return 0;
}

void tgrey::database::traverse(db_visitor& visitor) {
  if(!data->ctx)
    throw std::runtime_error("Trying to traverse unopened TDB database.");

  struct traverse_callback cb = { *this, visitor };
  ::tdb_traverse(data->ctx, traverse_helper, &cb);
}
