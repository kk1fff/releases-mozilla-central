/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Value-Profiling Utility.
 *
 * The Initial Developer of the Original Code is
 * Intel Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mohammad R. Haghighat [mohammad.r.haghighat@intel.com] 
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//
//  Here are a few examples of using the value-profiling utility:
//
//  _vprof (e);
//    at the end of program execution, you'll get a dump of the source location of this probe,
//    its min, max, average, the total sum of all instances of e, and the total number of times this probe was called. 
//
//  _vprof (x > 0); 
//    shows how many times and what percentage of the cases x was > 0, 
//    that is the probablitiy that x > 0.
// 
// _vprof (n % 2 == 0); 
//    shows how many times n was an even number 
//    as well as th probablitiy of n being an even number. 
// 
// _hprof (n, 4, 1000, 5000, 5001, 10000); 
//    gives you the histogram of n over the given 4 bucket boundaries:
//        # cases <  1000 
//        # cases >= 1000 and < 5000
//        # cases >= 5000 and < 5001
//        # cases >= 5001 and < 10000
//        # cases >= 10000  
// 
// _nvprof ("event name", value);   
//    all instances with the same name are merged
//    so, you can call _vprof with the same event name at difference places
// 
// _vprof (e, myProbe);  
//    value profile e and call myProbe (void* vprofID) at the profiling point.
//    inside the probe, the client has the predefined variables:
//    _VAL, _COUNT, _SUM, _MIN, _MAX, and the general purpose registers
//    _IVAR1, ..., IVAR4      general integer registrs
//    _I64VAR1, ..., I64VAR4  general integer64 registrs    
//    _DVAR1, ..., _DVAR4     general double registers
//    _GENPTR a generic pointer that can be used by the client 
//    the number of registers can be changed in vprof.h
//

#ifndef __VPROF__
#define __VPROF__
//
// If the application for which you want to use vprof is threaded, THREADED must be defined as 1, otherwise define it as 0
//
// If your application is not threaded, define THREAD_SAFE 0,
// otherwise, you have the option of setting THREAD_SAFE to 1 which results in exact counts or to 0 which results in a much more efficient but non-exact counts
//
#define THREADED 0
#define THREAD_SAFE 0

#include "VMPI.h"

// Note, this is not supported in configurations with more than one AvmCore running
// in the same process.

// portable align macro
#if defined(_MSC_VER)
	#define vprof_align8(t) __declspec(align(8)) t
#elif defined(__GNUC__)
	#define vprof_align8(t) t __attribute__ ((aligned (8)))
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
	#define vprof_align8(t) t __attribute__ ((aligned (8)))
#elif defined(VMCFG_SYMBIAN)
	#define vprof_align8(t) t __attribute__ ((aligned (8)))
#endif

#ifdef __cplusplus
extern "C" {
#endif

int initValueProfile(void** id, char* file, int line, ...);
int profileValue(void* id, int64_t value);
int initHistProfile(void** id, char* file, int line, int nbins, ...);
int histValue(void* id, int64_t value);
uint64_t readTimestampCounter();

#ifdef __cplusplus
}
#endif 

//#define DOPROF

#ifndef DOPROF
#define _nvprof(e,v)
#ifndef VMCFG_SYMBIAN
#define _vprof(v,...)
#define _hprof(h,n,...)
#define _nhprof(e,v,n,...)
#define _ntprof_begin(e)
#define _ntprof_end(e)
#define _jvprof_init(id,...)
#define _jnvprof_init(id,e,...)
#define _jvprof(id,v)
#endif // ! VMCFG_SYMBIAN
#else

// Historical/compatibility note:
// The macros below were originally written using conditional expressions, not if/else.  The original author
// said that this was done to allow _vprof and _nvprof to be used in an expression context, but the old code
// had already wrapped the macro bodies in { }, so it is not clear how this could have worked.  At present,
// the profiling macros must appear in a statement context only.
 
#define _vprof(v,...) \
do { \
    static void* id = 0; \
    if (id == 0) \
        initValueProfile(&id, __FILE__, __LINE__, ##__VA_ARGS__, NULL); \
    profileValue(id, (int64_t) (v)); \
} while (0)

#define _nvprof(e,v) \
do { \
    static void* id = 0; \
    if (id == 0) \
        initValueProfile(&id, (char*) (e), -1, NULL); \
    profileValue(id, (int64_t) (v)); \
} while (0)

#define _hprof(v,n,...) \
do { \
    static void* id = 0; \
    if (id == 0) \
        initHistProfile(&id, __FILE__, __LINE__, (int) (n), ##__VA_ARGS__); \
    histValue(id, (int64_t) (v)); \
} while (0)

#define _nhprof(e,v,n,...) \
do { \
    static void* id = 0; \
    if (id == 0) \
        initHistProfile(&id, (char*) (e), -1, (int) (n), ##__VA_ARGS__); \
    histValue(id, (int64_t) (v)); \
} while (0)

// Profile execution time between _ntprof_begin(e) and _ntprof_end(e).
// The tag 'e' must match at the beginning and end of the region to
// be timed.  Regions may be nested or overlap arbitrarily, as it is
// the tag alone that defines the begin/end correspondence.

#define _ntprof_begin(e) \
do { \
    static void* id = 0; \
    if (id == 0) \
        initValueProfile(&id, (char*)(e), -1, NULL); \
    ((entry_t)id)->i64var[0] = readTimestampCounter(); \
} while (0)

// Assume 2.6 Ghz CPU
#define TICKS_PER_USEC 2600

#define _ntprof_end(e) \
do { \
    static void* id = 0; \
    uint64_t stop = readTimestampCounter(); \
    if (id == 0) \
        initValueProfile(&id, (char*)(e), -1, NULL); \
    uint64_t start = ((entry_t)id)->i64var[0]; \
    uint64_t usecs = (stop - start) / TICKS_PER_USEC; \
    profileValue(id, usecs); \
} while (0)

// These macros separate the creation of a profile record from its later usage.
// They are intended for profiling JIT-generated code.  Once created, the JIT can
// bind a pointer to the profile record into the generated code, which can then
// record profile events during execution.

#define _jvprof_init(id,...) \
    if (*(id) == 0) \
        initValueProfile((id), __FILE__, __LINE__, ##__VA_ARGS__, NULL)

#define _jnvprof_init(id,e,...) \
    if (*(id) == 0) \
        initValueProfile((id), (char*) (e), -1, ##__VA_ARGS__, NULL)

// Calls to the _jvprof macro must be wrapped in an actual function
// in order to be invoked from JIT-compiled code.

#define _jvprof(id,v) \
    profileValue((id), (int64_t) (v))

#endif

#define NUM_EVARS 4

enum {
    LOCK_IS_FREE = 0, 
    LOCK_IS_TAKEN = 1
};

extern
#ifdef __cplusplus
"C" 
#endif
long _InterlockedCompareExchange (
   long volatile * Destination,
   long Exchange,
   long Comperand
);

typedef struct hist hist;

typedef struct hist {
    int nbins;
    int64_t* lb;
    int64_t* count;
} *hist_t;

typedef struct entry entry;

typedef struct entry {
    long lock;
    char* file;
    int line;
    int64_t value;
    int64_t count;
    int64_t sum;
    int64_t min;
    int64_t max;
    void (*func)(void*);
    hist* h;

    entry* next;

    // exposed to the clients
    void* genptr;
    int ivar[NUM_EVARS];
    vprof_align8(int64_t) i64var[NUM_EVARS];
    vprof_align8(double) dvar[NUM_EVARS];
    //

    char pad[128]; // avoid false sharing
} *entry_t;

#define _VAL ((entry_t)vprofID)->value
#define _COUNT ((entry_t)vprofID)->count
#define _SUM ((entry_t)vprofID)->sum
#define _MIN ((entry_t)vprofID)->min
#define _MAX ((entry_t)vprofID)->max

#define _GENPTR ((entry_t)vprofID)->genptr

#define _IVAR0 ((entry_t)vprofID)->ivar[0]
#define _IVAR1 ((entry_t)vprofID)->ivar[1]
#define _IVAR2 ((entry_t)vprofID)->ivar[2]
#define _IVAR3 ((entry_t)vprofID)->ivar[3]

#define _I64VAR0 ((entry_t)vprofID)->i64var[0]
#define _I64VAR1 ((entry_t)vprofID)->i64var[1]
#define _I64VAR2 ((entry_t)vprofID)->i64var[2]
#define _I64VAR3 ((entry_t)vprofID)->i64var[3]

#define _DVAR0 ((entry_t)vprofID)->dvar[0]
#define _DVAR1 ((entry_t)vprofID)->dvar[1]
#define _DVAR2 ((entry_t)vprofID)->dvar[2]
#define _DVAR3 ((entry_t)vprofID)->dvar[3]

#endif /* __VPROF__ */
