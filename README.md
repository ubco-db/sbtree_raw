# Sequential B-tree (SBtree) Embedded Index Structure

The sequential B-tree (SBtree) efficiently stores data in a B-tree structure that is inserted in sorted order. Inserts are buffered and require no searching in the tree. Queries can typically be performed with less than 4 page reads depending on memory size. The SBtree uses a minimal amount of memory for use with embedded systems. Key features:

1. Uses only two page buffers for performing all operations. The memory usage is less than 1.5 KB for 512 byte pages.
2. Optimized performance for sequential data such as time series data.
3. No use of dynamic memory (i.e. malloc()). All memory is pre-allocated at creation of the tree.
4. Efficient insert (put) and query (get) of arbitrary key-value data. Ability to search data on key.
5. Support for iterator to traverse data in sorted order.
6. Easy to use and include in existing projects. 
7. Open source license. Free to use for commerical and open source projects.

## Code Files

* main.cpp - main Arduino file
* test_sbtree.h - test file demonstrating how to get, put, and iterate through data in index
* sbtree.h, sbtree.c - implementation of sequential B-tree structure supporting arbitrary key-value data items
* dbbuffer.h, dbbuffer.c - provides buffering of pages in memory
* fileStorage.h, fileStorage.c - support for file based storage including on SD cards
* memStorage.h, memStorage.c - support for raw memory (NOR/NAND) storage
* storage.h - generic storage interface

## Usage

### Setup Index and Configure Memory

```c
/* Configure file-based storage. RAW flash memory storage is also possible. */
fileStorageState *storage = (fileStorageState*) malloc(sizeof(fileStorageState));
storage->fileName = (char*) "myfile.bin";
if (fileStorageInit((storageState*) storage) != 0)
{
    printf("Error: Cannot initialize storage!\n");
    return;
}

/* Configure buffer */
dbbuffer *buffer = (dbbuffer*) malloc(sizeof(dbbuffer));
buffer->pageSize = 512;
uint16_t M = 10;
buffer->numPages = M;
buffer->status = (id_t*) malloc(sizeof(id_t)*M);
buffer->buffer  = malloc((size_t) buffer->numPages * buffer->pageSize);   
buffer->storage = (storageState*) storage; 

/* Configure SBTree state */
sbtreeState *state = (sbtreeState*) malloc(sizeof(sbtreeState));

state->recordSize = 16;
state->keySize = 4;
state->dataSize = 12;           
state->buffer = buffer;

state->tempKey = malloc(sizeof(int32_t)); 

/* Initialize SBTree structure */
sbtreeInit(state);
```

### Insert (put) items into tree

```c
/* keyPtr points to key to insert. dataPtr points to associated data value. */
sbtreePut(state, (void*) keyPtr, (void*) dataPtr);
```

### Query (get) items from tree

```c
/* keyPtr points to key to search for. dataPtr must point to pre-allocated space to copy data into. */
int8_t result = sbtreeGet(state, (void*) keyPtr, (void*) dataPtr);
```

### Iterate through items in tree

```c
/* Iterator with filter on keys */
sbtreeIterator it;
uint32_t *itKey, *itData;

uint32_t minKey = 1, maxKey = 1000;     
it.minKey = &minKey; 
it.maxKey = &maxKey; 

sbtreeInitIterator(state, &it);

while (sbtreeNext(state, &it, (void**) &itKey, (void**) &itData))
{                      
	/* Process record */	
}
```
#### Ramon Lawrence<br>University of British Columbia Okanagan



