#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "storage_mgr.h"
#include "buffer_mgr.h"

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		const int numPages, ReplacementStrategy strategy,
		void *stratData){
		
  //Initialize parameters
  bm->mgmtData = NULL;
  SM_FileHandle fh;
  if(openPageFile(pageFileName,&fh)!=RC_OK) //open page file
  {
    return RC_FILE_NOT_FOUND;
  }
  //ensureCapacity(numPages,&fh);
  bm->pageFile = pageFileName;
  bm->numPages = numPages;
  bm->strategy = strategy;

  //Make and initialize an array of page frames
  BM_PageFrame * pageFrameArray;
  pageFrameArray = malloc(sizeof(BM_PageFrame)*numPages);
  for(int x = 0; x<numPages; x++){
    pageFrameArray[x].dirtyBit = false;
    pageFrameArray[x].pinCount = 0;
    pageFrameArray[x].page = NO_PAGE;
    pageFrameArray[x].data = NULL;
  }
  
  //Make and initialize a pagetable
  BM_PageTable * pageTable;
  pageTable = malloc(sizeof(BM_PageTable));
  pageTable->pageFrame = pageFrameArray;
  pageTable->frameCount = 0;
  pageTable->numReadIO = 0;
  pageTable->numWriteIO = 0;
  pageTable->fileHandle = fh;
  pageTable->contents = initFrameContents(bm);
  pageTable->dirtyBits = initDirtyFlags(bm);
  pageTable->pins = initFixCounts(bm);
  bm->mgmtData = pageTable;
  
  //create additional data struct depending on replacement strategy type
  if(strategy==RS_FIFO){
    BM_Queue * qStruct = malloc(sizeof(BM_Queue));
    int * queue = malloc(sizeof(int)*numPages);
    qStruct->pointer = 0;
    qStruct->queue = queue;
    qStruct->maxTimeStamp = 0;
    qStruct->tsQueue = NULL;
    pageTable->repStratInfo = qStruct;
  }
  else if(strategy==RS_LRU){
    BM_Queue * qStruct = malloc(sizeof(BM_Queue));
    unsigned long int *tsQueue = malloc(sizeof(unsigned long int)*numPages);
    qStruct->maxTimeStamp = 0;
    qStruct->tsQueue = tsQueue;
    qStruct->pointer = 0;
    qStruct->queue = NULL;
    pageTable->repStratInfo = qStruct;
  }
  printf("Buffer Pool Initialized\n");
  return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm){

  //Verify pool is initialized
  RC err = poolInitialized(bm);
  if(err!=RC_OK){
    return err;
  }

  //get page table and frame array
  BM_PageTable * pt = bm->mgmtData;
  BM_PageFrame * pfArray = pt->pageFrame;
  BM_Queue * q = pt->repStratInfo;
  //determine if any pages are pinned
  for(int x = 0; x < bm->numPages; x++){
    if(!pinCountIsZero(pt,x)){
      printf("ERROR: pinned page exists at index %d\n",x);
      return RC_BUFFER_SD_FAILED;
    }
  }
  
  //Write back all dirty pages to the disk
  forceFlushPool(bm);
  //free sm_pageHandles
  for(int y = 0; y < bm->numPages; y++){
    if(!indexIsEmpty(pt,y)){
      free(pfArray[y].data);
      pfArray[y].data = NULL; //Necesarry?
    }
  }
   
  //Free replacement stragtegy allocs
  if(bm->strategy==RS_FIFO){
    free(q->queue);
    free(q);
    //SET pt->repStratInfo to NULL??
  }
  else if(bm->strategy==RS_LRU){
    free(q->tsQueue);
    free(q);
  }
  pt->repStratInfo = NULL; //Needed
  
  //Free page frame array, close pagefile, free statistic arrays,  free page table, and set buffer manager data struct to NULL
  free(pfArray);
  closePageFile(&(pt->fileHandle));
  free(pt->contents);
  free(pt->dirtyBits);
  free(pt->pins);
  pt->contents=NULL;
  pt->dirtyBits=NULL; //Needed?
  pt->pins=NULL;
  free(pt);
  bm->mgmtData = NULL; 
  
  return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm){

  //Verify bm is initialized
  RC err = poolInitialized(bm);
  if(err!=RC_OK){
    return err;
  }
  
  //If page exists, force page
  BM_PageTable * pt = bm->mgmtData;
  BM_PageFrame * pfArray = pt->pageFrame;
  for(int x = 0; x <bm-> numPages; x++){
    if(!indexIsEmpty(pt,x)){
      if(isDirty(pfArray[x])){
      	safelyWriteBack(bm,&(pfArray[x]),x);
	pfArray[x].dirtyBit = false;
	pt->dirtyBits[x] = false;
      }
    }
  }
  return RC_OK;
}

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){

  //Find page to mark dirty
  BM_PageTable * pt = bm->mgmtData;
  BM_PageFrame * pfArray = pt->pageFrame;
  int index =  findPageIndex(bm,page->pageNum);
  if(index == -1){
    printf("ERROR: PAGE_NOT_FOUND\n");
    return RC_PAGE_NOT_FOUND;
  }
  
  //Mark page as dirty
  setDirtyBit(&(pfArray[index]),true);
  pt->dirtyBits[index] = true;
  return RC_OK;
}

RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){

  //Find page to unpin
  BM_PageTable * pt = bm->mgmtData;
  BM_PageFrame * pfArray = pt->pageFrame;
  int index = findPageIndex(bm,page->pageNum);
  if(index == -1){
    printf("Error: PAGE NOT FOUND\n");
    return RC_PAGE_NOT_FOUND;
  }
  
  //Unpin page
  if(pfArray[index].pinCount-1>=0){
    pfArray[index].pinCount = pfArray[index].pinCount-1;
    pt->pins[index]=pfArray[index].pinCount;
  }
  return RC_OK;
}

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){

  //Find page to force
  int index = findPageIndex(bm,page->pageNum);
  if(index == -1){
    printf("Error: PAGE NOT FOUND\n");
    return RC_PAGE_NOT_FOUND;
  }
  
  //Check if page to be forced is pinned
  BM_PageTable * pt = bm->mgmtData;
  BM_PageFrame * pfArray = pt->pageFrame;
  if(!pinCountIsZero(pt,index)){
    printf("Error: page is currently pinned");
    return RC_PAGE_IS_PINNED; 
  }
  
  //Force page
  if(isDirty(pfArray[index])){
    writeBack(bm,&(pfArray[index]));
    pfArray[index].dirtyBit = false;
    pt->dirtyBits[index] = false;
  }
  return RC_OK;
}

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
		const PageNumber pageNum){
		
  //Verify bm is initialized
  RC err = poolInitialized(bm);
  if(err!=RC_OK){
    return err;
  }
  
  //Verify page num is not negative
  if(pageNum<0){
    printf("ERROR: pageNum requested is negative\n");
    return RC_PAGE_IS_NEGATIVE; 
  }	
  
  //search buffer pool for page and if page is found, set page->data to the pointer to that frame and increase pin count
  BM_PageTable * pt = bm->mgmtData;
  BM_PageFrame * frameArray = pt->pageFrame;
  BM_Queue * q = pt->repStratInfo;
  int destIndex = findPageIndex(bm,pageNum);
  if(destIndex!=-1){
    if(bm->strategy==RS_LRU){
      setLRUIndex(q,destIndex);//
    }
    page->pageNum = frameArray[destIndex].page;
    page->data = frameArray[destIndex].data;
    frameArray[destIndex].pinCount = frameArray[destIndex].pinCount + 1;
    pt->pins[destIndex]=frameArray[destIndex].pinCount;
    return RC_OK;
  }
  
  //Else if page is not found, store info read from disk in new SM_PageHandle, increase readIO count and search for empty index for page
  SM_PageHandle smph;
  smph = (SM_PageHandle)malloc(PAGE_SIZE);
  page->pageNum = pageNum;
  //if(!poolIsFull(bm)){
    ensureCapacity(pageNum+1,&(pt->fileHandle));
  //}
  readBlock(pageNum,&(pt->fileHandle),smph);
  pt->numReadIO = pt->numReadIO + 1;
  
  //If pool is full determine index of what page to evict and save page to be evicted back to disk, else find open index for new page data and increase frame count
  if(poolIsFull(bm)){
    if(bm->strategy==RS_FIFO){
      if((destIndex = fifoIndex(-1,q,bm->numPages,frameArray))==-1){
        return RC_CANNOT_EVICT_PAGE;
      }
    } 
    else if(bm->strategy==RS_LRU){
      if((destIndex = getLRUIndex(q,bm->numPages,frameArray))==-1){
        return RC_CANNOT_EVICT_PAGE;
      }
    }
    safelyWriteBack(bm,&(frameArray[destIndex]),destIndex);
    free(frameArray[destIndex].data);
  }
  else{
    destIndex = findOpenIndex(bm,page);
    pt->frameCount = pt->frameCount+1;
    if(bm->strategy == RS_FIFO){
      fifoIndex(destIndex,q,bm->numPages,frameArray);
    }
    else if(bm->strategy == RS_LRU){
      setLRUIndex(q,destIndex);
    }
  }
  
  //Point page handle to newly allocated data an insert page handle data into frame
  page->data = smph;
  insertPageAtIndex(destIndex,page,bm);

  return RC_OK;
}


int fifoIndex(int index,BM_Queue * q, int qSize, BM_PageFrame * pfArray){

  //Find "First In" index of frame array. If this index pinned, move on to "Second In" index and so on unit an index is found
  int i = index;
  if(i==-1){
    if(pfArray[q->queue[q->pointer]].pinCount==0){
      i = q->queue[q->pointer];
      q->pointer = (q->pointer+1)%qSize;
      return i;
    }
    int iterator = q->pointer+1;
    while(iterator%qSize!=q->pointer){
        if(pfArray[q->queue[iterator]].pinCount==0){
          i = q->queue[iterator];
          //Once index is found, shift all indices after it left by 1 and put the index at the end of the queue array
          while((iterator+1)%qSize!=q->pointer){
            q->queue[iterator]=q->queue[(iterator+1)%qSize];
            iterator++;
          }
          q->queue[(iterator)%qSize] = i;
          return i;
        }
        iterator++;
    }
    
    //If no index found, return error
    printf("ERROR: No suitable replacement found\n");
    return -1; //ERROR CODE TBD
  }
  else{
    q->queue[q->pointer] = i;
  }
  q->pointer = (q->pointer+1)%qSize;
  return i;
}

void setLRUIndex(BM_Queue * q,int index){

  //Increase indexes timestamp
  unsigned long int * indexArray = q->tsQueue;
  indexArray[index] = q->maxTimeStamp;
  q->maxTimeStamp = q->maxTimeStamp+1;
}
  

int getLRUIndex(BM_Queue * q, int qSize, BM_PageFrame * pfArray){

  //Get index with the smalles timestamp
  unsigned long int * indexArray = q->tsQueue;
  unsigned long int  min = ULONG_MAX;
  int minIndex = -1;
  //linear search of cache
  for(int x = 0; x<qSize; x++){
    if(indexArray[x]<min && pfArray[x].pinCount == 0){
      min = indexArray[x];
      minIndex = x;
    }
  }
  if(minIndex==-1){
    printf("ERROR: No suitable replacement index found\n");
  }
  else{
    setLRUIndex(q,minIndex);
  }
  return minIndex;
}
  

// Statistics Interface
PageNumber *getFrameContents(BM_BufferPool *const bm)
{
  BM_PageTable *pageTable = (BM_PageTable *)bm->mgmtData;
  return pageTable->contents;
}

bool *getDirtyFlags(BM_BufferPool *const bm)
{
  BM_PageTable *pageTable = (BM_PageTable *)bm->mgmtData;
  return pageTable->dirtyBits; 
}

int *getFixCounts(BM_BufferPool *const bm)
{ 
  BM_PageTable *pageTable = (BM_PageTable *)bm->mgmtData;
  return pageTable->pins; 
}

PageNumber *initFrameContents(BM_BufferPool *const bm)
{
  PageNumber *frameContents = malloc(sizeof(PageNumber) * bm->numPages);
  int i = 0;
  while (i < bm->numPages) {
    frameContents[i] = -1;
    i++;
  }
  return frameContents; 
}

bool *initDirtyFlags(BM_BufferPool *const bm)
{
  bool *dirtyFlags = malloc(sizeof(bool) * bm->numPages);
  int i;
  for (i = 0; i < bm->numPages; i++) {
    dirtyFlags[i] = false;
  }
  return dirtyFlags; 
}

int *initFixCounts(BM_BufferPool *const bm)
{
  int *fixCounts = malloc(sizeof(int) * bm->numPages);
  int i = 0;
  while (i < bm->numPages) {
    fixCounts[i] = 0;
    i++;
  }
  return fixCounts; 
}

int getNumReadIO(BM_BufferPool *const bm)
{
  BM_PageTable *pageTable = (BM_PageTable *)bm->mgmtData;
  return pageTable->numReadIO;
}

int getNumWriteIO(BM_BufferPool *const bm)
{
  BM_PageTable *pageTable = (BM_PageTable *)bm->mgmtData;
  return pageTable->numWriteIO;
}

//Additional Helper Files

/* given a buffer pool, this function returns whether or not a buffer pool is full.
   It does this by checking the "frameCount" parameter of the buffer pool's page table.
   "frameCount" is the number of pages currently in the pool. "numPages" is the size of the pool
   i.e. the number of frames in the pool. When these two parameters are equal, the pool is full
*/
bool poolIsFull(BM_BufferPool * bm){
  BM_PageTable * pt = bm->mgmtData;
  if(bm->numPages == pt->frameCount){
    return true;
  }
  return false;
}

/*  given a pageHandle (the information to be inserted into a buffer pool), a buffer pool to be inserted into, and 
    an index indicating where in the buffer pools pageFrame array to insert the information, this function
    inserts a pageHandle into a page frame in the buffer pool. This function will overwrite what is currently 
    in the pageFrame array at the given index. After insertion, this function sets the dirty bit to false, sets the pinCount to 1,  and increaes the numReadIO by 1
*/
int insertPageAtIndex(int index, BM_PageHandle * ph, BM_BufferPool * bm){
  int poolSize = bm->numPages; //get number of pages in pool
  //verify index is valid
  if(index<0 || index>=poolSize){
    printf("ERROR: index out of bounds\n");
    return -1;
  }
  BM_PageTable * pt = bm->mgmtData; //Get the buffer pools page table
  BM_PageFrame * pfArray = pt->pageFrame; //get array of page frames
  
  //Transfer data from page handle to page frame initialize page frame
  pfArray[index].data = ph->data; 
  pfArray[index].page = ph->pageNum;
  pfArray[index].dirtyBit = false;
  pfArray[index].pinCount = 1;
  pt->pins[index]=1;
  pt->contents[index]=ph->pageNum;
  pt->dirtyBits[index] = false;
  return 0;
}

/* given a buffer pool and a pageHandle, this function finds an empty space in the bufferPools pageFrame array
   and returns that loction (the index). If the buffer pool is full, -1 is returned. The hash value for searching the 
   pageFrame array is calculated using the modulo function i.e. pageHandle page number  % size of pool. If the hash value returns
   an index in which there is an existing pageHandle, the function linearly probes the frameArray until an empty spot is found.
*/
int findOpenIndex(BM_BufferPool * bm, BM_PageHandle *  ph){
  //determine if pool is full
  if(poolIsFull(bm)){
    return -1;
  }
  //get index to look for from page
  PageNumber pageNo = ph->pageNum;
  //get size of buffer pool
  int poolSize = bm->numPages;
  BM_PageTable * pt = bm->mgmtData; //get the page table for this puffer pool
  //calculate hash value
  int hValue = pageNo%poolSize;
  //Check if pageTable[hValue is empty]
  if(indexIsEmpty(pt,hValue)){
    return hValue;
  }
  int iPointer = hValue+1;
  //use linear probing to find next available open spot in hashtable (pageTable)
  while((iPointer%poolSize)!=hValue){
    if(indexIsEmpty(pt,iPointer%poolSize)){
      break;
    }
    iPointer++;
  }
  return iPointer%poolSize;
}

/* given a buffer pools page table and an index for that tables pageFrame array, this function returns
   whether or not there is any page pinned to a particular page frame. If the pin count of a page frame
   is 0, then it is considered empty.
*/
bool pinCountIsZero(BM_PageTable * pt, int index){
  BM_PageFrame * pf= pt->pageFrame;
  if(pf[index].pinCount == 0){
    return true;
  }
  return false;
}

/* given a buffer pools page table and an index for that tables PageFrame array, this function returns whether or not there
   are any pageHandles associated with a particular page frame. If pageHandle == NULL, then it is considered empty
*/
bool indexIsEmpty(BM_PageTable * pt, int index){
  BM_PageFrame * pf = pt->pageFrame;
  if(pf[index].page == NO_PAGE){
    return true;
  }
  return false;
}

/* given a buffer pool and a page number of a page to search for, this function searches the buffer pools 
   pageFrame array for a frame that contains the page pageNo. It returns the index of the frame in the array.
   If no page is found, -1 is returned
*/
int findPageIndex(BM_BufferPool * bm, PageNumber pageNo){
  //Get page number to find and size of pool
  int poolSize = bm->numPages;
  BM_PageTable * pt = bm->mgmtData; //get the page table for this buffer pool
  //calculate hash value
  int hValue = pageNo%poolSize;
  //find if the page is already in the pool
  if(pageNo == getFramePageNumber(pt,hValue)){
    return hValue;
  }
  int iPointer = hValue+1;
  //use linear probing to find the page in the hashtable
  while((iPointer%poolSize)!=hValue){
    if(pageNo == getFramePageNumber(pt,iPointer%poolSize)){
      return iPointer%poolSize;
    }
    iPointer++;
  }
  //If the particular pageNo not found, return -1
  return -1;
}

/* given a buffer pools page table and an index value for that tables pageFrame array, this function
   returns the page number of the page stored in the frame at that index. If there is no pageHandle stored at
   the frame, then -1 is returned.
*/
PageNumber getFramePageNumber(BM_PageTable * pt, int index){
  if(indexIsEmpty(pt,index)){
    return -1;
  }
  BM_PageFrame * pfArray = pt->pageFrame;
  return pfArray[index].page;
}

bool isDirty(BM_PageFrame pf){
  return pf.dirtyBit;
}

void setDirtyBit(BM_PageFrame * pf, bool status){
  pf->dirtyBit = status;
}

void writeBack(BM_BufferPool * bm, BM_PageFrame * pf){
  BM_PageTable * pt = bm->mgmtData;
  int writePage = pf->page;
  writeBlock(writePage,&(pt->fileHandle),pf->data); //write block back to disk
  pt->numWriteIO = pt->numWriteIO + 1;
}

bool safelyWriteBack(BM_BufferPool * bm, BM_PageFrame * pf, int index){
  BM_PageTable * pt = bm->mgmtData;
  if(!pf->dirtyBit){
    printf("No Operation - page %d not dirty\n",pf->page);
    return false;
  }
  if(pf->pinCount!=0){
    printf("No Operation - page %d is currently pinned\n",pf->page);
    return false;
  }
  writeBack(bm,pf);
  pf->dirtyBit = false;
  pt->dirtyBits[index] = false;
  return true;
}

RC poolInitialized(BM_BufferPool * bm){
  if(bm->mgmtData==NULL){
    printf("ERROR: Attempt to access uninitialized pool"); 
    return RC_POOL_NOT_INIT;
  }
  return RC_OK;
}
  
 
  
  
  
  
  
  

