BUFFER MANAGER - 3/12/2021

To run our buffer manager, please make use of makefile. Type "make" to compile the code and generate test case.

--------------------------------------------------------------------------------
TEAM MEMBERS:

1. Rene Lamb
2. Li-Han Feng

--------------------------------------------------------------------------------
OVERVIEW:

Implemented a buffer manager manages a fixed number of pages in memory that represent pages from a page file managed by the storage manager implemented in assignment 1. The memory pages managed by the buffer manager are called page frames. We call the combination of a page table, page frame and page handles storing pages from that file a Buffer Pool. The Buffer manager can handle a fixed number of buffer pools at the same time. Each buffer pool uses one page replacement strategy that is determined when the buffer pool is initialized. We implement two replacement strategies: FIFO, LRU. The figure below is an outline of the overall structure of our program. The folloiwn is a description of each parameter.

Buffer Pool:
char *PageFile - Stores name of a pagefile. This is used at initialization to open the file and associate it with a file handle.
int numPages - This is the number of frames in the buffer pool
ReplacementStrategy Strategy - This is the replacement strategy for choosing pages to evict when the buffer pool is full
void *mgmtData - This points to the page table struct that stores the metadata required for navigating the overall system

Page Table:
BM_PageFrame * pageFrame - Stores a pointer to an array of page frames. These frame are initialized upon buffer manager startup
int frameCount - This keeps count of the number of pages in the buffer pool. Initialized as 0 and incremented whenever a page is pinned to a frame (until the pool is full). This is used to determine if a pool is full (when frameCount = numPages)
int numReadIO - This keeps track every time a page is read from a page file on disk and stored in an SM_PageHandle. This occurs when a page is pinned to a frame in the buffer pool
int numWriteIO - This keeps trace of every time the content of a page stored in a frame in the pool is written back to memory. This occurs during evictions, page forces, pool flushes and BM shutdowns
SM_FileHandle fileHandle - This stores the metadata of an a page that is opened upon BM initialization. This is required when reading from disk and writing to disk
void *repStratInfo - This points to the data structures required to manage the buffer pools replacment strategy. In our implementation this point to a buffer manager "queue" which stores the 	metadata and data structure required for the replacment strategies
PageNumber *contents - This points to an array of the page numbers stored within each page frame. These are initialized to all -1 upon buffer pool startup
bool *dirtyBits - This points to an array of dirty bits for each page frame in the page frame array. These are initialized to all "false" upon buffer pool startup
int *pins - This points to an array of fixCounts for each page stored in the page frame array. These are initialized to all 0 upon buffer pool startup

Page Frame Array:
bool dirtyBit - This represents whether a particular frame in the array is dirty or not
int pinCount - This represents how many times the page stored at a particular page frame is pinned by clients
PageNumber page - This represents the number of the page stored in a particular page frame
char *data - This points to an SM_PagHandle object that stores data from a page onto memory. Memory for these page handles are allocated whenever a page is pinned that is not currently in the pool

Buffer Queue:
int pointer - This is used in the FIFO strategy to point to the index of the frame array whose page was the first to be inserted into the pool
int *queue - This is used in the FIFO strategy. It is an array of page frame array index values which represents a FIFO queue. The first index is the front of the queue. The last index is the end.
unsigned long int *tsQueue - This is used in the LRU strategy. It is an array whose indices correspond directly to each page frame number in the page frame array. At each index, a timestamp, represented by a long it is stored. The smaller the timestamp, the less recently a particular frame has been accessed.
unsigned long int maxTimeStamp - This is used in the LRU strategy. It stores maximum timestamp of the array so that when a frame is accesed, it can be easily given assigned a new timestamp (maxTimesStamp + 1)

--------------------------------------------------------------------------------
Structure

|-------------|       |-------------|
| Buffer Pool |   --->| Page Table  |----->Dirty Bit Array
|-------------|   |   |-------------|----->Fix Count Array            --------------
|  *Pagefile  |   |   | framecount  |----->Frame Content Array       | Buffer Queue |
|  numPages   |---|   | numReadIO   |       ------------------------>|--------------|
|  Strategy   |       | *PageFrame--|--     |                        |pointer       |
|  *mgmtData  |       | fileHandle  |  |    |                        |*queue        |
|-------------|       |*repStratInfo|- |----|                        |*tsQueue      | 
                      |-------------|  |                             |maxTimeStamp  |
                      -----------------|                             |--------------|
                      |
                      V
   |-----------------------------  ----------------
   |frame 0 | frame 1 | frame 2 / / frame numpages|
   |---------------------------/ /----------------|
   |dirtyBit|dirtyBit |dirtyBit| | dirtyBit       |
 --|-*data  |*data--- |*data-- | | *data-----     |
 | | page   | page  | | page | | |  page    |     |
 | |----------------|--------|-| |-----------|-----|
 |---               |        |----------    |--------------
    |               |                  |                  |
    V               V                  V                  V
|--------------|  |--------------|  |--------------|  |--------------|
|SM_PageHandle |  |SM_PageHandle |  |SM_PageHandle |  |SM_PageHandle |                                           |
|--------------|  |--------------|  |--------------|  |--------------|       

--------------------------------------------------------------------------------
Bellow is a summary of the functions implemented:

Buffer Pool Functions
1. initBufferPool()

 - Initialize the parameters, an array of page frames, and a page table to create a new buffer pool. Also, create an additional data structure for different replacement strategy types for it.

2. shutdownBufferPool()

 - Destroy a buffer pool by free up all resources associated with buffer pool.

3. forceFlushPool()

 - Write all the dirty pages back to disk.

--------------------------------------------------------------------------------
Page Management Functions
4. markDirty()

 - Marks a page as dirty.

5. pinPage()

 - Pin the page with page number pageNum. Search buffer pool for page and if page is found, set page->data to the pointer to that frame and increase pin count. If page is not found, search for empty index for page. If pool is full, then determine index of what page to evict by using LRU or FIFO strategy and save page to be evicted back to disk, else find open index for new page data and increase frame count.

6. unpinPage()

 - Unpin the page page.

7. forcePage()

 - Write the current content of the page back to the page file on disk.
--------------------------------------------------------------------------------
Statistics Functions
8. getFrameContents()

- Returns an array that represents the page in each page frame. Frames with no page are represented by -1.

9. getDirtyFlags()

 - Returns an array that represents the dirty bit for each page frame


10. getFixCounts()

 - Returns an array that represents the count for how many times a particular page in a frame is pinned. 

11. getNumReadIO()

 - Return the number of pages that have been read from disk since a buffer pool has been initialized.

12. getNumWriteIO()

 - Return the number of pages written to the page file since the buffer pool has been
initialized.

--------------------------------------------------------------------------------
Helper Functions
13. insertPageAtIndex()

 -  Insert a page handle into a page frame in the buffer pool. This function will overwrite what is currently in the pageFrame array at the given index. After insertion, this function sets the dirty bit to false, sets the pinCount to 1, updates the statistics arrays (fixCount, dirtyFlags, frameContents), and stores the data in the page handle in the page frame.

14. poolIsFull()

 - Return whether or not a buffer pool is full. It does this by checking the "frameCount" parameter of the buffer pool's page table. "frameCount" is the number of pages currently in the pool. "numPages" is the size of the pool. When these two parameters are equal, the pool is full

15. findOpenIndex()

 - Find an empty space in the bufferPools pageFrame array and return that location. If the buffer pool is full, -1 is returned. The hash value for searching the pageFrame array is calculated using the modulo function i.e. pageHandle page number  % size of pool. If the hash value returns an index in which there is an existing page, the function linearly probes the frameArray until an empty spot is found.

16. indexIsEmpty()

 - Return whether or not there are any pages in a particular page frame. If page == -1, then it is considered empty.

17. findPageIndex()

 - Search the buffer pools pageFrame array for a frame that contains the page pageNo. It returns the index of the frame in the array. If no page is found, -1 is returned. It completes this search using the modulo function pageNumber%poolSize. If the page is not found at that location, the function linearly probes the page frame array until it is found or no page is found.

18. getFramePageNumber()

 - Return the page number of the page stored in the frame at that index. If there is no page stored at the frame, then -1 is returned.

19. pinCountIsZero()

- Return whether or not there is any page pinned to a particular page frame. If the pin count of a page frame is 0, then it is considered empty.

20. isDirty()

- Returns whether or not a given page frame contains a page that is dirty. Returns either true or false

21. setDirtyBit()

- Given a particular page frame and a status (either true or false), this function sets the dirty bit of the page at the page frame to equal status

22. writeBack()

- Writes back pageHandle contents of a page at a given page frame back to disk and increases the writeIO count. It does not check if the page has been pinned or if it is dirty.

22. safelyWriteBack()

- Calls writeBack function after verifying the page is dirty and that it is not pinned. Sets pages dirty bit to false. If succesful the function returns true. If not it returns false. 

23. setLRUIndex()

- Given a queue of timestamps associated with page frame accesses this function updates the timestamp queue. It takes in the index of the page frame that was last accessed. It uses the maxTimeStamp variable of the BM_Queue struct and sets that page frame indexe's timestamp to be maxTimeStamp+1. It updates maxTimeStamp accordingly. This function is called in the pinPage() function when a page already exists in the pool and is accessed and when a new page is added to the pool.

24. getLRUIndex()

- Given a queue of timestamps associated with page frame accesses and the size of that queue, this function returns the page frame array index of the last recently used page. The function linearly searches the timestamp array for the index of page frame with a minimum timestamp value AND does not contain a page that is pinned. If all pages are pinned, the function returns -1 so that an error can be thrown. If an index is found, setLRUIndex() is called to now make that index the most recently accessed (So that the new page inserted is the most recently accesses).

25. fifoIndex()

- This function returns the index of a page frame containing the page to be evicted. This function utilizes the pointer variable in the BM_Queue struct. This pointer point to the index of the page frame whos page was inserted first. If this page is pinned, the function linearly continues on until a page frame index at which a non-pinned page exists is found. In order to keep the array in order from first in to last in, the remaining indices in the array are shifted left and the found index is placed at the end of the array (because it will now become the index of the page frame whose page was inserted last once the eviction occurs). If all pages are pinned, then -1 is returned so an error can be thrown. This function is called by the pinPage() function when a new page is added

26. poolInitialized()

- This function returns whether or not a buffer pool has been initialed and is used by other functions to verif the pool was succesfully initialized before performing their actions. Upon initializing a buffer pool, the mgmtData parameter is first set to NULL. If the pagefile passed to the buffer pool is able to succesfully be opened, then it is initialized to be a pageTable struct. If the file does not open succesfully, then the parameter stays NULL. Therefore, this function checks if mgmtData is NULL in order to determine if a buffer pool has not been initialized.

27. *initFrameContents ()

- This function allocates memory for and array of page numbers that are all initially set to -1 and returns a pointer to the array

28. *initDirtyFlags()

- This function allocates memory for an array of dirtyBits that are all initially "false". It returns a pointer to this array.

29. *initFixCounts()

- This function allocates memory for an array of pin counts that are all initially 0. It returns a pointer to this array.

--------------------------------------------------------------------------------
TEST CASES

This program verifies that all the test cases mentioned in the file "test_assign2_1.c" and "test_assign2_1.c"  execute correctly.

