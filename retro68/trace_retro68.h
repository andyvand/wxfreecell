/* trace_retro68.h -- startup tracing for testing the classic Mac OS build
   in an emulator. Only compiled in with -DFREECELL_TRACE=ON (CMake); see
   retro68/trace_retro68.cpp. */

#ifndef TRACE_RETRO68_H
#define TRACE_RETRO68_H

#ifdef FREECELL_TRACE
void FcTrace(const char *fmt, ...);
void FcTraceInit();
void FcTraceAlive(class wxFrame *frame);
#  define FCTRACE(...) FcTrace(__VA_ARGS__)
#else
#  define FCTRACE(...) ((void)0)
#endif

#endif
