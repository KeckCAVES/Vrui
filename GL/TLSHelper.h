/***********************************************************************
TLSHelper.h - Macro definition to simplify supporting thread-local
storage types inside the GLSupport library.
Copyright (c) 2006 Oliver Kreylos
***********************************************************************/

#ifndef GLTLSHELPER_INCLUDED
#define GLTLSHELPER_INCLUDED

#ifdef GLSUPPORT_USE_TLS

#ifdef SYSTEM_HAVE_TLS

/* Use the compiler's support for thread-local storage: */
#define GL_THREAD_LOCAL(VariableType) __thread VariableType

#else

/* Use pthreads' thread-local storage mechanism: */
#define GL_THREAD_LOCAL(VariableType) Threads::Local<VariableType>

#endif

#else

/* Ignore the THREAD_LOCAL modifier: */
#define GL_THREAD_LOCAL(VariableType) VariableType

#endif

#endif
