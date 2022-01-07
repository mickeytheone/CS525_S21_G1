STORAGE MANAGER - Version 1.0 - 2/18/2021

To run our storage manager, please make use of makefile. Type "make" to compile the code and generate test case.

TEAM MEMBERS:

1. Rene Lamb
2. Li-Han Feng
3. Milana Saradzic

OVERVIEW: Our storage manager implements functions that allow a user to read/write data from a page file (disk) and store in memory allocated by the user. Our program allows a user to initialize a new page file. A page file must be opened before further operations can be performed on it. Our program utilizes a fileHandle struct to manage the 
meta-data about an open file. This meta-data is initialized when the page file is opened. Upon opening, a fileHandle is initialized with the filename of the page file associated with the fileHandle, the total number of the pages in the file sets the current page position to 0, and sets the fileHandles mgmtInfo variable to be a pointer to the page file that is associated with the fileHandle. This allows other functions to operate on a particular open page file just by passing in the fileHandle to that page file. PageHandles are pointers to memory arrays of characters. Using a pageHandle and fileHandle, a user can read/write and modify page files (See functions below). On succesful operation of a particular function, an RC constant value RC_OK is returned. If there is an issue, the appropriate RC constant error code is returned. 

Bellow is a summary of the functions implemented

1. initStorageManager()

 - This function is used to initialize the storage manager.

2. createPageFile()

 - This function is used to create a  new page file. If the filename exists, it erases that file and creates a new blank file. The new file is populated with a single page of '\0' bytes.

3. openPageFile()

 - This function is used to open a previously created page file and initialize the fileHandle passed in for that page file. fHandle->mgmtInfo is set to point to the opened file. 

4. closePageFile()

 - This function is used to close the file and set the file pointer to NULL.

5. destroyPageFile()

 - This is used to remove the file from disk.

6. readBlock()

 - This function is used to read a block of the file using the parameter pageNum passed in the function call. Given a page number, and a fileHandle, this function reads the data in the given page   from the file associated with the fileHandle and stores in in memory pointed to by memPage of size PAGE_SIZE. Page number index is from 0 to totNumPages-1. If an attempt is made to access a page number that is out of bound, an error will be issued.

 - The file is read using fread() functions. Current pagePos is set using fseek.

7. getBlockPos()

 - This function is used to return the position of the current page in the file.

8. readFirstBlock()

 - This function is used to retrieve first page from the file.

9. readLastBlock()

 - This function is used to retrieve last page from the file.

10. readPreviousBlock()

 - This function is used to fetch a block from the file that is one block before the current page position.

11. readCurrentBlock()

 - This function is used to fetch a block from the file that is pointed by the current page position.

12. readNextBlock()

 - This function is used to fetch a block from the file that is one block after the current page position.

13. writeBlock()

 - This function is used to write new content into the file. Given a page number, and a fileHandle, data from memory pointed to by memPage is written to the file associated with the fileHandle
   at the given pageNum. Page number index is from 0 to pageNum. If an attempt is made to write to a page number on file that is <0 then error is returned. If an attempt is made to write to a 
   page number >= the total number of pages on the file, this function will generate additional empty pages to the file so that it can write to the appropriate page number index.

 - We write the contents into the file using fseek() and fwrite() functions. Space on disk is ensured using ensureCapacity function.

14. writeCurrentBlock()

 - This function is used to write content on the current page.

15. appendEmptyBlock()

 - This function is used to add one block at the end of the file with a size of one block.

 - The new page is filled with '\0' bytes.

16. ensureCapacity()

 - This function is used to check if the file has the number of pages we require. It appends empty block to the end of a file until number of pages of file associated with fHandle = numberOfPages.

 - This function uses appendEmptyBlock to add pages

17. getPages()

 - This is a helper function used by openPageFile. It calculates the size of a file in bytes (res) and returns the pages in the file (res/PAGE_SIZE)

18. writeEmptyBlock()

 - This is a helper function used by createPageFile and appendEmptyBlock functions. It creates and array of PAGE_SIZE '\0' chars and stores them into the file spot pointed to by a file pointer.
 
TEST CASES

This program verifies that all the test cases mentioned in the file "test_assign1_1.c" execute correctly.
