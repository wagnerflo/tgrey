/* * *
  This file constitutes the slo software package: A stream based
  message logger.

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

#ifndef SLO_HH
#define SLO_HH

#include <syslog.h>
#include <memory>
#include <vector>
#include <sstream>
#include <iostream>

namespace slo
{
  class logger;
  class stage;

  class level {
    protected:
      unsigned int _numeric;

    public:
      level(unsigned int n) : _numeric(n) { /* empty */ }
      operator unsigned int() const { return _numeric; }
      bool operator>  (const level& (&fun)()) const { return *this <  fun(); }
      bool operator<  (const level& (&fun)()) const { return *this >  fun(); }
      bool operator>= (const level& (&fun)()) const { return *this >= fun(); }
      bool operator<= (const level& (&fun)()) const { return *this <= fun(); }
  };

  inline const level& debug()  { static level l(1000); return l; }
  inline const level& info()   { static level l(2000); return l; }
  inline const level& notice() { static level l(3000); return l; }
  inline const level& warn()   { static level l(4000); return l; }
  inline const level& error()  { static level l(5000); return l; }
  inline const level& crit()   { static level l(6000); return l; }
  inline const level& alert()  { static level l(7000); return l; }
  inline const level& emerg()  { static level l(8000); return l; }

  class msg {
    protected:
      const logger& _logger;
      level _level;
      std::auto_ptr<std::ostringstream> _stream;

    public:
      msg(msg& m)
        : _logger(m._logger), _level(m._level), _stream(m._stream) {
        /* empty */
      }

      msg(const logger& l);
      ~msg();

      const std::string str() const {
        return _stream->str();
      }

      const level& lvl() const {
        return _level;
      }

      template<typename T> msg& operator<< (const T& value) {
        *_stream << value;
        return *this;
      }

      msg& operator<< (const level& (&fun)()) {
        _level = fun();
        return *this;
      }
  };

  class stage {
    public:
      typedef std::auto_ptr<stage> ptr;

    protected:
      ptr next;

    public:
      virtual ~stage() { /* empty */ }

      virtual bool pass(msg&) const = 0;

      void append(ptr& item) {
        if(next.get())
          next->append(item);
        else
          next = item;
      }

      void send(msg& m) const {
        if(pass(m) && next.get())
          next->send(m);
      }
  };

  inline stage::ptr operator| (stage::ptr first, stage::ptr second) {
    first->append(second);
    return first;
  }

  inline stage::ptr operator| (stage::ptr (&first)(), stage::ptr second) {
    return first() | second;
  }

  inline stage::ptr operator| (stage::ptr first, stage::ptr (&second)()) {
    return first | second();
  }

  class logger {
    protected:
      level _msg_level;
      std::vector<stage*> _pipes;

    public:
      logger() : _msg_level(notice()) { /* empty */ }

      ~logger() {
        for(std::vector<stage*>::iterator it = _pipes.begin();
            it != _pipes.end(); ++ it)
          delete *it;
      }

      void add_pipe(stage::ptr stage) { _pipes.push_back(stage.release()); }
      void add_pipe(stage::ptr (&fun)()) { add_pipe(fun()); }

      void msg_level(const level& lvl)      { _msg_level = lvl; }
      void msg_level(const level& (&fun)()) { msg_level(fun()); }
      const level& msg_level() const        { return _msg_level; }

      void send(msg& m) const {
        for(std::vector<stage*>::const_iterator it = _pipes.begin();
            it != _pipes.end(); ++ it)
          (*it)->send(m);
      }

      template<typename T> msg operator<< (const T& value) const {
        msg m(*this);
        m << value;
        return m;
      }
  };

  inline msg::msg(const logger& l)
    : _logger(l), _level(l.msg_level()), _stream(new std::ostringstream) {
    /* empty */
  }

  inline msg::~msg() {
    if(_stream.get())
      _logger.send(*this);
  }

  namespace slo_detail
  {
    class stderr_stage : public stage {
      public:
        virtual bool pass(msg& m) const {
          std::cerr << m.str() << std::endl;
          return false;
        }
    };

    class syslog_stage : public stage {
      protected:
        const std::string _ident;

      public:
        syslog_stage(const std::string& ident, int facility = LOG_USER)
          : _ident(ident) {
          ::openlog(_ident.c_str(), 0, facility);
        }

        virtual bool pass(msg& m) const {
          const level& lvl = m.lvl();
          ::syslog(lvl <= debug  ? LOG_DEBUG :
                   lvl <= info   ? LOG_INFO :
                   lvl <= notice ? LOG_NOTICE :
                   lvl <= warn   ? LOG_WARNING :
                   lvl <= error  ? LOG_ERR :
                   lvl <= crit   ? LOG_CRIT :
                   lvl <= alert  ? LOG_ALERT :
                   LOG_EMERG,
                   "%s", m.str().c_str());
          return false;
        }
    };

    template<typename T> class level_cmp_stage : public stage {
      protected:
        T _fun;
        const level& _with;

      public:
        level_cmp_stage(T fun, const level& with) : _fun(fun), _with(with) {
          /* empty */
        }

        virtual bool pass(msg& m) const {
          return _fun(m.lvl(), _with);
        }
    };
  }

  inline stage::ptr stderr() {
    return stage::ptr(new slo_detail::stderr_stage());
  }

  inline stage::ptr syslog(const std::string& ident,
                           int facility = LOG_USER) {
    return stage::ptr(new slo_detail::syslog_stage(ident, facility));
  }

  inline stage::ptr min_level(const level& (&fun)()) {
    return stage::ptr(new slo_detail::level_cmp_stage
                            <std::greater_equal<const level&> >
                            (std::greater_equal<const level&>(), fun()));
  }

  inline stage::ptr max_level(const level& (&fun)()) {
    return stage::ptr(new slo_detail::level_cmp_stage
                            <std::less_equal<const level&> >
                            (std::less_equal<const level&>(), fun()));
  }
}

#endif /* SLO_HH */
