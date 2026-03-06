#include <vector>
#include <memory>
#include <numeric>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <unordered_set>

#include "HeapProfiler.h"

#include "MinHook.h"
#include "dbghelp.h"
#include <tlhelp32.h>
#include <winternl.h>

typedef __int64 int64_t;
typedef void * (__cdecl *PtrMalloc)(size_t);
typedef void (__cdecl *PtrFree)(void *);
typedef void* (__cdecl *PtrRealloc)(void *, size_t);
typedef void * (__cdecl *PtrCalloc)(size_t, size_t);
typedef void * (__cdecl *PtrRecalloc)(void *, size_t, size_t);
typedef void * (__cdecl *PtrCallocDbg)(size_t, size_t, int, char const*, int);
typedef void * (__cdecl *PtrMallocDbg)(size_t, int, char const*, int);
typedef void * (__cdecl *PtrReallocDbg)(void*, size_t, int, char const*, int);
typedef void * (__cdecl *PtrRecallocDbg)(void*, size_t, size_t, int, char const*, int);
typedef void (__cdecl *PtrFreeDbg)(void*, int);

typedef void (__cdecl *PtrAlignedFree)(void*);
typedef void* (__cdecl *PtrAlignedMalloc)(size_t, size_t);
typedef void* (__cdecl *PtrAlignedOffsetMalloc)(size_t, size_t, size_t);
typedef void* (__cdecl *PtrAlignedOffsetRealloc)(void*, size_t, size_t, size_t);
typedef void* (__cdecl *PtrAlignedOffsetRecalloc)(void*, size_t, size_t, size_t, size_t);
typedef void* (__cdecl *PtrAlignedRealloc)(void*, size_t, size_t);
typedef void* (__cdecl *PtrAlignedRecalloc)(void*, size_t, size_t, size_t);

typedef void (__cdecl *PtrAlignedFreeDbg)(void*);
typedef void* (__cdecl *PtrAlignedMallocDbg)(size_t, size_t, char const*, int);
typedef void* (__cdecl *PtrAlignedOffsetMallocDbg)(size_t, size_t, size_t, char const*, int);
typedef void* (__cdecl *PtrAlignedOffsetReallocDbg)(void*, size_t, size_t, size_t, char const*, int);
typedef void* (__cdecl *PtrAlignedOffsetRecallocDbg)(void*, size_t, size_t, size_t, size_t, char const*, int);
typedef void* (__cdecl *PtrAlignedReallocDbg)(void*, size_t, size_t, char const*, int);
typedef void* (__cdecl *PtrAlignedRecallocDbg)(void*, size_t, size_t, size_t, char const*, int);

// Hook tables. (Lot's of static data, but it's the only way to do this.)
const int numHooks = 128;
Mutex hookTableMutex;
int nUsedMallocHooks = 0;
int nUsedFreeHooks = 0;
int nUsedReallocHooks = 0;
int nUsedCallocHooks = 0;
int nUsedRecallocHooks = 0;
int nUsedCallocDbgHooks = 0;
int nUsedMallocDbgHooks = 0;
int nUsedReallocDbgHooks = 0;
int nUsedRecallocDbgHooks = 0;
int nUsedFreeDbgHooks = 0;
int nUsedAlignedFreeHooks = 0;
int nUsedAlignedMallocHooks = 0;
int nUsedAlignedOffsetMallocHooks = 0;
int nUsedAlignedOffsetReallocHooks = 0;
int nUsedAlignedOffsetRecallocHooks = 0;
int nUsedAlignedReallocHooks = 0;
int nUsedAlignedRecallocHooks = 0;
int nUsedAlignedFreeDbgHooks = 0;
int nUsedAlignedMallocDbgHooks = 0;
int nUsedAlignedOffsetMallocDbgHooks = 0;
int nUsedAlignedOffsetReallocDbgHooks = 0;
int nUsedAlignedOffsetRecallocDbgHooks = 0;
int nUsedAlignedReallocDbgHooks = 0;
int nUsedAlignedRecallocDbgHooks = 0;
PtrMalloc mallocHooks[numHooks];
PtrFree freeHooks[numHooks];
PtrRealloc reallocHooks[numHooks];
PtrCalloc callocHooks[numHooks];
PtrRecalloc recallocHooks[numHooks];
PtrCallocDbg callocDbgHooks[numHooks];
PtrMallocDbg mallocDbgHooks[numHooks];
PtrReallocDbg reallocDbgHooks[numHooks];
PtrRecallocDbg recallocDbgHooks[numHooks];
PtrFreeDbg freeDbgHooks[numHooks];
PtrAlignedFree alignedFreeHooks[numHooks];
PtrAlignedMalloc alignedMallocHooks[numHooks];
PtrAlignedOffsetMalloc alignedOffsetMallocHooks[numHooks];
PtrAlignedOffsetRealloc alignedOffsetReallocHooks[numHooks];
PtrAlignedOffsetRecalloc alignedOffsetRecallocHooks[numHooks];
PtrAlignedRealloc alignedReallocHooks[numHooks];
PtrAlignedRecalloc alignedRecallocHooks[numHooks];
PtrAlignedFreeDbg alignedFreeDbgHooks[numHooks];
PtrAlignedMallocDbg alignedMallocDbgHooks[numHooks];
PtrAlignedOffsetMallocDbg alignedOffsetMallocDbgHooks[numHooks];
PtrAlignedOffsetReallocDbg alignedOffsetReallocDbgHooks[numHooks];
PtrAlignedOffsetRecallocDbg alignedOffsetRecallocDbgHooks[numHooks];
PtrAlignedReallocDbg alignedReallocDbgHooks[numHooks];
PtrAlignedRecallocDbg alignedRecallocDbgHooks[numHooks];

PtrMalloc originalMallocs[numHooks];
PtrFree originalFrees[numHooks];
PtrRealloc originalReallocs[numHooks];
PtrCalloc originalCallocs[numHooks];
PtrRecalloc originalRecallocs[numHooks];
PtrCallocDbg originalCallocDbgs[numHooks];
PtrMallocDbg originalMallocDbgs[numHooks];
PtrReallocDbg originalReallocDbgs[numHooks];
PtrRecallocDbg originalRecallocDbgs[numHooks];
PtrFreeDbg originalFreeDbgs[numHooks];
PtrAlignedFree originalAlignedFrees[numHooks];
PtrAlignedMalloc originalAlignedMallocs[numHooks];
PtrAlignedOffsetMalloc originalAlignedOffsetMallocs[numHooks];
PtrAlignedOffsetRealloc originalAlignedOffsetReallocs[numHooks];
PtrAlignedOffsetRecalloc originalAlignedOffsetRecallocs[numHooks];
PtrAlignedRealloc originalAlignedReallocs[numHooks];
PtrAlignedRecalloc originalAlignedRecallocs[numHooks];
PtrAlignedFreeDbg originalAlignedFreeDbgs[numHooks];
PtrAlignedMallocDbg originalAlignedMallocDbgs[numHooks];
PtrAlignedOffsetMallocDbg originalAlignedOffsetMallocDbgs[numHooks];
PtrAlignedOffsetReallocDbg originalAlignedOffsetReallocDbgs[numHooks];
PtrAlignedOffsetRecallocDbg originalAlignedOffsetRecallocDbgs[numHooks];
PtrAlignedReallocDbg originalAlignedReallocDbgs[numHooks];
PtrAlignedRecallocDbg originalAlignedRecallocDbgs[numHooks];

// Hook for LdrpCallInitRoutine
typedef BOOLEAN (NTAPI *PDLL_INIT_ROUTINE)(PVOID, ULONG, PCONTEXT);
typedef BOOLEAN (NTAPI *LdrpCallInitRoutine_t)(PDLL_INIT_ROUTINE, PVOID, ULONG, PVOID);
static LdrpCallInitRoutine_t orgLdrpCallInitRoutine;

HMODULE hDllModule;
HANDLE hCurrentProcess;
Mutex dbgHelpMutex;
HeapProfiler *heapProfiler;
std::unordered_set<std::string, std::hash<std::string>, std::equal_to<std::string>, HeapAllocator<std::string> > hookedDllSet;
Mutex hookedDllSetMutex;

// Mechanism to stop us profiling ourself.
DWORD tlsIndex;

struct PreventSelfProfile{
	PreventSelfProfile(){
		intptr_t depthCount = (intptr_t)TlsGetValue(tlsIndex);
		TlsSetValue(tlsIndex, (LPVOID)(depthCount+1));
	}
	~PreventSelfProfile(){
		intptr_t depthCount = (intptr_t)TlsGetValue(tlsIndex);
		TlsSetValue(tlsIndex, (LPVOID)(depthCount-1));
	}

	inline bool shouldProfile(){
		intptr_t depthCount = (intptr_t)TlsGetValue(tlsIndex);
		return depthCount <= 1;
	}
private:
	PreventSelfProfile(const PreventSelfProfile&){}
	PreventSelfProfile& operator=(const PreventSelfProfile&){}
};

void PreventEverProfilingThisThread(){
	intptr_t depthCount = (intptr_t)TlsGetValue(tlsIndex);
	TlsSetValue(tlsIndex, (LPVOID)(depthCount+1));
}

// Malloc hook function. Templated so we can hook many mallocs.
template <int N>
void * __cdecl mallocHook(size_t size){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalMallocs[N](size);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			trace.trace();
			heapProfiler->malloc(p, size, trace);
		}
	}
	SetLastError(lastError);

	return p;
}

// Free hook function.
template <int N>
void  __cdecl freeHook(void * p){
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		originalFrees[N](p);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			//trace.trace();
			heapProfiler->free(p, trace);
		}
	}
	SetLastError(lastError);
}

// Realloc hook function. Templated so we can hook many mallocs.
template <int N>
void * __cdecl reallocHook(void* memblock, size_t size){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalReallocs[N](memblock, size);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// Calloc hook function. Templated so we can hook many Callocs.
template <int N>
void * __cdecl callocHook(size_t num, size_t size){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalCallocs[N](num, size);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			trace.trace();
			heapProfiler->malloc(p, num * size, trace);
		}
	}
	SetLastError(lastError);

	return p;
}

// Recalloc hook function. Templated so we can hook many recallocs.
template <int N>
void * __cdecl recallocHook(void* memblock, size_t num, size_t size){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalRecallocs[N](memblock, num, size);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, num * size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, num * size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// CallocDbg hook function. Templated so we can hook many _calloc_dbgs.
template <int N>
void * __cdecl callocDbgHook(size_t num, size_t size, int block, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalCallocDbgs[N](num, size, block, filename, linenumber);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			trace.trace();
			heapProfiler->malloc(p, num * size, trace);
		}
	}
	SetLastError(lastError);

	return p;
}

// MallocDbg hook function. Templated so we can hook many _malloc_dbgs.
template <int N>
void * __cdecl mallocDbgHook(size_t size, int block, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalMallocDbgs[N](size, block, filename, linenumber);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			trace.trace();
			heapProfiler->malloc(p, size, trace);
		}
	}
	SetLastError(lastError);

	return p;
}

// Realloc hook function. Templated so we can hook many _realloc_dbgs.
template <int N>
void * __cdecl reallocDbgHook(void* memblock, size_t size, int block, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalReallocDbgs[N](memblock, size, block, filename, linenumber);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// RecallocDbg hook function. Templated so we can hook many _recalloc_dbgs.
template <int N>
void * __cdecl recallocDbgHook(void* memblock, size_t num, size_t size, int block, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalRecallocDbgs[N](memblock, num, size, block, filename, linenumber);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, num * size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, num * size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// FreeDbg hook function.
template <int N>
void  __cdecl freeDbgHook(void * p, int block){
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		originalFreeDbgs[N](p, block);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			//trace.trace();
			heapProfiler->free(p, trace);
		}
	}
	SetLastError(lastError);
}

// AlignedFree hook function.
template <int N>
void  __cdecl alignedFreeHook(void * p){
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		originalAlignedFrees[N](p);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			//trace.trace();
			heapProfiler->free(p, trace);
		}
	}
	SetLastError(lastError);
}

// AlignedMalloc hook function. Templated so we can hook many _aligned_mallocs.
template <int N>
void * __cdecl alignedMallocHook(size_t size, size_t alignment){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedMallocs[N](size, alignment);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			trace.trace();
			heapProfiler->malloc(p, size, trace);
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedOffsetMalloc hook function. Templated so we can hook many _aligned_offset_mallocs.
template <int N>
void * __cdecl alignedOffsetMallocHook(size_t size, size_t alignment, size_t offset){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedOffsetMallocs[N](size, alignment, offset);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			trace.trace();
			heapProfiler->malloc(p, size, trace);
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedOffsetRealloc hook function. Templated so we can hook many _aligned_offset_reallocs.
template <int N>
void * __cdecl alignedOffsetReallocHook(void* memblock, size_t size, size_t alignment, size_t offset){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedOffsetReallocs[N](memblock, size, alignment, offset);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedOffsetRecalloc hook function. Templated so we can hook many _aligned_offset_recallocs.
template <int N>
void * __cdecl alignedOffsetRecallocHook(void* memblock, size_t num, size_t size, size_t alignment, size_t offset){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedOffsetRecallocs[N](memblock, num, size, alignment, offset);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, num * size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, num * size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedRealloc hook function. Templated so we can hook many _aligned_reallocs.
template <int N>
void * __cdecl alignedReallocHook(void* memblock, size_t size, size_t alignment){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedReallocs[N](memblock, size, alignment);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedRecalloc hook function. Templated so we can hook many _aligned_recallocs.
template <int N>
void * __cdecl alignedRecallocHook(void* memblock, size_t num, size_t size, size_t alignment){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedRecallocs[N](memblock, num, size, alignment);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, num * size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, num * size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedFreeDbg hook function.
template <int N>
void  __cdecl alignedFreeDbgHook(void * p){
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		originalAlignedFreeDbgs[N](p);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			//trace.trace();
			heapProfiler->free(p, trace);
		}
	}
	SetLastError(lastError);
}

// AlignedMallocDbg hook function. Templated so we can hook many _aligned_mallocs.
template <int N>
void * __cdecl alignedMallocDbgHook(size_t size, size_t alignment, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedMallocDbgs[N](size, alignment, filename, linenumber);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			trace.trace();
			heapProfiler->malloc(p, size, trace);
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedOffsetMallocDbg hook function. Templated so we can hook many _aligned_offset_malloc_dbgs.
template <int N>
void * __cdecl alignedOffsetMallocDbgHook(size_t size, size_t alignment, size_t offset, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedOffsetMallocDbgs[N](size, alignment, offset, filename, linenumber);
		lastError = GetLastError();
		if(preventSelfProfile.shouldProfile()){
			StackTrace trace;
			trace.trace();
			heapProfiler->malloc(p, size, trace);
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedOffsetReallocDbg hook function. Templated so we can hook many _aligned_offset_realloc_dbgs.
template <int N>
void * __cdecl alignedOffsetReallocDbgHook(void* memblock, size_t size, size_t alignment, size_t offset, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedOffsetReallocDbgs[N](memblock, size, alignment, offset, filename, linenumber);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedOffsetRecallocDbg hook function. Templated so we can hook many _aligned_offset_recalloc_dbgs.
template <int N>
void * __cdecl alignedOffsetRecallocDbgHook(void* memblock, size_t num, size_t size, size_t alignment, size_t offset, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedOffsetRecallocDbgs[N](memblock, num, size, alignment, offset, filename, linenumber);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, num * size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, num * size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedReallocDbg hook function. Templated so we can hook many _aligned_realloc_dbgs.
template <int N>
void * __cdecl alignedReallocDbgHook(void* memblock, size_t size, size_t alignment, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedReallocDbgs[N](memblock, size, alignment, filename, linenumber);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// AlignedRecallocDbg hook function. Templated so we can hook many _aligned_recalloc_dbgs.
template <int N>
void * __cdecl alignedRecallocDbgHook(void* memblock, size_t num, size_t size, size_t alignment, const char* filename, int linenumber){
	void * p;
	DWORD lastError;
	{
		PreventSelfProfile preventSelfProfile;

		p = originalAlignedRecallocDbgs[N](memblock, num, size, alignment, filename, linenumber);
		lastError = GetLastError();
		if (memblock == NULL){
			// memblock == NULL -> call malloc()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->malloc(p, num * size, trace);
			}
		}
		else if (size == 0){
			// size == 0 -> call free()
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				//trace.trace();
				heapProfiler->free(memblock, trace);
			}
		}
		else if (p == NULL){
			// p == NULL -> no memory, memblock not touched
		}
		else {
			if(preventSelfProfile.shouldProfile()){
				StackTrace trace;
				trace.trace();
				heapProfiler->free(memblock, trace);
				heapProfiler->malloc(p, num * size, trace);
			}
		}
	}
	SetLastError(lastError);

	return p;
}

// Template recursion to init a hook table.
template<int N> struct InitNHooks{
	static void initHook(){
		InitNHooks<N-1>::initHook();  // Compile time recursion. 

		mallocHooks[N-1] = &mallocHook<N-1>;
		freeHooks[N-1] = &freeHook<N-1>;
		reallocHooks[N-1] = &reallocHook<N-1>;
		callocHooks[N-1] = &callocHook<N-1>;
		recallocHooks[N-1] = &recallocHook<N-1>;
		callocDbgHooks[N-1] = &callocDbgHook<N-1>;
		mallocDbgHooks[N-1] = &mallocDbgHook<N-1>;
		reallocDbgHooks[N-1] = &reallocDbgHook<N-1>;
		recallocDbgHooks[N-1] = &recallocDbgHook<N-1>;
		freeDbgHooks[N-1] = &freeDbgHook<N-1>;

		alignedFreeHooks[N-1] = &alignedFreeHook<N-1>;
		alignedMallocHooks[N-1] = &alignedMallocHook<N-1>;
		alignedOffsetMallocHooks[N-1] = &alignedOffsetMallocHook<N-1>;
		alignedOffsetReallocHooks[N-1] = &alignedOffsetReallocHook<N-1>;
		alignedOffsetRecallocHooks[N-1] = &alignedOffsetRecallocHook<N-1>;
		alignedReallocHooks[N-1] = &alignedReallocHook<N-1>;
		alignedRecallocHooks[N-1] = &alignedRecallocHook<N-1>;
		alignedFreeDbgHooks[N-1] = &alignedFreeDbgHook<N-1>;
		alignedMallocDbgHooks[N-1] = &alignedMallocDbgHook<N-1>;
		alignedOffsetMallocDbgHooks[N-1] = &alignedOffsetMallocDbgHook<N-1>;
		alignedOffsetReallocDbgHooks[N-1] = &alignedOffsetReallocDbgHook<N-1>;
		alignedOffsetRecallocDbgHooks[N-1] = &alignedOffsetRecallocDbgHook<N-1>;
		alignedReallocDbgHooks[N-1] = &alignedReallocDbgHook<N-1>;
		alignedRecallocDbgHooks[N-1] = &alignedRecallocDbgHook<N-1>;
	}
};
 
template<> struct InitNHooks<0>{
	static void initHook(){
		// stop the recursion
	}
};

// Internal function to reverse string buffer
static void internal_reverse(char str[], int length){
	int start = 0;
	int end = length -1;
	while (start < end){
		char c = str[start];
		str[start] = str[end];
		str[end] = c;

		start++;
		end--;
	}
}

// Internal itoa()
static char* internal_itoa(__int64 num, char* str, int base){
	int i = 0;
	bool isNegative = false;
 
	// Handle 0 explicitely, otherwise empty string is printed for 0
	if (num == 0){
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}
 
	// In standard itoa(), negative numbers are handled only with 
	// base 10. Otherwise numbers are considered unsigned.
	if (num < 0 && base == 10){
		isNegative = true;
		num = -num;
	}
 
	// Process individual digits
	while (num != 0){
		int rem = num % base;
		str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
		num = num/base;
	}
 
	// If number is negative, append '-'
	if (isNegative)
		str[i++] = '-';

	str[i] = '\0'; // Append string terminator

	// Reverse the string
	internal_reverse(str, i);

	return str;
}

// Internal function to write inject log to InjectLog.txt
void InjectLog(const char* szStr1, const char* szStr2=NULL, const char* szStr3=NULL, const char* szStr4=NULL, const char* szStr5=NULL){
	HANDLE hFile = CreateFileA("InjectLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE){
		LARGE_INTEGER lEndOfFilePointer;
		LARGE_INTEGER lTemp;
		lTemp.QuadPart = 0;
		SetFilePointerEx(hFile, lTemp, &lEndOfFilePointer, FILE_END);

		DWORD dwByteWritten;
		WriteFile(hFile, szStr1, strlen(szStr1), &dwByteWritten, NULL);
		if (szStr2 != NULL)
			WriteFile(hFile, szStr2, strlen(szStr2), &dwByteWritten, NULL);
		if (szStr3 != NULL)
			WriteFile(hFile, szStr3, strlen(szStr3), &dwByteWritten, NULL);
		if (szStr4 != NULL)
			WriteFile(hFile, szStr4, strlen(szStr4), &dwByteWritten, NULL);
		if (szStr5 != NULL)
			WriteFile(hFile, szStr5, strlen(szStr5), &dwByteWritten, NULL);
		CloseHandle(hFile);
	}
}

// Callback which recieves addresses for mallocs/frees which we hook.
BOOL CALLBACK enumSymbolsCallback(PSYMBOL_INFO symbolInfo, ULONG symbolSize, PVOID userContext){
	lock_guard lk(hookTableMutex);
	PreventSelfProfile preventSelfProfile;

	PCSTR moduleName = (PCSTR)userContext;
	char logBuffer[30];

#define GEN_HOOK(func, nUsedHooks, HookTable, OrgHookTable)\
	do {\
		if(strcmp(symbolInfo->Name, #func) == 0){\
			unsigned char dwInstructionBytes[2] = { 0 };\
			memcpy(&dwInstructionBytes, (void*)symbolInfo->Address, sizeof(dwInstructionBytes));\
			if(dwInstructionBytes[0] == 0xFF && dwInstructionBytes[1] == 0x25)\
			{\
				InjectLog("Not hooking " #func " from module ", moduleName, "\r\n");\
				break;\
			}\
			if(nUsedHooks >= numHooks){\
				InjectLog("All " #func " hooks used up!\r\n");\
				break;\
			}\
			MH_STATUS ret = MH_CreateHook((void*)symbolInfo->Address, HookTable[nUsedHooks],  (void **)&OrgHookTable[nUsedHooks]);\
			if(ret == MH_ERROR_ALREADY_CREATED){\
				InjectLog("Already hooked " #func " from module ", moduleName, "\r\n");\
				break;\
			}\
			else if(ret == MH_OK){\
				internal_itoa(nUsedHooks, logBuffer, 10);\
				InjectLog("Hooking " #func " from module ", moduleName, " into " #func " hook num ", logBuffer, ".\r\n");\
			}else {\
				internal_itoa(ret, logBuffer, 10);\
				InjectLog("Create hook " #func " failed! error=", logBuffer, "\r\n");\
			}\
		\
			ret = MH_EnableHook((void*)symbolInfo->Address);\
			if(ret != MH_OK){\
				internal_itoa(ret, logBuffer, 10);\
				InjectLog("Enable " #func " hook failed! error=", logBuffer, "\r\n");\
			}\
		\
			nUsedHooks++;\
		}\
	} while (0)

	GEN_HOOK(malloc, nUsedMallocHooks, mallocHooks, originalMallocs);
	GEN_HOOK(free, nUsedFreeHooks, freeHooks, originalFrees);
	GEN_HOOK(realloc, nUsedReallocHooks, reallocHooks, originalReallocs);
	GEN_HOOK(calloc, nUsedCallocHooks, callocHooks, originalCallocs);
	GEN_HOOK(_recalloc, nUsedRecallocHooks, recallocHooks, originalRecallocs);
	GEN_HOOK(_malloc_dbg, nUsedMallocDbgHooks, mallocDbgHooks, originalMallocDbgs);
	GEN_HOOK(_realloc_dbg, nUsedReallocDbgHooks, reallocDbgHooks, originalReallocDbgs);
	GEN_HOOK(_calloc_dbg, nUsedCallocDbgHooks, callocDbgHooks, originalCallocDbgs);
	GEN_HOOK(_recalloc_dbg, nUsedRecallocDbgHooks, recallocDbgHooks, originalRecallocDbgs);
	GEN_HOOK(_free_dbg, nUsedFreeDbgHooks, freeDbgHooks, originalFreeDbgs);
	GEN_HOOK(_aligned_free, nUsedAlignedFreeHooks, alignedFreeHooks, originalAlignedFrees);
	GEN_HOOK(_aligned_malloc, nUsedAlignedMallocHooks, alignedMallocHooks, originalAlignedMallocs);
	GEN_HOOK(_aligned_offset_malloc, nUsedAlignedOffsetMallocHooks, alignedOffsetMallocHooks, originalAlignedOffsetMallocs);
	GEN_HOOK(_aligned_offset_realloc, nUsedAlignedOffsetReallocHooks, alignedOffsetReallocHooks, originalAlignedOffsetReallocs);
	GEN_HOOK(_aligned_offset_recalloc, nUsedAlignedOffsetRecallocHooks, alignedOffsetRecallocHooks, originalAlignedOffsetRecallocs);
	GEN_HOOK(_aligned_realloc, nUsedAlignedReallocHooks, alignedReallocHooks, originalAlignedReallocs);
	GEN_HOOK(_aligned_recalloc, nUsedAlignedRecallocHooks, alignedRecallocHooks, originalAlignedRecallocs);
	GEN_HOOK(_aligned_free_dbg, nUsedAlignedFreeDbgHooks, alignedFreeDbgHooks, originalAlignedFreeDbgs);
	GEN_HOOK(_aligned_malloc_dbg, nUsedAlignedMallocDbgHooks, alignedMallocDbgHooks, originalAlignedMallocDbgs);
	GEN_HOOK(_aligned_offset_malloc_dbg, nUsedAlignedOffsetMallocDbgHooks, alignedOffsetMallocDbgHooks, originalAlignedOffsetMallocDbgs);
	GEN_HOOK(_aligned_offset_realloc_dbg, nUsedAlignedOffsetReallocDbgHooks, alignedOffsetReallocDbgHooks, originalAlignedOffsetReallocDbgs);
	GEN_HOOK(_aligned_realloc_dbg, nUsedAlignedReallocDbgHooks, alignedReallocDbgHooks, originalAlignedReallocDbgs);
	GEN_HOOK(_aligned_recalloc_dbg, nUsedAlignedRecallocDbgHooks, alignedRecallocDbgHooks, originalAlignedRecallocDbgs);

	return true;
}

// Callback which recieves loaded module names which we search for malloc/frees to hook.
BOOL CALLBACK enumModulesCallback(PCSTR ModuleName, DWORD_PTR BaseOfDll, PVOID UserContext){
	if (BaseOfDll == (DWORD_PTR)hDllModule)
		return true;

	{
	lock_guard lk(hookedDllSetMutex);
	if (hookedDllSet.find(ModuleName) != hookedDllSet.end())
		return true;
	}
	HANDLE currentProcess = hCurrentProcess;
	static const char* symList[] = { "malloc", "free", "realloc", "calloc", "_recalloc",
							  "_malloc_dbg", "_free_dbg", "_realloc_dbg", "_calloc_dbg", "_recalloc_dbg",
							  "_aligned_malloc", "_aligned_free", "_aligned_realloc", "_aligned_recalloc",
							  "_aligned_malloc_dbg", "_aligned_free_dbg", "_aligned_realloc_dbg", "_aligned_recalloc_dbg",
							  "_aligned_offset_malloc", "_aligned_offset_realloc", "_aligned_offset_recalloc",
							  "_aligned_offset_malloc_dbg", "_aligned_offset_realloc_dbg", "_aligned_offset_recalloc_dbg" };

	for(int i=0; i<sizeof(symList)/sizeof(symList[0]); ++i)
	{
		SymEnumSymbols(currentProcess, BaseOfDll, symList[i], enumSymbolsCallback, (void*)ModuleName);
	}

	lock_guard lk(hookedDllSetMutex);
	hookedDllSet.insert(ModuleName);
	return true;
}

BOOLEAN NTAPI _LdrpCallInitRoutine(PDLL_INIT_ROUTINE entryPoint, PVOID baseAddress, ULONG reason, PVOID context)
{
	if(reason == DLL_PROCESS_ATTACH){
		DWORD dwLastError = GetLastError();
		PreventSelfProfile preventSelfProfile;

		DWORD_PTR dllBaseAddress = (DWORD_PTR)baseAddress;
		DWORD dwModuleNameLen = 0;
		DWORD dwLen = 0;
		char* moduleName = NULL;
		do{
			dwModuleNameLen += MAX_PATH;
			HeapFree(GetProcessHeap(), 0, moduleName);
			moduleName = (char*)HeapAlloc(GetProcessHeap(), 0, dwModuleNameLen);
			if (moduleName == NULL)
				break;
			dwLen = GetModuleFileNameA((HMODULE)baseAddress, moduleName, dwModuleNameLen);
		} while (dwLen >= dwModuleNameLen);

		if(moduleName != NULL){
			lock_guard lk(dbgHelpMutex);
			if(SymLoadModuleEx(hCurrentProcess, NULL, moduleName, NULL, dllBaseAddress, 0, NULL, 0) != 0){
				enumModulesCallback(moduleName, dllBaseAddress, NULL);
			}
			HeapFree(GetProcessHeap(), 0, moduleName);
		}
		SetLastError(dwLastError);
	}
	BOOLEAN ret = orgLdrpCallInitRoutine(entryPoint, baseAddress, reason, context);
	return ret;
}

BOOL CALLBACK enumNtdllSymbolsCallback(PSYMBOL_INFO symbolInfo, ULONG symbolSize, PVOID userContext){
	PCSTR moduleName = (PCSTR)userContext;

	// Hook LdrLoadDll.
	if(strcmp(symbolInfo->Name, "LdrpCallInitRoutine") == 0){
		InjectLog("Hooking LdrpCallInitRoutine from module ", moduleName, ".\r\n");
		if(MH_CreateHook((void*)symbolInfo->Address, _LdrpCallInitRoutine,  (void **)&orgLdrpCallInitRoutine) != MH_OK){
			InjectLog("Create hook LdrpCallInitRoutine failed!\r\n");
		}

		if(MH_EnableHook((void*)symbolInfo->Address) != MH_OK){
			InjectLog("Enable LdrpCallInitRoutine hook failed!\r\n");
		}
	}
	return true;
}

BOOL CALLBACK enumNtdllCallback(PCSTR ModuleName, DWORD_PTR BaseOfDll, PVOID UserContext){
	if (strcmp(ModuleName, "ntdll") == 0)
	{
		SymEnumSymbols(hCurrentProcess, BaseOfDll, "LdrpCallInitRoutine", enumNtdllSymbolsCallback, (void*)ModuleName);
		return false;
	}

	return true;
}

void printTopAllocationReport(int numToPrint, bool profileNumberOfAllocations){
	std::vector<HeapProfiler::CallStackInfo> allocsSortedBySize;
	heapProfiler->getAllocationSiteReport(allocsSortedBySize);

	auto size = [profileNumberOfAllocations](const HeapProfiler::CallStackInfo &i) {
		return profileNumberOfAllocations ? i.n : i.totalSize;
	};

	// Sort retured allocation sites by size of memory allocated, descending.
	std::sort(allocsSortedBySize.begin(), allocsSortedBySize.end(), 
		[size](const HeapProfiler::CallStackInfo &a, const HeapProfiler::CallStackInfo &b){
			return size(a)< size(b);
		}
	);
	

	std::ofstream stream("Heapy_Profile.txt",  std::ios::out | std::ios::app);
	stream << "=======================================\n\n";
	stream << "Printing top allocation points.\n\n";
	// Print top allocations sites in ascending order.
	auto precision = std::setprecision(5);
	size_t totalPrintedAllocSize = 0;
	size_t numPrintedAllocations = 0;
	double bytesInAMegaByte = 1024*1024;
	for(size_t i = (size_t)(std::max)(int64_t(allocsSortedBySize.size())-numToPrint, int64_t(0)); i < allocsSortedBySize.size(); ++i){

		if(size(allocsSortedBySize[i]) == 0)
			continue;

		if(!profileNumberOfAllocations) 
			stream << "Alloc size " << precision << size(allocsSortedBySize[i])/bytesInAMegaByte << "Mb, stack trace: \n";
		else
			stream << "Number of allocs " << size(allocsSortedBySize[i]) << ", stack trace: \n";

		allocsSortedBySize[i].trace.print(stream);

		stream << "\n";

		totalPrintedAllocSize += size(allocsSortedBySize[i]);
		numPrintedAllocations++;
	}

	size_t totalAlloctaions = std::accumulate(allocsSortedBySize.begin(), allocsSortedBySize.end(), size_t(0),
		[size](size_t a,  const HeapProfiler::CallStackInfo &b){
			return a + size(b);
		}
	);

	if (!profileNumberOfAllocations) {
		stream << "Top " << numPrintedAllocations << " allocations: " << precision << totalPrintedAllocSize/bytesInAMegaByte << "Mb\n";
		stream << "Total allocations: " << precision << totalAlloctaions / bytesInAMegaByte << "Mb" <<
			" (difference between total and top " << numPrintedAllocations << " allocations : " << (totalAlloctaions - totalPrintedAllocSize)/bytesInAMegaByte << "Mb)\n\n";
	}else {
		stream << "Top " << numPrintedAllocations << " allocations: " << precision << totalPrintedAllocSize<< "\n";
		stream << "Total number of allocations: " <<  totalAlloctaions  <<
			" (difference between total and top " << numPrintedAllocations << " number of allocations : " << (totalAlloctaions - totalPrintedAllocSize) << ")\n\n";
	}
}

// Do an allocation report on exit.
// Static data deconstructors are supposed to be called in reverse order of the construction.
// (According to the C++ spec.)
// 
// So this /should/ be called after the staic deconstructors of the injectee application.
// Probably by whatever thread is calling exit. 
// 
// We are well out of the normal use cases here and I wouldn't be suprised if this mechanism
// breaks down in practice. I'm not even that interested in leak detection on exit anyway.
// 
// Also: The end game will be send malloc/free information to a different
// process instead of doing reports the same process - then shutdown issues go away.
// But for now it's more fun to work inside the injected process.
struct CatchExit{
	~CatchExit(){
		PreventSelfProfile p;
		printTopAllocationReport(25, false);
	}
};
CatchExit catchExit;

int heapProfileReportThread(){
	PreventEverProfilingThisThread();
	while(true){
		Sleep(10000); 
		printTopAllocationReport(25, false);
	}
}

void setupHeapProfiling(){
	// We use InjectLog() thoughout injection becasue it's just safer/less troublesome
	// than printf/iostreams for this sort of low-level/hacky/threaded work.
	InjectLog("Injecting library...\r\n");

	nUsedMallocHooks = 0;
	nUsedFreeHooks = 0;
	nUsedReallocHooks = 0;
	nUsedCallocHooks = 0;
	nUsedRecallocHooks = 0;
	nUsedCallocDbgHooks = 0;
	nUsedMallocDbgHooks = 0;
	nUsedReallocDbgHooks = 0;
	nUsedRecallocDbgHooks = 0;
	nUsedFreeDbgHooks = 0;
	nUsedAlignedFreeHooks = 0;
	nUsedAlignedMallocHooks = 0;
	nUsedAlignedOffsetMallocHooks = 0;
	nUsedAlignedOffsetReallocHooks = 0;
	nUsedAlignedOffsetRecallocHooks = 0;
	nUsedAlignedReallocHooks = 0;
	nUsedAlignedRecallocHooks = 0;
	nUsedAlignedFreeDbgHooks = 0;
	nUsedAlignedMallocDbgHooks = 0;
	nUsedAlignedOffsetMallocDbgHooks = 0;
	nUsedAlignedOffsetReallocDbgHooks = 0;
	nUsedAlignedOffsetRecallocDbgHooks = 0;
	nUsedAlignedReallocDbgHooks = 0;
	nUsedAlignedRecallocDbgHooks = 0;

	tlsIndex = TlsAlloc();
	TlsSetValue(tlsIndex, (LPVOID)0);
	PreventEverProfilingThisThread();

	// Create our hook pointer tables using template meta programming fu.
	InitNHooks<numHooks>::initHook(); 

	// Init min hook framework.
	MH_Initialize(); 

	DWORD dwSymOptions = SymGetOptions();
#ifdef _WIN64
	// dwSymOptions == SYMOPT_UNDNAME by default.
	// On x86, __cdecl function symbol name is prefixed by '_' and DbgHelp can
	// find symbols like _aligned_malloc with SYMOPT_UNDNAME enabled.
	// On x64, there is no prefix for __cdecl function symbol name but DbgHelp 
	// seems to strip the leading '_' for symbols like _aligned_malloc during
	// enumeration with SYMOPT_UNDNAME enabled. So disable SYMOPT_UNDNAME on
	// this platform.
	dwSymOptions &= ~SYMOPT_UNDNAME;
#endif
	dwSymOptions |= SYMOPT_AUTO_PUBLICS;
	SymSetOptions(dwSymOptions);

	HANDLE hProcess = GetCurrentProcess();
	if (!DuplicateHandle(hProcess, hProcess, hProcess, &hCurrentProcess, 0, FALSE, DUPLICATE_SAME_ACCESS)){
		InjectLog("DuplicateHandle failed\n");
		return;
	}

	// Init dbghelp framework.
	if(!SymInitialize(hCurrentProcess, NULL, true))
		InjectLog("SymInitialize failed\n");

	// Yes this leaks - cleauing it up at application exit has zero real benefit.
	// Might be able to clean it up on CatchExit but I don't see the point.
	void* p = HeapAlloc(GetProcessHeap(), 0, sizeof(HeapProfiler));
	heapProfiler = new(p) HeapProfiler();

	// Trawl though loaded modules and hook any mallocs, frees, reallocs and callocs we find.
	SymEnumerateModules(hCurrentProcess, enumModulesCallback, NULL);

	// Catch any dlls that are dynamically loaded later
	SymEnumerateModules(hCurrentProcess, enumNtdllCallback, NULL);

	// Spawn and a new thread which prints allocation report every 10 seconds.
	//
	// We can't use std::thread here because of deadlock issues which can happen 
	// when creating a thread in dllmain.
	// Some background: http://blogs.msdn.com/b/oldnewthing/archive/2007/09/04/4731478.aspx
	//
	// We have to create a new thread because we "signal" back to the injector that it is
	// safe to resume the injectees main thread by terminating the injected thread.
	// (The injected thread ran LoadLibrary so got us unti DllMain DLL_PROCESS_ATTACH.)
	//
	// TODO: Could we signal a different way, or awake the main thread from the dll thread
	// and do the reports from the injeted thread. This was what EasyHook was doing.
	// I feel like that might have some benefits (more stable?)
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&heapProfileReportThread, NULL, 0, NULL);
}

extern "C"{

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reasonForCall, LPVOID lpReserved){
	switch (reasonForCall){
		case DLL_PROCESS_ATTACH:
			hDllModule = (HMODULE)hModule;
			setupHeapProfiling();
		break;
		case DLL_THREAD_ATTACH:
		break;
		case DLL_THREAD_DETACH:
		break;
		case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

}