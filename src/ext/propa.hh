/* * *
  This file constitutes the propa software package: A program options
  parser for the C++ language.

  Copyright (c) 2012, Florian Wagner <florian@wagner-flo.net>.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * * */

#ifndef PROPA_HH
#define PROPA_HH

#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iomanip>

namespace propa
{
  class spec;

  namespace propa_detail
  {
    template<class T> inline T convert(const std::string& value) {
      T ret;
      std::istringstream iss(value);
      iss >> ret;
      return ret;
    }

    template<> inline std::string convert(const std::string& value) {
      return value;
    }

    class optbase {
      friend class propa::spec;

      public:
        virtual ~optbase() { /* empty */ };
        virtual bool expects_value() const { return true; }
        virtual std::string help() const = 0;

      private:
        virtual void set(const std::string&) = 0;
    };

    typedef std::pair<std::string,char> optkey;
    typedef std::pair<optkey,optbase*> optitem;
    typedef std::vector<optitem> optlist;
    typedef std::vector<optbase*> arglist;

    inline optlist::const_iterator
    find(const optlist& lst, const std::string& key) {
      if(key.empty())
        return lst.end();

      for(optlist::const_iterator it = lst.begin(); it != lst.end(); ++it)
        if(it->first.first == key)
          return it;

      return lst.end();
    }

    inline optlist::const_iterator
    find(const optlist& lst, const char key) {
      if(key == 0)
        return lst.end();

      for(optlist::const_iterator it = lst.begin(); it != lst.end(); ++it)
        if(it->first.second == key)
          return it;

      return lst.end();
    }

    std::string format_key(const optkey& key, const optbase* item) {
      std::stringstream ss;
      ss << "  ";
      if(key.second != 0) {
        ss << "-" << key.second;
        if(!key.first.empty())
          ss << ", ";
      }
      if(!key.first.empty()) {
        ss << "--" << key.first;
        if(item->expects_value())
          ss << "=VALUE";
      }
      else if(item->expects_value())
        ss << " VALUE";
      return ss.str();
    }

    std::string format_help(const optbase* item, size_t first_column) {
      std::string word;
      std::stringstream in(item->help());
      std::stringstream out;
      size_t at = first_column;

      while(std::getline(in, word, ' ')) {
        if(word.empty())
          continue;

        if(at + word.length() + 1 > 72) {
          out << std::endl;
          for(unsigned pad = first_column; pad > 0; --pad)
            out.put(' ');
          at = first_column;
        }

        out << " " << word;
        at += word.length() + 1;
      }

      return out.str();
    }
  }

  template<class T> class validator {
    public:
      virtual bool accepts(const T&) const { return true; }
      virtual ~validator() { /* empty */ }
  };

  template<class T, class U> class function_validator : public validator<T> {
    protected:
      const U _func;

    public:
      function_validator(const U& f) : _func(f) { /* empty */ }
      virtual bool accepts(const T& value) const { return _func(value); }
  };

  template<class T> class valueopt : public propa_detail::optbase {
    protected:
      T& _ref;
      std::string _help;
      T (*_conv) (const std::string&);
      std::vector<validator<T>*> _validators;

    private:
      const T& validate(const T& value) const {
        for(typename std::vector<validator<T>*>::const_iterator it =
              _validators.begin(); it != _validators.end(); ++it)
          if(!(*it)->accepts(value))
            throw std::runtime_error("value not valid");

        return value;
      }

      virtual void set(const std::string& v) { _ref = validate(_conv(v)); }

    public:
      valueopt(T& ref) : _ref(ref), _conv(&propa_detail::convert<T>) {
        /* empty */
      }
      virtual bool expects_value() const {  return true; }
      virtual std::string help() const { return _help; }

      virtual ~valueopt() {
        for(typename std::vector<validator<T>*>::const_iterator it =
              _validators.begin(); it != _validators.end(); ++it)
          delete *it;
      }

      valueopt<T>& help(const std::string& h) {
        _help = h; return *this;
      }

      valueopt<T>& converter(T (*conv) (const std::string&)) {
        _conv = conv; return *this;
      }

      valueopt<T>& range(const T& min, const T& max) {
        return greater_equal(min).less_equal(max);
      }

      valueopt<T>& greater(const T& min) {
        _validators.push_back(
          new function_validator<T,std::binder2nd<std::greater<T> > >(
            std::binder2nd<std::greater<T> >(std::greater<T>(), min)));
        return *this;
      }

      valueopt<T>& greater_equal(const T& min) {
        _validators.push_back(
          new function_validator<T,std::binder2nd<std::greater_equal<T> > >(
            std::binder2nd<std::greater_equal<T> >(
              std::greater_equal<T>(), min)));
        return *this;
      }

      valueopt<T>& less(const T& max) {
        _validators.push_back(
          new function_validator<T,std::binder2nd<std::less<T> > >(
            std::binder2nd<std::less<T> >(std::less<T>(), max)));
        return *this;
      }

      valueopt<T>& less_equal(const T& max) {
        _validators.push_back(
          new function_validator<T,std::binder2nd<std::less_equal<T> > >(
            std::binder2nd<std::less_equal<T> >(std::less_equal<T>(), max)));
        return *this;
      }
  };

  template<class T> class flagopt : public propa_detail::optbase {
    protected:
      T& _ref;
      T _max;
      std::string _help;

    private:
      void _set(bool const*) { _ref = true; }
      void _set(...)         { _ref++; }

      virtual void set(const std::string& value) {
        if(_max && _ref >= _max)
          throw std::runtime_error("too often");

        _set((T const*) 0);
      }

    public:
      flagopt(T& ref) : _ref(ref), _max(0) { /* empty */ }
      flagopt<T>& max(const T& max) { _max = max; return *this; }
      virtual bool expects_value() const { return false; }
      virtual std::string help() const { return _help; }

      flagopt<T>& help(const std::string& h) {
        _help = h;
        return *this;
      }
  };

  class spec {
    protected:
      propa_detail::optlist _opts;
      propa_detail::arglist _args;

      template<class T> T* opt(const std::string& l, const char s, T* v) {
        if(find(_opts, l) != _opts.end() || find(_opts, s) != _opts.end())
          throw std::runtime_error("duplicate");

        _opts.push_back(std::make_pair(std::make_pair(l, s), v));
        return v;
      }

    public:
      void parse(int argc, const char* argv[]) const {
        parse(std::vector<std::string>(argv, argv + argc));
      }

      void parse(const std::vector<std::string>& items) const {
        std::vector<std::string>::const_iterator it = items.begin() + 1;
        std::string key,value;

        // first parse all options
        for(; it != items.end(); ++it) {
          bool shortopt = false;
          std::string item = *it;

          // detect the end-of-options delimiter
          if(item == "--")
            break;

          switch(size_t pos = item.find_first_not_of('-')) {
            // 1 or 2 hyphens at the start make an option
            case 1:
            case 2:
              // a key expecting a value, may not be followed by another
              // key immediately
              if(!key.empty())
                throw std::runtime_error("two keys in a row");

              item = item.substr(pos);

              switch(pos) {
                case 1:
                  shortopt = true;
                  key = item;
                  break;

                case 2:
                  if(item.length() == 1)
                    throw std::runtime_error("long option too short");

                  // extract the key from the item; takes into account the
                  // possibility of a key=value form
                  pos = item.find('=');
                  key = item.substr(0, pos);
                  if(pos != std::string::npos)
                    value = item.substr(pos + 1);

                  break;
              }
              break;

            // anything else is a value
            default:
              value = item;
              break;
          }

          // no key but an value, means we have the second value in a
          // row: start parsing arguments
          if(key.empty() && !value.empty())
            break;

          propa_detail::optlist::const_iterator opt;

          // handle short options with multiple flags in out argument
          if(shortopt && key.length() > 1) {
            std::string::const_iterator it;

            // all but the last option may only be flags
            for(it = key.begin(); it != key.end() - 1; ++it) {
              if((opt = propa_detail::find(_opts, *it)) == _opts.end())
                 throw std::runtime_error("bad option");

              if(opt->second->expects_value())
                throw std::runtime_error(
                            "non-flag at not-end of multi-char short option");

              opt->second->set(value);
            }

            // leave the last char as the key to be handled as normal
            key = *it++;
          }

          // select the matching option
          opt = key.length() == 1 ? propa_detail::find(_opts, key[0]) :
            propa_detail::find(_opts, key);

          if(opt == _opts.end())
            throw std::runtime_error("bad option");

          // if the option expects an argument and value is not yet set
          // we need to wait for the next iteration to decide what to do
          if(opt->second->expects_value() && value.empty())
            continue;

          opt->second->set(value);
          key.clear();
          value.clear();
        }

        // then continue with arguments
        for(propa_detail::arglist::const_iterator arg = _args.begin();
            arg != _args.end(); ++arg) {
          if(it == items.end())
            throw std::runtime_error("not enough arguments");

          (*arg)->set(*it++);
        }

        if(it != items.end())
           throw std::runtime_error("too many arguments");
      };

      ~spec() {
        for(propa_detail::optlist::iterator it = _opts.begin();
            it != _opts.end(); ++it)
          delete it->second;

        for(propa_detail::arglist::iterator it = _args.begin();
            it != _args.end(); ++it)
          delete *it;
      }

      void usage(std::ostream& os, const std::string& prog) const {
        os << "Usage: " << prog << std::endl;
      }

      void usage(std::ostream& os, const char* argv[]) const {
        usage(os, argv[0]);
      }

      void options(std::ostream& os) const {
        os << "Options:" << std::endl;

        size_t width = 23;
        for(propa_detail::optlist::const_iterator it = _opts.begin();
            it != _opts.end(); ++it)
          width = std::max(
                   propa_detail::format_key(it->first, it->second).length(),
                   width);

        ++width;

        for(propa_detail::optlist::const_iterator it = _opts.begin();
            it != _opts.end(); ++it) {
          os << std::left << std::setw(width)
             << propa_detail::format_key(it->first, it->second)
             << propa_detail::format_help(it->second, width)
             << std::endl;
        }
      }

      template<class T>
      valueopt<T>& opt(const std::string& l, T& ref) {
        return *opt(l, 0, new valueopt<T>(ref));
      }

      template<class T>
      valueopt<T>& opt(const char s, T& ref) {
        return *opt("", s, new valueopt<T>(ref));
      }

      template<class T>
      valueopt<T>& opt(const std::string& l, const char s, T& ref) {
        return *opt(l, s,  new valueopt<T>(ref));
      }

      template<class T>
      flagopt<T>& flag(const std::string& l, T& ref) {
        return *opt(l, 0, new flagopt<T>(ref));
      }

      template<class T>
      flagopt<T>& flag(const char s, T& ref) {
        return *opt("", s, new flagopt<T>(ref));
      }

      template<class T>
      flagopt<T>& flag(const std::string& l, const char s, T& ref) {
        return *opt(l, s,  new flagopt<T>(ref));
      }
  };
}

#endif /* PROPA_HH */
