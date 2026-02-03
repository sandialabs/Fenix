#ifndef FENIX_LOGGING_UTIL_H
#define FENIX_LOGGING_UTIL_H

#include "fenix_opt.hpp"

//#define FENIX_MESSAGE_LOG_VERBOSE

#ifdef FENIX_MESSAGE_LOG_VERBOSE
#define MLOG(...) debug_print(__VA_ARGS__);
#else
#define MLOG(...)
#endif

#endif
