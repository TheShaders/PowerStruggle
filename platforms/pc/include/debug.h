#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <cstdio>

#ifdef NDEBUG
#define debug_printf(...) ;
#else
#define debug_printf printf
#endif


#endif
