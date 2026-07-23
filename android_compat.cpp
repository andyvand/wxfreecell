/****************************************************************************
 * android_compat.cpp
 *
 * Compatibility shims for the wxWidgets wxQt build on Android.
 *
 * The wxQt Android build of wxWidgets 3.3.4 declares
 * wxGUIAppTraits::GetEventLoopSourcesManager() in its vtable (because
 * wxUSE_EVENTLOOP_SOURCE is 1 for the GUI event loop) but does not compile an
 * implementation: src/unix/evtloopunix.cpp is excluded since the console event
 * loop and the select/epoll dispatchers are disabled in that build's setup.h
 * (wxUSE_CONSOLE_EVENTLOOP / wxUSE_SELECT_DISPATCHER / wxUSE_EPOLL_DISPATCHER
 * are all 0). That leaves the symbol undefined at link time.
 *
 * FreeCell never registers event-loop file-descriptor sources, so a stub that
 * returns nullptr is sufficient to satisfy the linker and is never actually
 * invoked at runtime. This file is compiled only for the Android target.
 ****************************************************************************/

/* Include the full GUI prelude so wxUSE_GUI (and therefore
 * wxUSE_EVENTLOOP_SOURCE) match the configuration the wx libraries were built
 * with. Including only wx/defs.h leaves wxUSE_EVENTLOOP_SOURCE at 0 and the
 * definition below would be compiled out. */
#include <wx/wx.h>
/* wx/evtloop.h defines wxUSE_EVENTLOOP_SOURCE; wx/wx.h alone does not pull it
 * in, so the macro would otherwise be undefined (treated as 0) here and the
 * definition below compiled out. apptrait.h declares the method. */
#include <wx/evtloop.h>
#include <wx/apptrait.h>
#include <wx/evtloopsrc.h>

#if wxUSE_EVENTLOOP_SOURCE
wxEventLoopSourcesManagerBase* wxGUIAppTraits::GetEventLoopSourcesManager()
{
    return nullptr;
}
#endif // wxUSE_EVENTLOOP_SOURCE
