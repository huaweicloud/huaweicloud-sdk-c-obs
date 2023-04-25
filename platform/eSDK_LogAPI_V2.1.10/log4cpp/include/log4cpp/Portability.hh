/*
 * Portability.hh
 *
 * Copyright 2001, LifeLine Networks BV (www.lifeline.nl). All rights reserved.
 * Copyright 2001, Bastiaan Bakker. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef _LOG4CPP_PORTABILITY_HH
#define _LOG4CPP_PORTABILITY_HH

#if defined (_MSC_VER) || defined(__BORLANDC__)
#    if defined (LOG4CPP_STLPORT_AND_BOOST_BUILD)
#        include <log4cpp/config-win32-stlport-boost.h>
#    else
#        include <log4cpp/config-win32.h>
#    endif

#ifdef MSVC_MEMORY_LEAK_CHECK
#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>

#ifdef _DEBUG
   #ifndef DBG_NEW
      #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
      #define new DBG_NEW
   #endif
#endif  // _DEBUG
#endif // MSVC_MEMORY_LEAK_CHECK

#else
#if defined(__OPENVMS__)
#    include <log4cpp/config-openvms.h>
#else
#   if defined(__MINGW32__)
#       include <log4cpp/config-MinGW32.h>
#   else
#       include <log4cpp/config.h>
#   endif
#endif
#endif

#include <log4cpp/Export.hh>

#if defined(_MSC_VER)
#    pragma warning( disable : 4786 ) // 255 char debug symbol limit
#    pragma warning( disable : 4290 ) // throw specifier not implemented
#    pragma warning( disable : 4251 ) // "class XXX should be exported"
#endif

#ifdef __APPLE__
#  include <sstream>
#else
#  ifndef LOG4CPP_HAVE_SSTREAM
#    include <strstream>
     namespace std {
       class LOG4CPP_EXPORT ostringstream : public ostrstream {
       public:
	 std::string str();
       };
     };
#  endif // LOG4CPP_HAVE_SSTREAM
#endif // _APPLE_

// Support both modern and deprecated compilers, supplying different c++language constructs via macros
#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define LOG4CPP_UNIQUE_PTR unique_ptr
#define LOG4CPP_UNIQUE_PTR_MOVE(ptr) std::move(ptr)
#else
#define LOG4CPP_UNIQUE_PTR auto_ptr
#define LOG4CPP_UNIQUE_PTR_MOVE(ptr) ptr
#endif //__cplusplus

#if defined(__cplusplus) && (__cplusplus >= 201703L)
#define LOG4CPP_NOTHROW noexcept
#else
#define LOG4CPP_NOTHROW throw()
#endif //__cplusplus

#endif //_LOG4CPP_PORTABILITY_HH
