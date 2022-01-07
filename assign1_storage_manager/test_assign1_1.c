#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testCreateOpenClose(void);
static void testSinglePageContent(void);

/* main function running all tests */
int main(void)
{
  testName = "";

  initStorageManager();

  testCreateOpenClose();
  testSinglePageContent();

  return 0;
}

/* check a return code. If it is not RC_OK then output a message, error description, and exit */
/* Try to create, open, and close a page file */
void testCreateOpenClose(void)
{
  SM_FileHandle fh;

  testName = "test create open and close methods";

  TEST_CHECK(createPageFile(TESTPF));

  TEST_CHECK(openPageFile(TESTPF, &fh));
  ASSERT_TRUE(strcmp(fh.fileName, TESTPF) == 0, "filename correct");
  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));

  // after destruction trying to open the file should cause an error
  ASSERT_TRUE((openPageFile(TESTPF, &fh) != RC_OK), "opening non-existing file should return an error.");

  TEST_DONE();
}

/* Try to create, open, and close a page file */
void testSinglePageContent(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test single page content";

  ph = (SM_PageHandle)malloc(PAGE_SIZE);

  // create a new page file
  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));
  printf("created and opened file\n");

  // read first page into handle
  TEST_CHECK(readFirstBlock(&fh, ph));
  // the page should be empty (zero bytes)
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
  printf("first block was empty\n");

  // change ph to be a string and write that one to disk
  for (i = 0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  TEST_CHECK(writeBlock(0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock(&fh, ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first block\n");

  // change ph to be a string and write that one to disk for second block
  for (i = 0; i < PAGE_SIZE; i++)
    ph[i] = (i % 11) + '0';
  TEST_CHECK(writeBlock(1, &fh, ph));
  printf("writing second block\n");

  // read back the page containing the string and check that it is correct for second block using readBlock
  TEST_CHECK(readBlock(1,&fh, ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 11) + '0'), "character in page read from disk is the one we expected");
  printf("reading second block using readCurrentBlock\n");
  //Current page position is pg 1


  // read back the page containing the string and check that it is correct for second block using readCurrentBlock
  TEST_CHECK(readCurrentBlock(&fh, ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 11) + '0'), "character in page read from disk is the one we expected");
  printf("reading second block using readCurrentBlock\n");
  //Current page position is pg1

  // read previous page into handle
  TEST_CHECK(readPreviousBlock(&fh, ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected");
  printf("reading first block using readPreviousBlock\n");
  //Current page position is pg 0

  // read Next page into handle
  TEST_CHECK(readNextBlock(&fh, ph));
  // the page should be empty (zero bytes)
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 11) + '0'), "character in page read from disk is the one we expected");
  printf("reading second block using readNextBlock\n");
  //Current page position is pg 1

  TEST_CHECK(appendEmptyBlock(&fh));
  printf("appended Empty Block\n");
  //Current page position is pg 1

  // change ph to be a string and write that one to disk for third block
  TEST_CHECK(readNextBlock(&fh,ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ph[i] = (i % 12) + '0'; 
  TEST_CHECK(writeCurrentBlock(&fh, ph));
  printf("writing third block using writeCurrentBlock\n");
  //Current page position is pg 2

  // read First page into handle
  TEST_CHECK(readFirstBlock(&fh, ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected");
  printf("reading first block using readFirstBlock\n");
  //Current page position pg 0

  // read Last page into handle
  TEST_CHECK(readLastBlock(&fh, ph));
  for (i = 0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 12) + '0'), "character in page read from disk is the one we expected");
  printf("reading last block using readLastBlock\n");
  //Current page position is pg 2

  // If the file has >= numberOfPages then do nothing with ensure capacity
  TEST_CHECK(ensureCapacity(3, &fh));
  ASSERT_TRUE((fh.totalNumPages == 3), "expect 3 pages after ensure capacity");
  printf("ensure the Capacity is = 3\n");


  // If the file has less than numberOfPages then increase the size of totalNumPages to numberOfPages by ensuring capacity to 5
  TEST_CHECK(ensureCapacity(5, &fh));
  ASSERT_TRUE((fh.totalNumPages == 5), "expect 5 pages after ensure capacity");
  printf("ensure the Capacity is >= 5\n");

  // If the file has >= numberOfPages then do nothing with ensure capacity
  TEST_CHECK(ensureCapacity(4, &fh));
  ASSERT_TRUE((fh.totalNumPages == 5), "expect 5 pages after ensure capacity again");
  printf("ensure the Capacity is = 5\n");

  //Close file, reopen and ensure consistent # of pages
  printf("Closing file for test...\n");
  TEST_CHECK(closePageFile(&fh));
  printf("Re-opening file for test...\n");
  TEST_CHECK(openPageFile(TESTPF,&fh));
  ASSERT_TRUE((fh.totalNumPages == 5), "expect 5 pages after reopening file");
 
  //Close page file
  TEST_CHECK(closePageFile(&fh));
  
  // destroy new page file
  TEST_CHECK(destroyPageFile(TESTPF));

  //free memory allocated for pageHandle
  free(ph);
    
  TEST_DONE();
}
