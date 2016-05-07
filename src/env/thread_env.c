// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2016, LH_Mouse. All wrongs reserved.

#include "thread_env.h"
#include "mcfwin.h"
#include "avl_tree.h"
#include "../ext/assert.h"
#include <stdlib.h>

static volatile uintptr_t g_uKeyCounter = 0;

typedef struct tagTlsObject {
	_MCFCRT_AvlNodeHeader avlhNodeByKey;
	struct tagTlsKey *pKey;
	uintptr_t uCounter;

	_MCFCRT_TlsDestructor pfnDestructor;
	intptr_t nContext;

	struct tagTlsThread *pThread;
	struct tagTlsObject *pPrevByThread;
	struct tagTlsObject *pNextByThread;

	alignas(max_align_t) unsigned char abyStorage[];
} TlsObject;

static inline int CompareTlsKeys(const struct tagTlsKey *pKey1, const struct tagTlsKey *pKey2){
	const uintptr_t u1 = (uintptr_t)pKey1, u2 = (uintptr_t)pKey2;
	if(u1 < u2){
		return -1;
	} else if(u1 > u2){
		return 1;
	}
	return 0;
}

static inline int ObjectComparatorNodeKey(const _MCFCRT_AvlNodeHeader *pObj1, intptr_t nKey2){
	return CompareTlsKeys(((const TlsObject *)pObj1)->pKey,
	                      (struct tagTlsKey *)nKey2);
}
static inline int ObjectComparatorNodes(const _MCFCRT_AvlNodeHeader *pObj1, const _MCFCRT_AvlNodeHeader *pObj2){
	return CompareTlsKeys(((const TlsObject *)pObj1)->pKey,
	                      ((const TlsObject *)pObj2)->pKey);
}

typedef struct tagTlsThread {
	_MCFCRT_AvlRoot avlObjects;
	struct tagTlsObject *pFirstByThread;
	struct tagTlsObject *pLastByThread;
} TlsThread;

static DWORD g_dwTlsIndex = TLS_OUT_OF_INDEXES;

static TlsThread *GetTlsForCurrentThread(void){
	_MCFCRT_ASSERT(g_dwTlsIndex != TLS_OUT_OF_INDEXES);

	TlsThread *const pThread = TlsGetValue(g_dwTlsIndex);
	return pThread;
}
static TlsThread *RequireTlsForCurrentThread(void){
	TlsThread *pThread = GetTlsForCurrentThread();
	if(!pThread){
		pThread = malloc(sizeof(TlsThread));
		if(!pThread){
			return nullptr;
		}
		pThread->avlObjects     = nullptr;
		pThread->pFirstByThread = nullptr;
		pThread->pLastByThread  = nullptr;

		const bool bSucceeded = TlsSetValue(g_dwTlsIndex, pThread);
		_MCFCRT_ASSERT(bSucceeded);
	}
	return pThread;
}

typedef struct tagTlsKey {
	uintptr_t uCounter;

	size_t uSize;
	_MCFCRT_TlsConstructor pfnConstructor;
	_MCFCRT_TlsDestructor pfnDestructor;
	intptr_t nContext;
} TlsKey;

TlsObject *GetTlsObject(TlsThread *pThread, TlsKey *pKey){
	_MCFCRT_ASSERT(pThread);

	if(!pKey){
		return nullptr;
	}

	TlsObject *const pObject = (TlsObject *)_MCFCRT_AvlFind(&(pThread->avlObjects), (intptr_t)pKey, &ObjectComparatorNodeKey);
	if(pObject && (pObject->uCounter != pKey->uCounter)){
		const _MCFCRT_TlsDestructor pfnDestructor = pObject->pfnDestructor;
		if(pfnDestructor){
			(*pfnDestructor)(pObject->nContext, pObject->abyStorage);
		}

		TlsObject *const pPrev = pObject->pPrevByThread;
		TlsObject *const pNext = pObject->pNextByThread;
		if(pPrev){
			pPrev->pNextByThread = pNext;
		}
		if(pNext){
			pNext->pPrevByThread = pPrev;
		}

		_MCFCRT_AvlDetach((_MCFCRT_AvlNodeHeader *)pObject);

		free(pObject);
		return nullptr;
	}
	return pObject;
}
TlsObject *RequireTlsObject(TlsThread *pThread, TlsKey *pKey, size_t uSize, _MCFCRT_TlsConstructor pfnConstructor, _MCFCRT_TlsDestructor pfnDestructor, intptr_t nContext){
	TlsObject *pObject = GetTlsObject(pThread, pKey);
	if(!pObject){
		const size_t uSizeToAlloc = sizeof(TlsObject) + uSize;
		if(uSizeToAlloc < sizeof(TlsObject)){
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return nullptr;
		}
		pObject = malloc(uSizeToAlloc);
		if(!pObject){
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return nullptr;
		}
#ifndef NDEBUG
		memset(pObject, 0xAA, sizeof(TlsObject));
#endif
		memset(pObject->abyStorage, 0, uSize);

		if(pfnConstructor){
			const DWORD dwErrorCode = (*pfnConstructor)(nContext, pObject->abyStorage);
			if(dwErrorCode != 0){
				free(pObject);
				SetLastError(dwErrorCode);
				return nullptr;
			}
		}

		pObject->pfnDestructor = pfnDestructor;
		pObject->nContext      = nContext;

		TlsObject *const pPrev = pThread->pLastByThread;
		TlsObject *const pNext = nullptr;
		if(pPrev){
			pPrev->pNextByThread = pObject;
		} else {
			pThread->pFirstByThread = pObject;
		}
		if(pNext){
			pNext->pPrevByThread = pObject;
		} else {
			pThread->pLastByThread = pObject;
		}
		pObject->pThread = pThread;
		pObject->pPrevByThread = pPrev;
		pObject->pNextByThread = pNext;

		if(pKey){
			pObject->pKey = pKey;
			pObject->uCounter = pKey->uCounter;

			_MCFCRT_AvlAttach(&(pThread->avlObjects), (_MCFCRT_AvlNodeHeader *)pObject, &ObjectComparatorNodes);
		}
	}
	return pObject;
}

bool __MCFCRT_ThreadEnvInit(void){
	const DWORD dwTlsIndex = TlsAlloc();
	if(dwTlsIndex == TLS_OUT_OF_INDEXES){
		return false;
	}

	g_dwTlsIndex = dwTlsIndex;
	return true;
}
void __MCFCRT_ThreadEnvUninit(void){
	const DWORD dwTlsIndex = g_dwTlsIndex;
	g_dwTlsIndex = TLS_OUT_OF_INDEXES;

	const bool bSucceeded = TlsFree(dwTlsIndex);
	_MCFCRT_ASSERT(bSucceeded);
}

void __MCFCRT_TlsCleanup(void){
	TlsThread *const pThread = GetTlsForCurrentThread();
	if(!pThread){
		return;
	}

	TlsObject *pObject = pThread->pLastByThread;
	while(pObject){
		const _MCFCRT_TlsDestructor pfnDestructor = pObject->pfnDestructor;
		if(pfnDestructor){
			(*pfnDestructor)(pObject->nContext, pObject->abyStorage);
		}

		TlsObject *const pPrevByThread = pObject->pPrevByThread;
		free(pObject);
		pObject = pPrevByThread;
	}

	const bool bSucceeded = TlsSetValue(g_dwTlsIndex, nullptr);
	_MCFCRT_ASSERT(bSucceeded);
	free(pThread);
}

_MCFCRT_TlsKeyHandle _MCFCRT_TlsAllocKey(size_t uSize, _MCFCRT_TlsConstructor pfnConstructor, _MCFCRT_TlsDestructor pfnDestructor, intptr_t nContext){
	TlsKey *const pKey = malloc(sizeof(TlsKey));
	if(!pKey){
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return nullptr;
	}
	pKey->uCounter       = __atomic_add_fetch(&g_uKeyCounter, 1, __ATOMIC_RELAXED);
	pKey->uSize          = uSize;
	pKey->pfnConstructor = pfnConstructor;
	pKey->pfnDestructor  = pfnDestructor;
	pKey->nContext       = nContext;

	return (_MCFCRT_TlsKeyHandle)pKey;
}
bool _MCFCRT_TlsFreeKey(_MCFCRT_TlsKeyHandle hTlsKey){
	TlsKey *const pKey = (TlsKey *)hTlsKey;
	if(!pKey){
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}

	free(pKey);
	return true;
}

size_t _MCFCRT_TlsGetSize(_MCFCRT_TlsKeyHandle hTlsKey){
	TlsKey *const pKey = (TlsKey *)hTlsKey;
	return pKey->uSize;
}
_MCFCRT_TlsConstructor _MCFCRT_TlsGetConstructor(_MCFCRT_TlsKeyHandle hTlsKey){
	TlsKey *const pKey = (TlsKey *)hTlsKey;
	return pKey->pfnConstructor;
}
_MCFCRT_TlsDestructor _MCFCRT_TlsGetDestructor(_MCFCRT_TlsKeyHandle hTlsKey){
	TlsKey *const pKey = (TlsKey *)hTlsKey;
	return pKey->pfnDestructor;
}
intptr_t _MCFCRT_TlsGetContext(_MCFCRT_TlsKeyHandle hTlsKey){
	TlsKey *const pKey = (TlsKey *)hTlsKey;
	return pKey->nContext;
}

bool _MCFCRT_TlsGet(_MCFCRT_TlsKeyHandle hTlsKey, void **restrict ppStorage){
	*ppStorage = nullptr;

	TlsKey *const pKey = (TlsKey *)hTlsKey;
	if(!pKey){
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}
	TlsThread *const pThread = GetTlsForCurrentThread();
	if(!pThread){
		return true;
	}
	TlsObject *const pObject = GetTlsObject(pThread, pKey);
	if(!pObject){
		return true;
	}
	*ppStorage = pObject->abyStorage;
	return true;
}
bool _MCFCRT_TlsRequire(_MCFCRT_TlsKeyHandle hTlsKey, void **restrict ppStorage){
	*ppStorage = nullptr;

	TlsKey *const pKey = (TlsKey *)hTlsKey;
	if(!pKey){
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}
	TlsThread *const pThread = RequireTlsForCurrentThread();
	if(!pThread){
		return false;
	}
	TlsObject *const pObject = RequireTlsObject(pThread, pKey, pKey->uSize, pKey->pfnConstructor, pKey->pfnDestructor, pKey->nContext);
	if(!pObject){
		return false;
	}
	*ppStorage = pObject->abyStorage;
	return true;
}

static void CrtAtExitThreadDestructor(intptr_t nContext, void *pStorage){
	const _MCFCRT_AtThreadExitCallback pfnProc = *(_MCFCRT_AtThreadExitCallback *)pStorage;
	(*pfnProc)(nContext);
}

bool _MCFCRT_AtThreadExit(_MCFCRT_AtThreadExitCallback pfnProc, intptr_t nContext){
	TlsThread *const pThread = RequireTlsForCurrentThread();
	if(!pThread){
		return false;
	}
	TlsObject *const pObject = RequireTlsObject(pThread, nullptr, sizeof(pfnProc), nullptr, &CrtAtExitThreadDestructor, nContext);
	if(!pObject){
		return false;
	}
	void *const pStorage = pObject->abyStorage;
	memcpy(pStorage, &pfnProc, sizeof(pfnProc));
	return true;
}
