#ifndef BINHEAP_H
#define BINHEAP_H

#include "stdtypes.h"

typedef struct GenericHashTableImp *GenericHashTable;
typedef struct HashTableImp *HashTable;

typedef int (*BinHeapCmpFp)(void *lhs,void *rhs);

typedef struct BinHeap
{
	BinHeapCmpFp cmp;
	void **elts;
} BinHeap;

BinHeap* binheap_Create( BinHeapCmpFp cmp);
void binheap_Destroy( BinHeap *hItem );
BinHeap *binheap_Copy( BinHeap *hItem );

void *binheap_Top(BinHeap *bh);
void binheap_Push(BinHeap *bh, void *elt);
void *binheap_Peek(BinHeap *bh, int i);
void *binheap_Remove(BinHeap *bh, int i);
void *binheap_RemoveElt(BinHeap *bh, void *elt);
void *binheap_Pop(BinHeap *bh);
int binheap_Find(BinHeap *bh,void *elt);
int binheap_Size(BinHeap *bh);
int binheap_IntCmp(void *lhs, void *rhs);


#endif //BINHEAP_H
