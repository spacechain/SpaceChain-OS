//  (C) Copyright ACOINFO 2015 - 2018.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version.

// sylixos specific config options:
// --------------------------------
#define BOOST_PLATFORM "SylixOS"


// Generally available headers:
#define BOOST_HAS_UNISTD_H
#define BOOST_HAS_STDINT_H
#define BOOST_HAS_DIRENT_H
#define BOOST_HAS_SLIST

// SylixOS does not have installed an iconv-library by default,
// so unfortunately no Unicode support from scratch is available!
// Thus, instead it is suggested to switch to ICU, as this seems
// to be the most complete and portable option...
#define BOOST_LOCALE_WITH_ICU

// Generally available functionality:
#define BOOST_HAS_THREADS
#define BOOST_HAS_NANOSLEEP
#define BOOST_HAS_GETTIMEOFDAY
#define BOOST_HAS_CLOCK_GETTIME
#define BOOST_HAS_MACRO_USE_FACET

// Generally unavailable functionality, delivered by boost's test function:
//#define BOOST_NO_DEDUCED_TYPENAME // Commented this out, boost's test gives an errorneous result!
#define BOOST_NO_CXX11_EXTERN_TEMPLATE
#define BOOST_NO_CXX11_VARIADIC_MACROS

// Generally available threading API's:
#define BOOST_HAS_PTHREADS
#define BOOST_HAS_SCHED_YIELD
#define BOOST_HAS_SIGACTION

// Disable BOOST ASSERTS in SylixOS.
// NOTE assert(e) do NOTHING in SylixOS.
// #define assert(e)   ((void)0)
#define BOOST_DISABLE_ASSERTS
#define BOOST_THREAD_DONT_USE_ATOMIC

#include <SylixOS.h>


