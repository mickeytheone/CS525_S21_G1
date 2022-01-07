#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage_mgr.h"

//Helper function definitions
int getPages(FILE * fp);
RC writeEmptyBlock(FILE * fp, int pgNo);

/* manipulating page files */

/* @name initStorageManager(void)
   @desc displays storage manager initialization method
   @param void
   @return n/a */
void initStorageManager(void){
  printf("Initialize Database Storage Manager -- Version 1.0 -- Authors: R.Lamb, M. Saradzic, L.Feng\n");
}

/* @name createPageFile(char * fileName)
   @desc creates new file titles 'filename'. If filename exists, erases file and creates new blank file.
new files is populated with a single page of '\0' bytes. One (1) page is PAGE_SIZE bytes
   @param char * filename - string name of new file to create
   @return RC - Write failure or OK*/
RC createPageFile(char * fileName){
  //Create and open new file
  FILE *fp; 
  fp = fopen(fileName,"w");
  //Determine if there was an issue opening new file
  if(fp==NULL)
  {
	  printf("Error opening file");
	  return RC_WRITE_FAILED;
  }
  
  int res1 =  writeEmptyBlock(fp,0); //populate open file with and array of \0 bytes of size PAGE_SIZE
  if(fclose(fp)!=0 || res1!=RC_OK){
    return RC_WRITE_FAILED;
  }
  return RC_OK; 
}

/* @name openPageFile(char * fileName, SM_FileHandle * fHandle)
   @desc opens previously created page file and initializes the fileHandle passed in for that page file.
If the file does not exist, an RC error is issued.
   @param char * filename - string name of existing file, SM_FileHandle * fHandle - pointer to fileHandle struct
   @return RC_OK on success, RC_FILE_NOT_FOUND on failure*/
RC openPageFile(char * fileName, SM_FileHandle * fHandle){
  //open file 
  FILE *fp;
  fp = fopen(fileName,"r+");
  //If file does not exist, return RC_FILE_NOT_FOUND
  if(fp==NULL)
  {
	  return RC_FILE_NOT_FOUND;
  }

  //Determine # of pages in file
  int pages = getPages(fp);

  //Initialize file handle
  fHandle->fileName = fileName;
  fHandle->totalNumPages = pages;
  fHandle->curPagePos = 0;
  fHandle->mgmtInfo = fp; //mgmtInfo associates the newly opened file with the newly initialized file handle
 
  return RC_OK;
}

/* @name closePageFile(SM_FileHandle * fHandle)
   @desc closes a file associated with the passed in fileHandle and sets the handle to NULL. if file was previously closed, error will occur and
RC_FILE_HANDLE_NOT_INIT will be issued. fHandle MUST be initialized by openPageFile first or set to NULL else function call will
have undefined behavior.
   @param SM_FileHandle * fHandle - pointer to previously initialized (or NULL) fileHandle struct
   @return RC_OK on success, RC_FILE_HANDLE_NOT_INIT or RC_FILE_NOT_FOUND on failure*/
RC closePageFile(SM_FileHandle * fHandle){
  //Determine if file has already been closed
  if(fHandle->mgmtInfo==NULL){
    return RC_FILE_HANDLE_NOT_INIT;
  }
  int c = fclose(fHandle->mgmtInfo);
  //Set mgmtInfo file pointer to null so that once closed, the fileHandle cannot be operated on
  fHandle->mgmtInfo = NULL;
  if(c==0)
  {
  	return RC_OK;
  }
  printf("ERROR: File could not be closed\n");
  return RC_FILE_NOT_FOUND;
}

/*@name destoryPageFile(char * fileName)
  @desc removes file from directory given string name to that file. Error if file does not exist.
  @para char * fileName - string name of file to remove
  @return RC_OK on success, RC_FILE_NOT_FOUND on failure */
RC destroyPageFile(char * fileName){
  //attempt to remove file from directory
    int r = remove(fileName);
    if(r==0){
  	return RC_OK;
      }
  printf("File could not be destroyed\n");
  return RC_FILE_NOT_FOUND;
}

/* reading blocks from disc */

/*@name readBlock(int pageNum, SM_FileHandle * fHandle, SM_PageHandle memPage)
  @desc given a page number, and a fileHandle, read the data in the given page from the file associated with the fileHandle and store 
it in memory pointed to by memPage of size PAGE_SIZE. Page number index is from 0 to totNumPages-1. If an attempt is made to access 
a page number that is out of bounds, error will be issued.
  @param int pageNum - index of page in file to store in memory, SM_FileHandle * fHandle - fileHandle to a page file, 
SM_PageHandle memPage - location in memory to store page read from file
  @return RC_OK on success, RC_FILE_HANDLE_NOT_INIT or RC_READ_NON_EXISTING_PAGE on failure */  
RC readBlock(int pageNum, SM_FileHandle * fHandle, SM_PageHandle memPage){
  //Verify fHandle is initialized
  if(fHandle->mgmtInfo==NULL){
    return RC_FILE_HANDLE_NOT_INIT;
  }
  //Verify that block to be read exists
  if(pageNum>fHandle->totalNumPages || pageNum<0)
  {
	  return RC_READ_NON_EXISTING_PAGE;
  }
 
  //Move file pointer to page pageNum and update currentPagePos in fHandle
  fseek(fHandle->mgmtInfo,pageNum*PAGE_SIZE,SEEK_SET);
  fHandle->curPagePos = pageNum;

  //store block pageNum from file in memory pointed to by memPage
  fread(memPage,1,PAGE_SIZE,fHandle->mgmtInfo); 
  return RC_OK;
}

/*@name getBlockPos(SM_FileHandle * fHandle)
  @desc get the block position of the most recently read block indicated by a files fHandle
  @param SM_FileHandle * fHandle - fHandle associated with open file
  @return int of current page position */
int getBlockPos(SM_FileHandle * fHandle){
  return fHandle->curPagePos;
}

//See readBlock above
RC readFirstBlock(SM_FileHandle * fHandle, SM_PageHandle memPage){
  return readBlock(0,fHandle,memPage);
}

//See readBlock above
RC readPreviousBlock(SM_FileHandle * fHandle, SM_PageHandle memPage){
  return readBlock(getBlockPos(fHandle)-1,fHandle,memPage);
}

//See readBlock above
RC readCurrentBlock(SM_FileHandle * fHandle, SM_PageHandle memPage){
  return readBlock(getBlockPos(fHandle),fHandle,memPage);
}

//See readBlock above
RC readNextBlock(SM_FileHandle * fHandle, SM_PageHandle memPage){
  return readBlock(getBlockPos(fHandle)+1,fHandle,memPage);
}

//See readBlock above
RC readLastBlock(SM_FileHandle * fHandle, SM_PageHandle memPage){
  return readBlock((fHandle->totalNumPages)-1,fHandle,memPage);
}

/* writing blocks to a page file */

/*@name writeBlock(int pageNum, SM_FileHandle * fHandle, SM_PageHandle memPage)
  @desc given a page number, and a fileHandle, write the data frin memory pointed to by memPage to the file associated with the fileHandle 
at the given pageNum. Page number index is from 0 to pageNum. If an attempt is made to write to a page number on file that is < 0 then error
returned. If an attempt is made to write to a page number >= the total number of pages on the file, this function will generate additional 
empty pages to the file so that it can write to the appropriate page number index.
  @param int pageNum - index of page in file to store in file (on disk), SM_FileHandle * fHandle - fileHandle to a page file, 
SM_PageHandle memPage - page location in memory containing data to write to file
  @return RC_OK on success, RC_FILE_HANDLE_NOT_INIT or RC_WRITE_FAILED on failure */  

RC writeBlock(int pageNum, SM_FileHandle * fHandle, SM_PageHandle memPage){
  if(fHandle->mgmtInfo==NULL)
    {
      return RC_FILE_HANDLE_NOT_INIT;
    }
  //Ensure that there is enough write capacity
  RC ret = ensureCapacity(pageNum+1,fHandle); 
  //set file pointer to where new block to be written
   int res1 =  fseek(fHandle->mgmtInfo,pageNum*PAGE_SIZE,SEEK_SET);
   //write block to file
   int res2 = fwrite(memPage,1,PAGE_SIZE,fHandle->mgmtInfo);
  if(res1!=0 || res2!=PAGE_SIZE || ret != RC_OK){
    return RC_WRITE_FAILED;
  }
  return RC_OK; 
}

//See writeBlock aove
RC writeCurrentBlock(SM_FileHandle * fHandle, SM_PageHandle memPage){
  return writeBlock(fHandle->curPagePos,fHandle,memPage);
}

/*@name appendEmptyBlock(SM_FileHandle * fHandle)
  @desc appends an empty block of '\0' characters to the end of a file associated with fHandle and increases the files total number 
of pages by 1. NOTE: this function moves the file pointer associated with the open file to point to the beginning of the last block
of the file
  @param SM_FileHandle * fHandle - fileHandle to a page file
  @return RC_OK on success, RC_WRITE_FAILED or RC_FILE_HANDLE_NOT_INIT on failure */
RC appendEmptyBlock(SM_FileHandle * fHandle){
  if(fHandle->mgmtInfo==NULL){
    return RC_FILE_HANDLE_NOT_INIT;
  }
  int res1 = fseek(fHandle->mgmtInfo,PAGE_SIZE,SEEK_END);//set file pointer to end of file
  int res2 = writeEmptyBlock(fHandle->mgmtInfo, fHandle->totalNumPages);//write empty bloc of size PAGE_SIZE to file pointed to by fHandle
  if(res1!=0 || res2!=RC_OK)
    {
      return RC_WRITE_FAILED;
    }
  fHandle->totalNumPages = fHandle->totalNumPages + 1; //increase page size
  return RC_OK;
}

/*@name ensureCapacity(int numberOfPages, SM_FileHandle * fHandle)
  @desc appends empty block to the end of a file until number of pages of file associated with 
fHandle = numberOfPages
  @param int numberOfPages - file pages to be increase to be set to numberOfPages if and only if numberOfPages > total number of pages in file
SM_FileHandle * fHandle - fileHandle to a page file
  @return RC_OK on success, RC_FILE_HANDLE_NOT_INIT or RC_WRITE_FAILED on failure */
RC ensureCapacity(int numberOfPages, SM_FileHandle * fHandle){
  if(fHandle->mgmtInfo==NULL){
    return RC_FILE_HANDLE_NOT_INIT;
  }
  //If numberOfPages > number than pages in the file, calculate the difference and append new block
  if((fHandle->totalNumPages)<numberOfPages){
    int newPages = numberOfPages-(fHandle->totalNumPages);
	  for(int x = 0; x<newPages; x++){
	    if(appendEmptyBlock(fHandle)!=RC_OK){
	      return RC_WRITE_FAILED;
	    }
	  }
  }
  return RC_OK;
}

/*Helper function - getPages takes a pointer to a file as a parameter, 
calculates the size of the file in bytes (res) and returns the pages in the file (res/PAGE_SIZE) */
int getPages(FILE * fp){
  //create temporary variable for file pointer
  FILE * temp;
  temp = fp;
  //Determine the size of the file    	
  fseek(fp,0,SEEK_END); //Put pointer at the end of the file
  long int res = ftell(fp); //return the # of bytes in the file
  //return file pointer to its previous location
  fp = temp;
  return res/PAGE_SIZE; //calculate the # of pages
}

/*Helper function - writeEmptyBlock creates an array of PAGE_SIZE '\0' chars 
and stores them into the file spot pointed to by fp */
RC writeEmptyBlock(FILE * fp, int pgNo){
 //Create and array of size PAGE_SIZE filled with '\0' bytes
  char * barray = malloc(PAGE_SIZE);
  memset(barray,'\0',PAGE_SIZE);
  //Set file pointer to position indicated by pgNo
  int res1 = fseek(fp,pgNo*PAGE_SIZE,SEEK_SET);
  //Write array to open file
  int res2 = fwrite(barray,1,PAGE_SIZE,fp);

  if(res1!=0 || res2!=PAGE_SIZE){
    return RC_WRITE_FAILED;
  }

  //free array
  free(barray);

  return RC_OK;
}
