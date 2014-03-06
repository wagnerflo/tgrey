/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include <arpa/inet.h>
#include <netdb.h>
#include <stdexcept>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "errors.hh"
#include "client.hh"

#define SEP '/'

inline void assert_seperator(std::istream& is) {
  char c = is.get();
  if(c != SEP)
    throw tgrey::conversion_error(
      std::string("Expected separator '") + SEP + "' but got '" + c + "'.");
}

inline const std::string mask_addr(
          const std::string& ip, unsigned int v4mask, unsigned int v6mask) {
  struct addrinfo hints;
  struct addrinfo* result = 0;

  // restrict getaddrinfo so it returns at most one result
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_NUMERICHOST;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // validate the string representation and detect whether it is v4 or v6
  if(getaddrinfo(ip.c_str(), 0, &hints, &result) || !result)
    throw tgrey::conversion_error(
                            "The given address '" + ip + "' is not valid.");

  int af = result->ai_family;
  freeaddrinfo(result);

  // allocate space for the byte representation and convert from string
  std::vector<unsigned char> addr(
       af == AF_INET ? sizeof(struct in_addr) : sizeof(struct in6_addr), 0);
  inet_pton(af, ip.c_str(), &addr[0]);

  // depending on address type use correct mask
  unsigned int mask = af == AF_INET ? v4mask : v6mask;

  // modify the (mask/8)th byte and set all after that to zero
  addr[mask/8] &= 256 - (1 << (8 - (mask % 8)));
  std::fill(addr.begin() + mask/8, addr.end(), 0);

  // convert bytes back to string
  std::ostringstream oss;

  for(std::vector<unsigned char>::const_iterator it = addr.begin();
      it != addr.end(); ++it) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<unsigned int>(*it);
  }

  return oss.str();
}

inline const std::string mask_name(const std::string& name) {
  size_t pos;

  // first we search for the last occurence of a dot
  pos = name.rfind('.');

  // if there is none or one at the very start, return the name unchanged
  if(pos == std::string::npos || !pos)
    return name;

  // now go for the previous to last occurence
  pos = name.rfind('.', pos - 1);

  // same sanity check again
  if(pos == std::string::npos || !pos)
    return name;

  // check if the substring after the second to last dot is 7 characters
  // or less in length; this is a heuristic for detecting ccSLDs and
  // prompts us to look for the third to last dot (in case mail comes from
  // mail.example.co.uk we want to use example.co.uk and not just co.uk
  // in the triplet)
  if(name.length() - pos <= 7)
    pos = name.rfind('.', pos - 1);

  // and same santiy check again
  if(pos == std::string::npos || !pos)
    return name;

  // finally return the substring
  return name.substr(pos + 1);
}

inline std::string lowercase(const std::string& str) {
  std::string res(str);
  std::transform(res.begin(), res.end(), res.begin(), tolower);
  return res;
}

tgrey::triplet::triplet(
           const std::string& s, const std::string& r, const std::string& a)
  : sender(lowercase(s)), recipient(lowercase(r)), address(lowercase(a)) {
  /* empty */
}

#define CONVERSION_ERROR_MISSING_ATTRIBUTE(attr) \
  tgrey::conversion_error( \
                    "Required attribute '" + attr + "' missing in request.")

tgrey::triplet::triplet(
      const tgrey::request& req, unsigned int v4mask, unsigned int v6mask) {
  try {
    // envelope sender and recipient
    sender = lowercase(req["sender"]);
    recipient = lowercase(req["recipient"]);
  }
  catch(const tgrey::key_error& err) {
    throw CONVERSION_ERROR_MISSING_ATTRIBUTE(err.key);
  }

  // client_name is available and not unknown
  try {
    if((address = lowercase(mask_name(req["client_name"]))) == "unknown")
      throw tgrey::key_error();
  }
  // as a fallback try to use the clients IP address; Postfix might
  // have set this to unknown, in which case mask_addr will throw
  catch(const tgrey::key_error& err) {
    try {
      address = mask_addr(req["client_address"], v4mask, v6mask);
    }
    catch(const tgrey::key_error& err) {
      throw CONVERSION_ERROR_MISSING_ATTRIBUTE(err.key);
    }
  }
}

tgrey::triplet::operator std::string() const {
  if(sender.empty() || recipient.empty() || address.empty())
    return std::string();

  return sender + SEP + recipient + SEP + address;
}

tgrey::client_info::client_info()
  : last_seen(time(0)), passed(false) {
  if(last_seen == -1)
    throw std::runtime_error("");
}

tgrey::client_info::client_info(bool p)
  : last_seen(time(0)), passed(p) {
  if(last_seen == -1)
    throw std::runtime_error("");
}

tgrey::client_info::client_info(const std::string& data)
  : last_seen(-1), passed(false) {
  std::istringstream iss(data);
  iss >> last_seen;
  assert_seperator(iss);
  iss >> std::boolalpha >> passed >> std::noboolalpha;
}

bool tgrey::client_info::older_than(unsigned int val) const {
  return last_seen < time(0) - val;
}

tgrey::client_info::operator std::string() const {
  std::ostringstream oss;
  oss << last_seen
      << SEP
      << std::boolalpha << passed << std::noboolalpha;
  return oss.str();
}
