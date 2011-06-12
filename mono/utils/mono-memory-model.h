/*
 * mono-memory-model.h: Mapping of the arch memory model.
 *
 * Author:
 *	Rodrigo Kumpera (kumpera@gmail.com)
 *
 * (C) 2011 Xamarin, Inc
 */

#ifndef _MONO_UTILS_MONO_MEMMODEL_H_
#define _MONO_UTILS_MONO_MEMMODEL_H_

#include <config.h>
#include <mono/utils/mono-membar.h>

/*
In order to allow for fast concurrent code, we must use fencing to properly order
memory access - specially on arch with weaker memory models such as ARM or PPC.

On the other hand, we can't use arm's weak model on targets such as x86 that have
a stronger model that requires much much less fencing.

The idea of exposing each arch memory model is to avoid fencing whenever possible
but at the same time make all required ordering explicit. 

There are four kinds of barriers, LoadLoad, LoadStore, StoreLoad and StoreStore.
Each arch must define which ones needs fencing.

We assume 3 kinds of barriers are available: load, store and memory (load+store).

TODO: Add support for weaker forms of CAS such as present on ARM.
TODO: replace all explicit uses of memory barriers with macros from this section. This will make a nicer read of lazy init code.
TODO: if we find places where a data depencency could replace barriers, add macros here to help with it
TODO: some arch with strong consistency, such as x86, support weaker access. We might need to expose more kinds of barriers once we exploit this.
*/

#define MEMORY_BARRIER mono_memory_barrier
#define LOAD_BARRIER mono_memory_read_barrier
#define STORE_BARRIEE mono_memory_write_barrier

enum {
	StoreStoreBarrier,
	LoadLoadBarrier,
	StoreLoadBarrier,
	LoadStoreBarrier,
	FullBarrier
};

#if defined(__i386__) || defined(__x86_64__)
/*
Both x86 and amd64 follow the SPO memory model:
-Loads are not reordered with other loads
-Stores are not reordered with others stores
-Stores are not reordered with earlier loads
*/

/*Neither sfence or mfence provide the required semantics here*/
#define STORE_LOAD_FENCE MEMORY_BARRIER

#elif defined(__arm__)
/*
ARM memory model is as weak as it can get. the only guarantee are data dependent
accesses.
LoadStore fences are much better handled using a data depencency such as:
load x;  if (x = x) store y;

This trick can be applied to other fences such as LoadLoad, but require some assembly:

LDR R0, [R1]
AND R0, R0, #0
LDR R3, [R4, R0]
*/

#define STORE_STORE_FENCE STORE_BARRIER
#define LOAD_LOAD_FENCE LOAD_BARRIER
#define STORE_LOAD_FENCE MEMORY_BARRIER
#define LOAD_STORE_FENCE MEMORY_BARRIER

#else

/*default implementation with the weakest possible memory model */
#define STORE_STORE_FENCE STORE_BARRIER
#define LOAD_LOAD_FENCE LOAD_BARRIER
#define STORE_LOAD_FENCE MEMORY_BARRIER
#define LOAD_STORE_FENCE MEMORY_BARRIER

#endif

#ifndef STORE_STORE_FENCE
#define STORE_STORE_FENCE
#endif 

#ifndef LOAD_LOAD_FENCE
#define LOAD_LOAD_FENCE
#endif 

#ifndef STORE_LOAD_FENCE
#define STORE_LOAD_FENCE
#endif 

#ifndef LOAD_STORE_FENCE
#define LOAD_STORE_FENCE
#endif 

/*
Acquire/release semantics macros.

Acquire/release models what most code needs, which is to do load/store pairing of barriers
from multiple threads.
Release semantics makes sure all previous store have completed before the next memory access.
Acquire semantics make sure all following loads won't execute before the previous one.

This is a slightly harmless variantion on ECMA's that further constraints ordering amongs
different kinds of access.
*/
#define mono_atomic_store_release(target,value) do {	\
	STORE_STORE_FENCE;	\
	*(target) = (value);	\
} while (0)

#define mono_atomic_load_acquire(target) ({	\
	typeof (*target) __tmp = *target;	\
	LOAD_LOAD_FENCE;	\
	__tmp; })

#endif /* _MONO_UTILS_MONO_MEMMODEL_H_ */
