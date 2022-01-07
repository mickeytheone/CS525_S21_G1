#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"
#include "btree_mgr.h"

typedef struct BTreeNode {
  bool isLeaf;
  Value * keys;
  int * children;
  int next;
  int prev;
  int parent;
  //BM_PageHandle * data;
  int pageNum;
  RID * rids;
  int numEntries;
  int n;
} BTreeNode;

typedef struct BTreeHelper {
  int n;
  int numNodes;
  int root;
  BM_BufferPool * bp;
  BTreeNode ** nodeArray;  //Array of b+tree nodes
} BTreeHelper;

//Helper Functions
BTreeNode * createNode(int n, bool isLeaf,BM_PageHandle * ph, BTreeHandle * handle);
void destroyNode(BTreeNode * node);
Value * getKeys(BTreeNode * node, int key);
int getChild(BTreeNode * node, int child);
int getRightSibling(BTreeNode * node);
int getLeftSibling(BTreeNode * node);
int getParent(BTreeNode * node);

BTreeNode * getChildNode(BTreeNode * node, int child, BTreeHelper * helper);
BTreeNode * getRightSiblingNode(BTreeNode * node, BTreeHelper * helper);
BTreeNode * getLeftSiblingNode(BTreeNode * node, BTreeHelper * helper);
BTreeNode * getParentNode(BTreeNode * node, BTreeHelper * helper);

BM_PageHandle * getData(BTreeNode * node);
RID * getRIDS(BTreeNode * node, int rid);
bool isEmpty(BTreeNode * node, BTreeHelper * helper);
void setNotEmpty(BTreeNode * node, BTreeHelper * helper);
void setIsLeaf(BTreeNode * node, bool isLeaf, BTreeHelper * helper);
bool getIsLeaf(BTreeNode * node);
void setParent(BTreeNode * node, BTreeNode * parent, BTreeHelper * helper);
void setLeftSibling(BTreeNode * node, BTreeNode * leftSibling, BTreeHelper * helper);
void setRightSibling(BTreeNode * node, BTreeNode * rightSibling, BTreeHelper * helper);
void setKey(BTreeNode * node, int keyLoc, Value key, BTreeHelper * helper);
void setChild(BTreeNode * node, int childNum, BTreeNode * child, int nValue, BTreeHelper * helper);
void setRID(BTreeNode * node, int ridNum, RID rid, int nValue, BTreeHelper * helper);
void setNumNodes(BTreeHandle * handle, int numNodes);
BTreeNode * insertIntoLeaf(BTreeNode * node, Value * key, RID rid, int n, BTreeNode * parent, BTreeHandle * handle);
BTreeNode * splitLeaf(BTreeNode * node, Value * key, RID rid, int n, BTreeNode * parent, BTreeHandle * handle);
RC shiftRightAndInsert(BTreeNode * node, int index, Value * key, RID * rid, BTreeNode * child, int n, BTreeHelper * helper);
int findOpenPage(BTreeHelper * helper);
BTreeNode * insertIntoNonLeaf(BTreeNode * node, BTreeNode * newChild, Value * key, int n, BTreeHandle * handle);
int findNode(BTreeHelper * helper, int pageNum);
int getPageNum(BTreeNode * node);
BTreeNode * splitNonLeaf(BTreeNode * node, BTreeNode * newChild, Value * key, int n, BTreeHandle * handle);
int findRootPage(BTreeHelper * helper, BTreeHandle *tree);

//Initialize BTreeNode 
BTreeNode * createNode(int n, bool isLeaf,BM_PageHandle * ph,BTreeHandle * handle){
  BTreeHelper  * helper = handle->mgmtData;
  BTreeNode * newNode = malloc(sizeof(BTreeNode));
  newNode->n = n;
  newNode->isLeaf = isLeaf;
  //Value keys[n];
  Value * keys = malloc(sizeof(Value)*n);
  for(int x = 0; x<n; x++){
    if(handle->keyType == DT_INT){
      keys[x].dt = DT_INT;
      keys[x].v.intV = INT_MIN; //IS THIS CORRECT?
    }
  }
  newNode->keys = keys;
  //newNode->keys = malloc(n*sizeof(Value)); //Should keys be initialized to null?
  newNode->children = malloc((n+1)*sizeof(int));
  newNode->parent = 0; //Should i make these -1?
  newNode->next = 0;
  newNode->prev = 0;
  newNode->numEntries = 0;
  newNode->pageNum = ph->pageNum;
  //RID rids[n]; //Should these be initialized
  RID * rids = malloc(sizeof(RID)*n);
  newNode->rids = rids;
  int result;
  getNumNodes(handle,&result);
  helper->nodeArray[result] = newNode;
  //Re-Allocate here
  setNumNodes(handle,result+1);
  //newNode->rids = malloc(n*sizeof(RID));
  return newNode;
}

//Initialize BTreeNode 
BTreeNode * readNode(int n, BM_PageHandle * ph,BTreeHandle * handle){
  SM_PageHandle data = ph->data;
  BTreeNode * newNode = malloc(sizeof(BTreeNode));
  newNode->n = n;
  newNode->isLeaf = false;
  int offset = sizeof(char);
  if(*(int *)(data+offset)==1){
    newNode->isLeaf = true;
  }
  offset+=sizeof(int);
  newNode->parent = *(int *)(data+offset);
  offset+=sizeof(int);
  newNode->prev = *(int *)(data+offset);
  offset+=sizeof(int);
  newNode->next = *(int *)(data+offset);
  offset+=sizeof(int);
  //Value keys[n];
  Value * keys = malloc(sizeof(Value)*n);
  newNode->numEntries = 0;
  for(int x = 0; x<n; x++){
    keys[x] = *(Value *)(data+offset);
    offset+=sizeof(Value);
    if(keys[x].v.intV != INT_MIN){
      newNode->numEntries++;
    }
  }
  newNode->keys = keys;
  newNode->children = malloc((n+1)*sizeof(int));
  for(int x = 0; x<n+1; x++){
    newNode->children[x] = *(int *)(data+offset);
    offset+=sizeof(int);
  }
  //RID rids[n]; 
  RID * rids = malloc(sizeof(RID)*n);
  for(int x = 0; x<n; x++){
    rids[x] = *(RID *)(data+offset);
    offset+=sizeof(RID);
  }
  newNode->rids = rids;
  newNode->pageNum = ph->pageNum;
  return newNode;
}

//Free node
void destroyNode(BTreeNode * node){
  free(node->keys); //Do i need to free values as well?
  free(node->children);
  free(node->rids);
  //free(node->data);
  free(node);
}

//Getters and setters
Value * getKeys(BTreeNode * node,int key){
  return &(node->keys[key]);
}

//return page # of a child to the node
int getChild(BTreeNode * node, int child){
  if(child>node->n || child<0){
    printf("ERROR: Child index does not exist!\n");
    return -1;
  }
  return node->children[child];
}

//return page # of parent of node
int getParent(BTreeNode * node){
  return node->parent;
}

//return page # of right sibling of node
int getRightSibling(BTreeNode * node){
  if(!node->isLeaf){
    printf("WARNING: Not a leaf node!\n");
  }
  return node->next;
}

int getLeftSibling(BTreeNode * node){
  if(!node->isLeaf){
    printf("WARNING: Not a leaf node!\n");
  }
  return node->prev;
}

BTreeNode * getChildNode(BTreeNode * node, int child,BTreeHelper * helper){
  int pageNum = getChild(node,child);
  int res = findNode(helper,pageNum);
  if(res==-1){
    return NULL;
  }
  return helper->nodeArray[res];
}

BTreeNode * getRightSiblingNode(BTreeNode * node, BTreeHelper * helper){
  int pageNum = getRightSibling(node);
  int res = findNode(helper,pageNum);
  if(res==-1){
    return NULL;
  }
  return helper->nodeArray[res];
}

BTreeNode * getLeftSiblingNode(BTreeNode * node, BTreeHelper * helper){
  int pageNum = getLeftSibling(node);
  int res = findNode(helper,pageNum);
  if(res==-1){
    return NULL;
  }
  return helper->nodeArray[res];
}

BTreeNode * getParentNode(BTreeNode * node, BTreeHelper * helper){
  int pageNum = getParent(node);
  int res = findNode(helper,pageNum);
  if(res==-1){
    return NULL;
  }
  return helper->nodeArray[res];
}

int getPageNum(BTreeNode * node){
  return node->pageNum;
}

int findNode(BTreeHelper * helper, int pageNum){
  for(int x = 0; x<helper->numNodes;x++){
    if(getPageNum(helper->nodeArray[x])==pageNum){
      return x;
    }
  }
  return -1;
}
    

/*BM_PageHandle * getData(BTreeNode * node){
  return node->data;
} */



/*RID * getRIDS(BTreeNode * node){
  return node->rids;
} */

RID * getRIDS(BTreeNode * node, int rid){
  return &(node->rids[rid]);
}

bool isEmpty(BTreeNode * node, BTreeHelper * helper){
  //BM_PageHandle * ph = node->data;
  BM_PageHandle * ph  = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,ph,node->pageNum);
 // markDirty(bm,h);
  if(*(char *)(ph->data)!='O'){
    unpinPage(helper->bp,ph);
    free(ph);
    return true;
  }
  unpinPage(helper->bp,ph);
  free(ph);
  return false;
}

void setNotEmpty(BTreeNode * node, BTreeHelper * helper){
  BM_PageHandle * ph  = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,ph,node->pageNum);
  //BM_PageHandle * ph = node->data;
  *(char *)(ph->data) = 'O'; //Set first char byte to be occupied;
  markDirty(helper->bp,ph);
  unpinPage(helper->bp,ph);
  free(ph);
}

void setIsLeaf(BTreeNode * node, bool isLeaf, BTreeHelper * helper){
  int x = 0;
  if(isLeaf){
    x = 1;
  }
  BM_PageHandle * ph  = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,ph,node->pageNum);
  //BM_PageHandle * ph = node->data;
  *(int *)(ph->data+sizeof(char))=x;
  node->isLeaf = isLeaf;
  markDirty(helper->bp,ph);
  unpinPage(helper->bp,ph);
  free(ph);
}

bool getIsLeaf(BTreeNode * node){
  return node->isLeaf;
}

void setParent(BTreeNode * node, BTreeNode * parent, BTreeHelper * helper){
  BM_PageHandle * phChild  = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,phChild,node->pageNum);
 // BM_PageHandle * phChild = node->data;
  if(parent==NULL){
    node->parent = 0;
    *(int *)(phChild->data+sizeof(char)+sizeof(int))=0;
  }
  else{
    //BM_PageHandle * phParent = parent->data;
    *(int *)(phChild->data+sizeof(char)+sizeof(int))=parent->pageNum;
    node->parent = parent->pageNum;
  }
  markDirty(helper->bp,phChild);
  unpinPage(helper->bp,phChild);
  free(phChild);
}

void setLeftSibling(BTreeNode * node, BTreeNode * leftSibling, BTreeHelper * helper){
  if(node==NULL){
    return;
  }
  BM_PageHandle * phNode = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,phNode,node->pageNum);
  //BM_PageHandle * phNode = node->data;
  if(leftSibling==NULL){
    *(int *)(phNode->data+sizeof(char)+sizeof(int)+sizeof(int))=0;
    node->prev = 0;
  }
  else{
    //BM_PageHandle * phSibling = leftSibling->data;
    *(int *)(phNode->data+sizeof(char)+sizeof(int)+sizeof(int))=leftSibling->pageNum;
    node->prev = leftSibling->pageNum;
  }
  markDirty(helper->bp,phNode);
  unpinPage(helper->bp,phNode);
  free(phNode);
}

void setRightSibling(BTreeNode * node, BTreeNode * rightSibling, BTreeHelper * helper){
  if(node==NULL){
    return;
  }
  //BM_PageHandle * phNode = node->data;
  BM_PageHandle * phNode = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,phNode,node->pageNum);
  if(rightSibling==NULL){
    *(int *)(phNode->data+sizeof(char)+sizeof(int)+sizeof(int)+sizeof(int))=0;
    node->next = 0;
  }
  else{
    //BM_PageHandle * phSibling = rightSibling->data;
    *(int *)(phNode->data+sizeof(char)+sizeof(int)+sizeof(int)+sizeof(int))=rightSibling->pageNum;
    node->next = rightSibling->pageNum;
  }
  markDirty(helper->bp,phNode);
  unpinPage(helper->bp,phNode);
  free(phNode);
}

void setKey(BTreeNode * node, int keyLoc, Value key, BTreeHelper * helper){
  node->keys[keyLoc] = key;
  int offset = sizeof(char)+sizeof(int)+sizeof(int)+sizeof(int)+sizeof(int)+sizeof(Value)*keyLoc;
  BM_PageHandle * phNode = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,phNode,node->pageNum);
  //BM_PageHandle * phNode = node->data;
  *(Value *)(phNode->data+offset)=key;
  markDirty(helper->bp,phNode);
  unpinPage(helper->bp,phNode);
  free(phNode);
}

void setChild(BTreeNode * node, int childNum, BTreeNode * child, int nValue, BTreeHelper * helper){
  
  int offset = sizeof(char)+sizeof(int)+sizeof(int)+sizeof(int)+sizeof(int)+sizeof(Value)*nValue+sizeof(int)*childNum; //Is "Value" correct?
  //BM_PageHandle * phNode = node->data;
  BM_PageHandle * phNode = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,phNode,node->pageNum);
  if(child==NULL){
    *(int *)(phNode->data+offset)=0;
    node->children[childNum] = 0;
  }
  else{
    //BM_PageHandle * phChild = child->data;
    *(int *)(phNode->data+offset)=child->pageNum;
    node->children[childNum] = child->pageNum;
  }
  markDirty(helper->bp,phNode);
  unpinPage(helper->bp,phNode);
  free(phNode);
}

void setRID(BTreeNode * node, int ridNum, RID rid, int nValue, BTreeHelper * helper){
  BM_PageHandle * phNode = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,phNode,node->pageNum);
  if(node->isLeaf){
    node->rids[ridNum] = rid;
    int offset = sizeof(char)+sizeof(int)+sizeof(int)+sizeof(int)+sizeof(int)+sizeof(Value)*nValue+sizeof(int)*nValue+sizeof(RID)*ridNum;

   // BM_PageHandle * phNode = node->data;
    *(RID *)(phNode->data+offset) = rid;
    markDirty(helper->bp,phNode);
  }
  unpinPage(helper->bp,phNode);
  free(phNode);
}

void setNumNodes(BTreeHandle * handle, int numNodes){
  BTreeHelper * helper = handle->mgmtData;
  helper->numNodes = numNodes;
  BM_BufferPool * bm = helper->bp;
  BM_PageHandle * h  = MAKE_PAGE_HANDLE();
  pinPage(helper->bp,h,0);
  SM_PageHandle headerData = h->data;
  *(int *)(headerData) = helper->numNodes;
  markDirty(bm,h);
  unpinPage(bm,h);
  free(h);
}

// init and shutdown index manager
RC initIndexManager (void *mgmtData){
  printf("Index manager Initialized!\n");
  return RC_OK;
}

RC shutdownIndexManager (){
  printf("Index manager Shutdown\n");
  return RC_OK;
}

// create, destroy, open, and close an btree index

//This function initializes a new emtpy Btree. It first opens a new page file named *idxId. In page 0 of the page file, it then writes the keyType and n value to the file and closes the file.
RC createBtree (char *idxId, DataType keyType, int n){
  //Create pagefile
  RC status = createPageFile(idxId);
  if(status!=RC_OK){
    return status; 
  }
  SM_FileHandle fHandle;
  SM_PageHandle pHandle = malloc(PAGE_SIZE);
  memset(pHandle,'\0',PAGE_SIZE);
  openPageFile(idxId,&fHandle);
  int offset = 0;
  *(int *)(pHandle+offset) = 0; //Initialize 0 nodes
  offset+=sizeof(int);
  *(DataType *)(pHandle+offset) = keyType; //Initialize data type
  offset+=sizeof(DataType);
  *(int *)(pHandle+offset) = n; //Initialize n value
  writeBlock(0,&fHandle,pHandle);
  closePageFile(&fHandle);
  free(pHandle);
  return RC_OK;
}


RC openBtree (BTreeHandle **tree, char *idxId){
  //BTreeHandle * newTree = malloc(sizeof(BTreeHandle));
  *tree = malloc(sizeof(BTreeHandle));
  BM_BufferPool * bm = MAKE_POOL();
  BM_PageHandle * h  = MAKE_PAGE_HANDLE();
  BTreeHelper * newHelper = malloc(sizeof(BTreeHelper));
  RC status = initBufferPool(bm,idxId,10,RS_FIFO,NULL);
  if(status!=RC_OK){
    printf("ERROR: Table could not be opened!\n");
    return status;
  }
  pinPage(bm,h,0);
  SM_PageHandle headerData = h->data;
  int offset = 0;
  newHelper->numNodes = *(int *)(headerData+offset);
  offset+=sizeof(int);
  (*tree)->keyType = *(DataType *)(headerData+offset);
  offset+=sizeof(DataType);
  newHelper->n = *(int *)(headerData+offset);
  (*tree)->idxId = idxId;
  newHelper->bp = bm;
  newHelper->nodeArray = malloc(sizeof(BTreeNode*)*newHelper->numNodes*10); //*10 added to reduce number of re-allocs required
  //Initialize node array
  for(int z = 0; z<newHelper->numNodes*10;z++){
    newHelper->nodeArray[z] = NULL;
  }
  unpinPage(bm,h);
  (*tree)->mgmtData = newHelper;
  int nodeCount = 0;
  int currentPage = 1;
  newHelper->root = 0;
  //iterate through pages 1...n where n is the number of nodes in the file
  while(nodeCount<newHelper->numNodes){
    pinPage(bm,h,currentPage);
    //If the pinned page has a node in it
    if(*(char *)(h->data)=='O'){
      newHelper->nodeArray[nodeCount] = readNode(newHelper->n,h,*tree);
      nodeCount++;
      int nodeParent = getParent(newHelper->nodeArray[nodeCount]);
      if(nodeParent==0){
        newHelper->root = currentPage;
      }
    }
    unpinPage(bm,h);
    currentPage++;
  }
  newHelper->root = findRootPage(newHelper,*tree); //Intialize root
  free(h); 
  return RC_OK;
}

int findRootPage(BTreeHelper * helper, BTreeHandle *tree){
  BM_PageHandle * h  = MAKE_PAGE_HANDLE();
  BM_BufferPool * bp = helper->bp;
  //int page = 1;
  //If there is no current root node, create one
  if(helper->root==0){
    pinPage(bp,h,1);
    BTreeNode * newRoot = createNode(helper->n,true,h,tree);
    setNotEmpty(newRoot,helper);
    markDirty(bp,h);
    unpinPage(bp,h);
    free(h);
    return 1;
  }
  return helper->root;
}

RC closeBtree (BTreeHandle *tree){
  BTreeHelper * helper = tree->mgmtData;
  for(int x = 0; x<helper->numNodes; x++){
    destroyNode(helper->nodeArray[x]);
  }
  free(helper->nodeArray);
  shutdownBufferPool(helper->bp);
  free(helper->bp);
  free(helper);
  free(tree);
  return RC_OK;
}

RC deleteBtree (char *idxId){
  destroyPageFile(idxId);
  return RC_OK;
}

// access information about a b-tree
RC getNumNodes (BTreeHandle *tree, int *result){
  BTreeHelper * helper = tree->mgmtData;
  *result = helper->numNodes;
  return RC_OK;
}

RC getNumEntries (BTreeHandle *tree, int *result){
  int total = 0;
  BTreeHelper * helper = tree->mgmtData;
  for(int x = 0; x<helper->numNodes; x++){
    BTreeNode * currentNode = helper->nodeArray[x];
    if(currentNode->isLeaf){
      total+=currentNode->numEntries;
    }
  }
  *result = total+1;
  //*result = total;
  return RC_OK;
}

RC getKeyType (BTreeHandle *tree, DataType *result){
  *result = tree->keyType;
  return RC_OK;
}

// index access
RC findKey (BTreeHandle *tree, Value *key, RID *result){
  BTreeHelper * helper = tree->mgmtData;
  for(int x = 0; x<helper->numNodes; x++){
    BTreeNode * currentNode = helper->nodeArray[x];
    if(currentNode->isLeaf){
      for(int y = 0; y<currentNode->n; y++){
        Value r;
        Value * k = getKeys(currentNode,y); //Make keys just an array?
        valueEquals (k, key, &r);
        if(r.v.boolV){
          result = getRIDS(currentNode,y); //is this correct?
          return RC_OK;
        }
      }
    }
  }
  return RC_IM_KEY_NOT_FOUND;
}

BTreeNode * findRoot(BTreeHelper * helper){
  for(int x = 0; x<helper->numNodes; x++){
    BTreeNode * currentNode = helper->nodeArray[x];
    if(currentNode->parent==0){
      return helper->nodeArray[x];
    }
  }
  return NULL;
}

RC insertKey (BTreeHandle *tree, Value *key, RID rid){
  RID res;
  if(findKey(tree,key,&res)==RC_OK){
    return RC_IM_KEY_ALREADY_EXISTS;
  }
  BTreeHelper * helper = tree->mgmtData;
  BTreeNode * root = findRoot(helper);
  int n = helper->n;
  BTreeNode * leaf = insertIntoLeaf(root,key,rid,n,NULL,tree);
  if(leaf!=NULL){
    Value * key = getKeys(leaf,0);
    BTreeNode * nonLeaf = insertIntoNonLeaf(getParentNode(leaf,helper),leaf,key,n,tree);
    if(nonLeaf!=NULL){
      //Creat new root node and insert key
    }
  }
  return RC_OK;
}

BTreeNode * insertIntoNonLeaf(BTreeNode * node, BTreeNode * newChild, Value * key, int n, BTreeHandle * handle){
     BTreeHelper * helper = handle->mgmtData;
     if(node->numEntries<n){
      Value result1;
      Value result2;
      Value minimum;
      minimum.dt=DT_INT;
      minimum.v.intV=INT_MIN;
      for(int x =0; x<n; x++){
        valueEquals(getKeys(node,x),&minimum,&result1);
        valueSmaller(key,getKeys(node,x),&result2);
        if(result1.v.boolV==true){
          setKey(node, x, *key,helper);
          setChild(node,x+1,newChild,n,helper);
          node->numEntries++;
          return NULL;
        }
        else if(result2.v.boolV==true){
          shiftRightAndInsert(node,x,key,NULL,newChild,n,helper);
          node->numEntries++;
          return NULL;
        }
     }
   }
   //Split node and insert key
   BTreeNode * nonLeaf = splitNonLeaf(node,newChild,key,n,handle);
   if(nonLeaf!=NULL){
     return insertIntoNonLeaf(getParentNode(node,helper),nonLeaf,getKeys(nonLeaf,0),n,handle);
   }
   return nonLeaf;
}
//Note * parent was removed
BTreeNode * splitNonLeaf(BTreeNode * node, BTreeNode * newChild, Value * key, int n, BTreeHandle * handle){
	//Create new node
	BTreeHelper * helper = handle->mgmtData;
	BM_PageHandle * h = MAKE_PAGE_HANDLE();
	BM_BufferPool * bp = helper->bp;
	int openPage = findOpenPage(helper);
	pinPage(bp,h,openPage);
	BTreeNode * newNode = createNode(n,false,h,handle);
	setNotEmpty(newNode,helper);
	markDirty(bp,h);
	//Make origal node not a leaf node
	setIsLeaf(node,false,helper);
	//THIS IS WHERE YOU LEFT OFF
	//Populate new node
	int rightNodeEndIndex = (n/2)-1; //What is n = 1?
	int leftNodeEndIndex = n-1;
	Value result;
	bool keyUsed = false;
	while(rightNodeEndIndex>=0){
	  valueSmaller(getKeys(node,leftNodeEndIndex),key,&result);
	  if(!keyUsed && result.v.boolV){
	    setKey(newNode,rightNodeEndIndex,*key,helper);
	    setChild(newNode,rightNodeEndIndex+1,newChild,n,helper);
	    keyUsed = true;
	  }
	  else{
	    setKey(newNode,rightNodeEndIndex,*(getKeys(node,leftNodeEndIndex)),helper);
	    setChild(newNode,rightNodeEndIndex+1,getChildNode(node,leftNodeEndIndex+1,helper),n,helper);
	    Value tkey;
	    tkey.dt=DT_INT;
	    tkey.v.intV = INT_MIN;
	    setKey(node,leftNodeEndIndex,tkey,helper);
	    setChild(node,leftNodeEndIndex+1,NULL,n,helper);
	    leftNodeEndIndex--;
	    node->numEntries--;
	  }
	  newNode->numEntries++;
	  rightNodeEndIndex--;
	}
	if(!keyUsed){
	  while(leftNodeEndIndex>=0){
	    valueSmaller(getKeys(node,leftNodeEndIndex),key,&result);
	    if(result.v.boolV){
	      shiftRightAndInsert(node,leftNodeEndIndex+1,key,NULL,newChild,n,helper);
	      break;
	    }
	    leftNodeEndIndex--;
	  }
	}
        //Do i need to update sibling if it was a root? or in any other case?
        unpinPage(bp,h);
	//If the node split is the root
	if(getParentNode(node,helper)==NULL){
	  //create new root
	  openPage = findOpenPage(helper);
	  pinPage(bp,h,openPage);
	  BTreeNode * newRoot = createNode(n,false,h,handle);
	  setNotEmpty(newRoot,helper);
	  BTreeHelper * newHelper = handle->mgmtData;
	  newHelper->root = getPageNum(newRoot);
	  setParent(node,newRoot,helper);
	  setParent(newNode,newRoot,helper);
	  setParent(newRoot,NULL,helper);
	  //Initialiate root and insert right most value of left node into root
	  setKey(newRoot,0,*(getKeys(node,node->numEntries-1)), helper);
	  newRoot->numEntries++;
	  setChild(newRoot,0,node,n,helper);
	  setChild(newRoot,1,newNode,n,helper);
	  Value temp;
	  temp.dt = DT_INT;
	  temp.v.intV = INT_MIN;
	  setKey(node,node->numEntries-1,temp,helper);
	  node->numEntries--;
	  setChild(node,node->numEntries,NULL,n,helper);
	  markDirty(bp,h);
	  unpinPage(bp,h);
	  free(h);
	  return NULL;
	}
	Value temp;
	temp.dt = DT_INT;
	temp.v.intV = INT_MIN;
	setKey(node,node->numEntries-1,temp,helper);
	node->numEntries--;
	setChild(node,node->numEntries,NULL,n,helper);
	free(h);
	return newNode;
}

BTreeNode * insertIntoLeaf(BTreeNode * node, Value * key, RID rid, int n, BTreeNode * parent, BTreeHandle * handle){
    BTreeHelper * helper = handle->mgmtData;
    if(node->isLeaf && node->numEntries<n){
      for(int x =0; x<n; x++){
      //YOU CAN USE HELPER METHOD FROM expr.c FOR NEXT LINE
        if(node->keys[x].v.intV==INT_MIN){
          setKey(node, x, *key,helper);
          setRID(node, x, rid, n,helper);
          node->numEntries++;
          return NULL;
        }
      //YOU CAN USE HELPER METHOD FROM expr.c FOR NEXT LINE
        else if(node->keys[x].v.intV>key->v.intV){
          shiftRightAndInsert(node,x,key,&rid,NULL,n,helper);
          node->numEntries++;
          return NULL;
        }
     }
   }
   if(node->isLeaf && node->numEntries==n){
    //Split node and insert middle value above
    return splitLeaf(node,key,rid,n,parent,handle);
   }
   int x = 0;
   while(x<node->numEntries){
     if(key->v.intV<node->keys[x].v.intV){
       break;
     }
     x++;
   }
   BTreeNode * result = insertIntoLeaf(getChildNode(node,x,helper),key,rid,n,node,handle);
   return result;
}
   
BTreeNode * splitLeaf(BTreeNode * node, Value * key, RID rid, int n, BTreeNode * parent, BTreeHandle * handle){
	//Create new node
	BTreeHelper * helper = handle->mgmtData;
	BM_PageHandle * h = MAKE_PAGE_HANDLE();
	BM_BufferPool * bp = helper->bp;
	int openPage = findOpenPage(helper);
	pinPage(bp,h,openPage);
	BTreeNode * newNode = createNode(n,true,h,handle);
	setNotEmpty(newNode,helper);
	markDirty(bp,h);
	//Make origal node a leaf node since no split leaf can be a non leaf node
	setIsLeaf(node,true,helper);
	//Populate new node
	int rightNodeEndIndex = (n/2)-((n+1)%2);
	int leftNodeEndIndex = n-1;
	Value result;
	bool keyUsed = false;
	while(rightNodeEndIndex>=0){
	  valueSmaller(getKeys(node,leftNodeEndIndex),key,&result);
	  if(!keyUsed && result.v.boolV){
	    setKey(newNode,rightNodeEndIndex,*key,helper);
	    setRID(newNode,rightNodeEndIndex,rid,n,helper);
	    keyUsed = true;
	  }
	  else{
	    setKey(newNode,rightNodeEndIndex,*(getKeys(node,leftNodeEndIndex)),helper);
	    setRID(newNode,rightNodeEndIndex,*(getRIDS(node,leftNodeEndIndex)),n,helper);
	    Value temp;
	    temp.dt = DT_INT;
	    temp.v.intV = INT_MIN;
	    setKey(node,leftNodeEndIndex,temp,helper);
	    //setRID(newNode,rightNodeEndIndex,node->rids[leftNodeEndIndex],n); //NOTE: RID IS NOT GETTING CLEARED
	    leftNodeEndIndex--;
	    node->numEntries--;
	  }
	  newNode->numEntries++;
	  rightNodeEndIndex--;
	}
	if(!keyUsed){
	  while(leftNodeEndIndex>=0){
	    valueSmaller(getKeys(node,leftNodeEndIndex),key,&result);
	    if(result.v.boolV){
	      shiftRightAndInsert(node,leftNodeEndIndex+1,key,&rid,NULL,n,helper);
	      break;
	    }
	    leftNodeEndIndex--;
	  }
	}
	//Initialize left and right sibling of new node
	setLeftSibling(newNode,node, helper);
	setRightSibling(newNode,getRightSiblingNode(node,helper),helper);
	//Initialize parent of new node
        if(getParentNode(node,helper)==NULL){
	  //create new root
	  openPage = findOpenPage(helper);
	  pinPage(bp,h,openPage);
	  BTreeNode * newRoot = createNode(n,false,h,handle);
	  setNotEmpty(newRoot,helper);
	  BTreeHelper * newHelper = handle->mgmtData;
	  newHelper->root = getPageNum(newRoot);
	  setParent(node,newRoot,helper);
	  setParent(newNode,newRoot,helper);
	  setParent(newRoot,NULL,helper);
	  //Initialiate root and insert right most value of left node into root
	  setKey(newRoot,0,*(getKeys(node,node->numEntries-1)), helper);
	  newRoot->numEntries++;
	  setChild(newRoot,0,node,n,helper);
	  setChild(newRoot,1,newNode,n,helper);
	  Value temp;
	  temp.dt = DT_INT;
	  temp.v.intV = INT_MIN;
	  setKey(node,node->numEntries-1,temp,helper);
	  node->numEntries--;
	  setChild(node,node->numEntries,NULL,n,helper);
	  markDirty(bp,h);
	  unpinPage(bp,h);
	  free(h);
	  return NULL;
	}
	setParent(newNode, getParentNode(node,helper),helper); //Does this need to be done here?
	//Update left sibling and right sibling of new node
	setRightSibling(node,newNode,helper);
	setLeftSibling(getRightSiblingNode(newNode,helper),newNode,helper);
	unpinPage(bp,h);
	free(h);
	return newNode;
}
	  
//Scan page file to find first open page
int findOpenPage(BTreeHelper * helper){
  BM_PageHandle * h  = MAKE_PAGE_HANDLE();
  BM_BufferPool * bp = helper->bp;
  int page = 1;
  while(true){
    pinPage(bp,h,page);
    if(*(char *)(h->data)!='O'){
      unpinPage(bp,h);
     // free(h);
      break;
    }
    unpinPage(bp,h);
    page++;
  }
  free(h);
  return page;
}

RC shiftRightAndInsert(BTreeNode * node, int index, Value * key, RID * rid, BTreeNode * child, int n, BTreeHelper * helper){
  for(int x = n-1; x>index; x--){
    setKey(node, x, *(getKeys(node,x-1)),helper);
    if(rid!=NULL){
      setRID(node, x, *(getRIDS(node,x-1)), n,helper);
    }
    else if(child!=NULL){
      setChild(node,x+1,getChildNode(node,x,helper),n,helper);
    }
  }
  setKey(node, index, *key,helper);
  if(rid!=NULL){
    setRID(node,index, *rid, n,helper);
  }
  else if(child!=NULL){
    setChild(node,index,child,n,helper);
  }
  node->numEntries++;
  return RC_OK;
}

RC deleteKey (BTreeHandle *tree, Value *key){
  return RC_OK;
}

RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle){
  return RC_OK;
}

RC nextEntry (BT_ScanHandle *handle, RID *result){
  return RC_OK;
}

RC closeTreeScan (BT_ScanHandle *handle){
  return RC_OK;
}

// debug and test functions
char *printTree(BTreeHandle *tree)
{
  BTreeNode *root, *temp1, *temp2;

  root = tree->mgmtData;
  int i = 0, k = 0;

  while (root != NULL)
  {
    for (i = 0; i < root->n; i++)
    {
      printf(" %d", root->keys[i]);
    }
    printf("\t");
    root = root->next;
  }
  printf("\n");

  root = tree->mgmtData;
  while (root->children != NULL)
  {
    root = root->children;
    while (root != NULL)
    {
      i = 0;
      while (i < root->n)
      {

        printf(" %d", root->keys[i]);
        i++;
      }
      printf("\t");

      root = root->next;
    }
    printf("\n");
  }
  printf("\n");
  return NULL;
}
