//
// repo:            event_loop_invoker
// file:			export_symbols.h
// path:			include/event_loop_invoker/export_symbols.h
// created on:		2024 Dec 07
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EXPORT_SYMBOLS_H
#define EVLOOPINVK_INCLUDE_EVLOOPINVK_EXPORT_SYMBOLS_H

#include <cinternal/internal_header.h>


#ifndef EVLOOPINVK_EXPORT
#if defined(EVLOOPINVK_COMPILING_SHARED_LIB)
#define EVLOOPINVK_EXPORT CPPUTILS_DLL_PUBLIC
#elif defined(EVLOOPINVK_USING_STATIC_LIB_OR_OBJECTS)
#define EVLOOPINVK_EXPORT
#elif defined(EVLOOPINVK_LOAD_FROM_DLL)
#define EVLOOPINVK_EXPORT CPPUTILS_IMPORT_FROM_DLL
#else
#define EVLOOPINVK_EXPORT CPPUTILS_DLL_PRIVATE
#endif
#endif


#endif  // #ifndef EVLOOPINVK_INCLUDE_EVLOOPINVK_EXPORT_SYMBOLS_H
