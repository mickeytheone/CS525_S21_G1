BUFFER MANAGER - 3/12/2021

To run our buffer manager, please make use of makefile. Type "make" to compile the code and generate test case.

--------------------------------------------------------------------------------
TEAM MEMBERS:

1. Rene Lamb
2. Li-Han Feng

--------------------------------------------------------------------------------
OVERVIEW:

Implemented a record manager that allows navigation through records, and inserting, updating and deleting records.


--------------------------------------------------------------------------------
Structure
The overall structure of consists of a Table Data struct, Page directory struct and Record Page struct. The table data struct stores the name of the table to be created along with that tables schema.
When creating a table, a schema for that table is stored in page 0 of a new page file. The number of pages that contain records (record pages) is also initialized to 0 and written to the page 0 of the page file as well. 

When a table is opened, the record manager reades the schema info and number of record pages from page 0 of the table page file and uses that to initialize the table data,
page directory and record page structs. A page directory contains metadata regarding the overall records and contains an array of pointers to record page structs. The record manager uses the record page to determine what record pages have free space (using a free space array), the total number of records (or tuples), the total number of record pages, and the buffer pool used to manage the 
interact witht he record pages. When opening a table, the page directory gets assigned to the table data struct. 

A record page contains records for a particular table. The record pages start at page 1 of the table page file. When a new record is inserted, the record manager first checks the page directory to determine the first page at which there is an open space for the record. If there is none, a new record page is created and the record is added there. At the beginning of each record page a pre defined amount of bytes are allocated in order to store the number of tuples in that particular record page. This value is updated whenever a record is inserted or deleted into a record page and written back. This value is used when opening a table to help initialize the page directory.A record page struct also contains metadata in regards to the number of tuples in a record page, the size of a record (since the records are of fixed size), the record pages number and the amount of free space in that record page. This meta data is used to initialize page directory and to allow the scanning functions to determine when they have reached the end of each record page.
--------------------------------------------------------------------------------
Bellow is a summary of the functions implemented:

Table and Manager Functions
1. initRecordManager()

 - This function prints that the record manager is initialized.

2. shutdownRecordManager()

 - This function prints that the record manager has been shutdown.

3. createTable()

 - This function takes in the name of the table to create and a schema. It creates a new page file using the name. It writes the schema data and number of records (initially zero) into page 0 of    the new page file and return RC_OK if the creation was succesful.

4. openTable()

 - This function takes in a pointer to a table data struct and a table name. It created a buffer pool and opens the table page file by initializing the buffer pool. It then reads from page 0 of the 
   page file and reads in the number of record pages and schema data to be used to initialize table data, page directory, and record page structs.

5. closeTable()

 - This function closes takes in a table represented by a table data struct and "closes" it by shutting down the buffer manager associated with a table and frees memory allocated to the table data, page directory, and record page structs.

6. deleteTable()

 - This function takes in the name of a table and destroys the page file associated with that table.

7. getNumTuples()

 - This function returns the number of records that are stored in a table. It does this by returning the "totalTuples" variable in the page directory of an open table. The page directory struct
   directly stores the number of records in a page file. The insert and delete functions update this variable whenever they are called on. 

--------------------------------------------------------------------------------
Record Functions
8. insertRecord()

 - This function inserts a new record in the table. It takes in a table data struct and the pointer to a record. This function first finds the first record page that is not full by linearly scanning 
   through a page directorys "pageFullArray". If a page is found, the function then uses the size of the record (given by getRecordSize) to scan through a record page to find an open "slot" for the
   record. Because the record sizes are fixed, this function simply jumps from one record to another until it encounters an unocupied location for the record. When an unoccupied location is found, 
   the function first reserves the first byte of that location and marks it using the character 'O' to represent "Occupied". This character is used by future insertion, deletions and scans to 
   determine if a particular spot is occupied or not. This works because the orignial page of each record page is populated with all '\0' bytes. Afther the '0' character, the record data is stored 
   in the record page. The record id information is then updated and the rid page is set to be the page the record was inserted and the rid slot is set to be the slot the record was inserted. If no
   free page is found, the function reallocates memory for the "pageFullArray" and "recPageArray" to accomodate the addition of a new record page to the end of the file. The function then inserts 
   the record as stated above. 

9. deleteRecord()

 - This function deletes a record with specific Record ID from the table. The function first locates the record using rid page and slot number. Once that location is found, the function looks at 
   the first character of that function. If it is 'O' then the function overwrites that characted and the record after that character with null bytes (clearing it) and updates the number of records 
   written to the beginning of the page. If the character is not 'O' then that means that record does not exist and RC_RM_NO_RECORD_EXISTS is returned. 

10. updateRecord()

 - This function update an existing record in the table. The function first locates the record using rid page and slot number. Once that location is found, the function looks at 
   the first character of that function. If it is 'O' then the function overwrites that character and the record after that character with null bytes (clearing it) and replaces it with the new 
   record data. If the character is not 'O' then that means that record does not exist and RC_RM_NO_RECORD_EXISTS is returned. 

11. getRecord()

 - This Function retrieve a record with specific Record ID. The function first locates the record using rid page and slot number. Once that location is found, the function looks at 
   the first character of that function. If it is 'O' then the function copies the record data at that location to the record structs data. If the character is not 'O' then that means that record
   does not exist and RC_RM_NO_RECORD_EXISTS is returned.


--------------------------------------------------------------------------------
Scan Functions
12. startScan()

- This function starts a scan by getting data from the RM_ScanHandle data structure which is passed as an argument to it. It initializes a "scan helper" struct. This struct contains helpful data
  that will help with scanning through the data. This struct contains the current page and slot (or tuple) the scan is on, the expression passed in to start scan, and the total number of record 
  pages.

13. next()

 - This function returns the next record that matches the scan condition. It uses the scan helper struct of the scan handler to navigate through the record pages. The function linearly scans through 
   each record page and updates the scan helpers current page and tuple number accordingly. At each record, the function attempts to get the record information stored at a particular page and slot. 
   If there is a record exisitng at that slot (Indicated by a 'O' character) then the function check to see if it should be returned based on whether or not it matches the scan condition. If the scan
   condition is NULL, the function returns that record regardless. If the scan reaches the end of the number of record pages with no next record found, it returns RC_RM_NO_MORE_TUPLES;

14. closeScan()

 - This function closes the scan operation and cleans up all the resources utilized by de-allocating the memory used by the scan helper.


--------------------------------------------------------------------------------
Schema Functions
15. getRecordSize()

 -  This function returns the size of a record in the specified schema.

16. createSchema()

 - This function create a new schema and assigns the memory for it with the specified parameters in memory.

17. freeSchema()

 -  This function removes the schema from the memory.


--------------------------------------------------------------------------------
Attribute Functions
18. createRecord()

 -  This function creates a new record by allocating memory for it and initializng the id page and slot to be -1. It properly sizes the allocation of data by calling getRecordSize to get the size
    of the record.

19. freeRecord()

 -  This function de-allocates the memory space allocated to the record.

20. getAttr()

 - This function retrieves an attribute from the given record in the specified schema. It does this by calculating the offset at which a particular attribute starts.

21. setAttr()

 - This function sets the attribute value in the record in the specified schema. It does this by calculating the offset at which a particular attribute starts.


--------------------------------------------------------------------------------

TEST CASES

This program verifies that all the test cases mentioned in the file "test_assign3_1.c" execute correctly.
