/* * *
  This file is part of the tgrey software package.

  Copyright (c) 2014, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  The simplified (2-clause) BSD license applies. See also the
  included file COPYING.
 * * */

#include <iomanip>
#include <istream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "policy.hh"
#include "misc.hh"

/** Forward declare helper functions.
 ** ** **/
const std::string mask_name(const std::string&);
const std::string mask_addr(const std::string&, unsigned int, unsigned int);

/** Construct policy request by parsing from a text stream. Extracts some
 ** fields by implementing the abstract protocol (one key=value pair per
 ** line, empty line ends request) used by the Postfix policy delegation.
 ** ** **/
tgrey::policy_request::policy_request(std::istream& inp) {
  std::string request;

  for(std::string line; std::getline(inp, line) && !line.empty(); ) {
    size_t pos = line.find('=');

    // ignore lines without an equal sign
    if(pos == std::string::npos)
      continue;

    // insert key value pair
    std::string key = lowercase(line.substr(0, pos));
    std::string val = lowercase(line.substr(pos + 1));

    if(key == "request")
      request = val;

    if(key == "sender")
      sender = val;

    if(key == "recipient")
      recipient = val;

    if(val != "unknown") {
      if(key == "client_name")
        client_name = val;

      if(key == "client_address")
        client_address = val;
    }
  }

  if(request != "smtpd_access_policy")
    throw std::runtime_error("Policy request is not smtpd_access_policy.");

  if(sender.empty())
    throw std::runtime_error("Policy request missing sender.");

  if(recipient.empty())
    throw std::runtime_error("Policy request missing recipient.");

  if(client_name.empty() && client_address.empty())
    throw std::runtime_error(
              "Policy request missing known client_name and client_address.");
}

const std::string
tgrey::policy_request::to_key(const unsigned int v4mask,
                              const unsigned int v6mask) const {
  return to_key(std::string() + field_separator, v4mask, v6mask);
}

const std::string
tgrey::policy_request::to_key(const std::string& delim,
                              const unsigned int v4mask,
                              const unsigned int v6mask) const {
  std::ostringstream oss;

  oss << sender << delim << recipient << delim;

  if(!client_name.empty())
    oss << mask_name(client_name);

  else
    oss << mask_addr(client_address, v4mask, v6mask);

  return oss.str();
}

/** Simple constructors for policy response objects. These are created
 ** with an action string and an optional textual description.
 ** */
tgrey::policy_response::policy_response(const std::string& a,
                                        const std::string& t)
  : action(a), text(t) {
  /* empty */
}

tgrey::policy_response::policy_response(const std::string& a)
  : action(a) {
  /* empty */
}

/** Implement the bitwise left shift operator to be able to easily write
 ** a vaild serialization (understood by Postfix) of an policy response
 ** object to a stream.
 ** ** **/
std::ostream& tgrey::operator<< (std::ostream& out,
                                 const tgrey::policy_response& res) {
  out << "action=" << res.action;

  if(!res.text.empty())
    out << " " << res.text;

  out << std::endl << std::endl;
  return out;
}

/** Predefined policy responses used by the protocol.
 ** ** **/
const tgrey::policy_response tgrey::policy_response::dunno("dunno");
const tgrey::policy_response tgrey::policy_response::service_unavailable(
                                 "defer_if_permit", "Service is unavailable");

/** Helper function to mask bits of an IPv4 or IPv6 address and return
 ** an string representation of it.
 ** ** **/
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
    throw std::runtime_error("Not a valid IP address: " + ip + ".");

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
