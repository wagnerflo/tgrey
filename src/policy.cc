/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include <algorithm>
#include <cctype>
#include <functional>

#include "policy.hh"
#include "errors.hh"

const tgrey::response tgrey::response::dunno("dunno");
const tgrey::response tgrey::response::service_unavailable(
                                 "defer_if_permit", "Service is unavailable");

tgrey::request::request(std::istream& inp) {
  for(std::string line; std::getline(inp, line) && !line.empty(); ) {
    size_t pos = line.find('=');

    if(pos == std::string::npos)
      continue;

    const std::string& value = line.substr(pos + 1);

    if(std::find_if(value.begin(), value.end(),
               std::not1(std::ptr_fun<int,int>(std::isspace))) == value.end())
      continue;

    attrs.insert(attrpair(line.substr(0, pos), value));
  }
}

const std::string&
tgrey::request::operator[] (const std::string& name) const {
  attrmap::const_iterator it = attrs.find(name);

  if(it == attrs.end())
    throw tgrey::key_error(name);

  return it->second;
}

tgrey::response::response(const std::string& a, const std::string& t)
  : action(a), text(t) {
  /* empty */
}

tgrey::response::response(const std::string& a)
  : action(a) {
  /* empty */
}

std::ostream& tgrey::operator<< (std::ostream& out,
                                 const tgrey::response& res) {
  out << "action=" << res.action;

  if(!res.text.empty())
    out << " " << res.text;

  out << std::endl << std::endl;
  return out;
}
