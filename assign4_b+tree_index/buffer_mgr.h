#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "storage_mgr.h"

// Include return codes and methods for logging errors
#include "dberror.h"

// Include bool DT
#include "dt.h"

// Replacement Strategies
typedef enum ReplacementStrategy {
	RS_FIFO = 0,
	RS_LRU = 1,
	RS_CLOCK = 2,
	RS_LFU = 3,
	RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

typedef struct BM_BufferPool {
	char *pageFile; 
	int numPages; 
	ReplacementStrategy strategy;
	void *mgmtData; // use this one to store the bookkeeping info your buffer
	// manager needs for a buffer pool
} BM_BufferPool;

typedef struct BM_PageHandle {
	PageNumber pageNum;
	char *data;
} BM_PageHandle;

typedef struct BM_PageFrame {
  bool dirtyBit;
  int pinCount;
  PageNumber page;
  char *data;
} BM_PageFrame;

typedef struct BM_PageTable {
  BM_PageFrame * pageFrame; 
  int frameCount;
  int numReadIO;
  int numWriteIO;
  SM_FileHandle fileHandle;
  void *repStratInfo;
  PageNumber *contents;
  bool *dirtyBits;
  int *pins;
} BM_PageTable;

typedef struct BM_Queue {
  int pointer;
  int *queue;
  unsigned long int *tsQueue;
  unsigned long int maxTimeStamp;
} BM_Queue;



// convenience macros
#define MAKE_POOL()					\
		((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()				\
		((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		const int numPages, ReplacementStrategy strategy,
		void *stratData);
RC shutdownBufferPool(BM_BufferPool *const bm);
RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
		const PageNumber pageNum);

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm);
bool *getDirtyFlags (BM_BufferPool *const bm);
int *getFixCounts (BM_BufferPool *const bm);
int getNumReadIO (BM_BufferPool *const bm);
int getNumWriteIO (BM_BufferPool *const bm);

// Additional Functions
extern int insertPageAtIndex(int index, BM_PageHandle * ph, BM_BufferPool * bm);
extern bool poolIsFull(BM_BufferPool * bm);
extern int findOpenIndex(BM_BufferPool * bm, BM_PageHandle * ph);
extern bool indexIsEmpty(BM_PageTable * pt, int index);
extern int findPageIndex(BM_BufferPool * bm, PageNumber pageNo);
extern PageNumber getFramePageNumber(BM_PageTable * pt, int index);
extern bool isDirty(BM_PageFrame pf);
extern void setDirtyBit(BM_PageFrame * pf, bool status);
extern void writeBack(BM_BufferPool * bm, BM_PageFrame * pf);
extern bool pinCountIsZero(BM_PageTable * pt, int index);
extern int fifoIndex(int index,BM_Queue * q, int qSize, BM_PageFrame * pfArray);
extern bool safelyWriteBack(BM_BufferPool * bm, BM_PageFrame * pf, int index);
extern void setLRUIndex(BM_Queue * q,int index);
extern int getLRUIndex(BM_Queue * q, int qSize, BM_PageFrame * pfArray);
extern RC poolInitialized(BM_BufferPool * bm);
extern PageNumber *initFrameContents(BM_BufferPool * bm);
extern bool *initDirtyFlags(BM_BufferPool * bm);
extern int *initFixCounts(BM_BufferPool *bm);

#endif
