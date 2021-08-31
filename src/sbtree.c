/******************************************************************************/
/**
@file		sbtree.c
@author		Ramon Lawrence
@brief		Implementation for sequential B-tree.
@copyright	Copyright 2021
			The University of British Columbia,
			Ramon Lawrence		
@par Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

@par 1.Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

@par 2.Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.

@par 3.Neither the name of the copyright holder nor the names of its contributors
	may be used to endorse or promote products derived from this software without
	specific prior written permission.

@par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "sbtree.h"


/*
Comparison functions. Code is adapted from ldbm.
*/
/**
@brief     	Compares two unsigned int32_t values.
@param     	a
                value 1
@param     	b
                value 2
*/
static int8_t uint32Compare(void *a, void *b)
{	
	uint32_t i1, i2;
    memcpy(&i1, a, sizeof(uint32_t));
    memcpy(&i2, b, sizeof(uint32_t));
    int32_t result = i1 - i2;
	if(result < 0) return -1;
	if(result > 0) return 1;
    return 0;
}

/**
@brief     	Compares two values by bytes. 
@param     	a
                value 1
@param     	b
                value 2
*/
/*
static int8_t byteCompare(void *a, void *b, int16_t size)
{
	return memcmp(a, b, size);	
}
*/

/**
@brief     	Initialize an SBTree structure.
@param     	state
                SBTree algorithm state structure
*/
void sbtreeInit(sbtreeState *state)
{	
	printf("Initializing SBTree.\n");
	state->recordSize = state->keySize + state->dataSize;	
	printf("Buffer size: %d  Page size: %d Record size: %d\n", state->buffer->numPages, state->buffer->pageSize, state->recordSize);		
	
	dbbufferInit(state->buffer);	
	state->buffer->activePath = state->activePath;	

	state->compareKey = uint32Compare;
	
	/* Set block header size */
	/* Header size fixed: 6 bytes: 4 byte id, 2 for record count. */	
	state->headerSize = 6;	

	/* Calculate number of records per page */
	state->maxRecordsPerPage = (state->buffer->pageSize - state->headerSize) / state->recordSize;
	/* Interior records consist of key and id reference. Note: One extra id reference (child pointer). If N keys, have N+1 id references (pointers). */
	state->maxInteriorRecordsPerPage = (state->buffer->pageSize - state->headerSize -sizeof(id_t)) / (state->keySize+sizeof(id_t));

	/* Hard-code for testing */
//	state->maxRecordsPerPage = 10;
//	state->maxInteriorRecordsPerPage = 3;	
	state->levels = 1;

	/* Create and write empty root node */
	state->writeBuffer = initBufferPage(state->buffer, 0);
	SBTREE_SET_ROOT(state->writeBuffer);		
	state->activePath[0] = writePage(state->buffer, state->writeBuffer);		/* Store root location */	

	/* Allocate first page of buffer as output page for data records */	
	initBufferPage(state->buffer, 0);
}


/**
@brief     	Return the smallest key in the node
@param     	state
                SBTree algorithm state structure
@param     	buffer
                In memory page buffer with node data
*/
void* sbtreeGetMinKey(sbtreeState *state, void *buffer)
{
	return (void*) (buffer+state->headerSize);
}

/**
@brief     	Return the smallest key in the node
@param     	state
                SBTree algorithm state structure
@param     	buffer
                In memory page buffer with node data
*/
void* sbtreeGetMaxKey(sbtreeState *state, void *buffer)
{
	int16_t count =  SBTREE_GET_COUNT(buffer); 
	if (count == 0)
		count = 1;		/* Force to have value in buffer. May not make sense but likely initialized to 0. */
	return (void*) (buffer+state->headerSize+(count-1)*state->recordSize);
}

void printSpaces(int num)
{
	for (int i=0; i < num; i++)
		printf(" ");
}

/**
@brief     	Print a node in an in-memory buffer.
@param     	state
                SBTree algorithm state structure
@param     	pageNum
                Physical page id (number)	
@param     	depth
                Used for nesting print out
@param     	buffer
                In memory page buffer with node data
*/
void sbtreePrintNodeBuffer(sbtreeState *state, id_t pageNum, int depth, void *buffer)
{
	int16_t count =  SBTREE_GET_COUNT(buffer); 
		
	if (SBTREE_IS_INTERIOR(buffer))
	{					
		printSpaces(depth*3);
		printf("Id: %lu Loc: %lu Cnt: %d [%d, %d]\n", SBTREE_GET_ID(buffer), pageNum, count, (SBTREE_IS_ROOT(buffer)), SBTREE_IS_INTERIOR(buffer));		
		/* Print data records (optional) */	
		/*
		uint32_t key, val;
		printSpaces(depth*3);		
		for (int c=0; c < count && c < state->maxInteriorRecordsPerPage; c++)
		{			
			memcpy(&key, (int32_t*) (buffer+state->keySize * c + state->headerSize), sizeof(int32_t));
			memcpy(&val, (int32_t*) (buffer+state->keySize * state->maxInteriorRecordsPerPage + state->headerSize + c*sizeof(id_t)), sizeof(int32_t));			
			
			printf(" (%lu, %lu)", key, val);						
		}
		*/
		/* Print last pointer */
		/*
		memcpy(&val, (int32_t*) (buffer+state->keySize * state->maxInteriorRecordsPerPage + state->headerSize + c*sizeof(id_t)), sizeof(uint32_t));		
		printf(" (, %lu)\n", val);		
		*/
	}
	else
	{		
		printSpaces(depth*3);
		uint32_t minkey, maxkey;
		memcpy(&minkey, (int32_t*) sbtreeGetMinKey(state, buffer), sizeof(uint32_t));
		memcpy(&maxkey, (int32_t*) sbtreeGetMaxKey(state, buffer), sizeof(uint32_t));
		printf("Id: %lu Loc: %lu Cnt: %d (%lu, %lu)\n", SBTREE_GET_ID(buffer), pageNum, count, minkey, maxkey);
		/* Print data records (optional) */		
		/*
		uint32_t key, val;
		for (int c=0; c < count; c++)
		{
			memcpy(&key, (int32_t*) (buffer + state->headerSize + state->recordSize * c), ), sizeof(int32_t));
			memcpy(&val, (int32_t*) (buffer + state->headerSize + state->recordSize * c + state->keySize), ), sizeof(int32_t));
			printSpaces(depth*3+2);
			printf("Key: %lu Value: %lu\n",key, val);			
		}	
		*/		
	}
}


/**
@brief     	Print a node read from storage.
@param     	state
                SBTree algorithm state structure
@param     	pageNum
                Physical page id (number)	
@param     	depth
                Used for nesting print out
*/
void sbtreePrintNode(sbtreeState *state, int pageNum, int depth)
{	
	void* buf = readPage(state->buffer, pageNum);
	
	int16_t c, count =  SBTREE_GET_COUNT(buf); 	

	sbtreePrintNodeBuffer(state, pageNum, depth, buf);
	if (SBTREE_IS_INTERIOR(buf))
	{					
		int32_t val;	
		for (c=0; c < count && c < state->maxInteriorRecordsPerPage; c++)
		{						
			memcpy(&val, (int32_t*) (buf+state->keySize * state->maxInteriorRecordsPerPage + state->headerSize + c*sizeof(id_t)), sizeof(int32_t));
			
			sbtreePrintNode(state, val, depth+1);	
			buf = readPage(state->buffer, pageNum);			
		}	
		
		/* Print last child node if active */				
		memcpy(&val, (int32_t*) (buf+state->keySize * state->maxInteriorRecordsPerPage + state->headerSize + c*sizeof(id_t)), sizeof(int32_t));
		if (val != 0)	
		{
			if (depth+1 < state->levels && pageNum == state->activePath[depth])
			{	/* Current pointer is on active path. */
				if (state->activePath[depth+1] != val)
				{	printSpaces(depth*3);	
					printf("Node mapping. Node: %lu Current: %lu\n", val, state->activePath[depth+1]);
				}
				val = state->activePath[depth+1];
			}	
			sbtreePrintNode(state, val, depth+1);	
		}
	}	
}


/**
@brief     	Print current SBTree as written on storage.
@param     	state
                SBTree algorithm state structure
*/
void sbtreePrint(sbtreeState *state)
{	
	printf("\n\nPrint tree:\n");
	sbtreePrintNode(state, state->activePath[0], 0);
}


/**
@brief     	Updates the B-tree index structure from leaf node to root node as required.
@param     	state
                SBTree algorithm state structure
@param     	minkey
                smallest key in currently full buffer page
@param     	key
                current key being inserted				
@param     	pageNum
                Physical page id of full leaf page just written to storage
*/
int8_t sbtreeUpdateIndex(sbtreeState *state, void *minkey, void *key, id_t pageNum)
{		
	/* Read parent pages (nodes) until find space for new interior pointer (key, pageNum) */
	int8_t l = 0;
	int16_t count;
	int32_t prevPageNum = -1;
	void *buf;

	for (l=state->levels-1; l >= 0; l--)
	{
		/* Forcing all reads to buffer 0 guarantees no read conflicts but results in more I/Os */
		// buf = readPageBuffer(state->buffer, state->activePath[l], 0);		
		buf = readPage(state->buffer, state->activePath[l]);	
		if (buf == NULL)
			return -1;		
		
		/* Determine if there is space in the page */		
		count =  SBTREE_GET_COUNT(buf); 
					
		if ( (count > state->maxInteriorRecordsPerPage) || (l < state->levels-1 && count >= state->maxInteriorRecordsPerPage))
		{	/* Interior node at this level is full. Create a new node. */	

			/* If tree is beyond level 1, update parent node last child pointer as will have changed. Currently in buffer. */											
			if (l < state->levels - 1)
			{
				memcpy(buf + state->keySize * state->maxInteriorRecordsPerPage + sizeof(id_t) * (count) + state->headerSize, &prevPageNum, sizeof(id_t));											
				state->activePath[l]  = writePage(state->buffer, buf);				
			}
			else
			{	/* If using deferred update, must write out full node */
			 	state->activePath[l]  = writePage(state->buffer, buf);
			}
			
			initBufferPage(state->buffer, 0);
			SBTREE_SET_INTERIOR(state->writeBuffer);
			buf = state->writeBuffer;

			/* Store pointer to new leaf node */
			/* For first interior node level above leaf the separator is the currently inserted key. For other levels no key inserted just pointer. */
			if (l == state->levels-1)
			{	memcpy(buf + state->headerSize, key, state->keySize);
				SBTREE_INC_COUNT(buf);	
			}			
			
			/* Insert child pointer into new node */
			memcpy(buf + state->keySize * state->maxInteriorRecordsPerPage + state->headerSize, &pageNum, sizeof(id_t));

			/* Write page. Update active page mapping. */
			prevPageNum = state->activePath[l];
			state->activePath[l] = writePage(state->buffer, buf);												
			pageNum = state->activePath[l];						
		}
		else 
		{
			/* Copy record onto page */
			/* Record is key just trying to insert (as above values already in block) and pageNum just written with previous data page */
			/* Keep keys and data as contiguous sorted arrays */
			if (count < state->maxInteriorRecordsPerPage)
			{	/* Do not store key for last child pointer */ 
				if (l == state->levels-1)
					memcpy(buf + state->keySize * count + state->headerSize, key, state->keySize);
				else
					memcpy(buf + state->keySize * count + state->headerSize, minkey, state->keySize);				
			}
		
			if (l == 0 && state->levels > 1)
			{	/* Root is special case */
				memcpy(buf + state->keySize * state->maxInteriorRecordsPerPage + sizeof(id_t) * (count+1) + state->headerSize, &pageNum, sizeof(id_t));		
				
				/* Update previous pointer as may have changed due to writes. */
				if (count > 0 && prevPageNum != -1)
					memcpy(buf + state->keySize * state->maxInteriorRecordsPerPage + sizeof(id_t) * (count) + state->headerSize, &prevPageNum, sizeof(id_t));	
			}
			else
			{
				/* Update previous pointer as may have changed due to writes. */
				if (prevPageNum != -1)
				{										
					memcpy(buf + state->keySize * state->maxInteriorRecordsPerPage + sizeof(id_t) * (count) + state->headerSize, &prevPageNum, sizeof(id_t));	
					count++;					
				}
			
				/* Add new child pointer to page */
				memcpy(buf + state->keySize * state->maxInteriorRecordsPerPage + sizeof(id_t) * (count) + state->headerSize, &pageNum, sizeof(id_t));						
			}						
				
			/* Update count */
			SBTREE_INC_COUNT(buf);	

			/* Write updated interior page */								
			/* Update location of page */
			// state->activePath[l] = writePage(state->buffer, buf);

			/* Deferring write and keeping updated page in buffer. */
			/* Note: Requires writing page and updating active path if buffer is used for reading. */							
			dbbufferSetModified(state->buffer, buf, l);
			break;
		}		
	}		 
	
	/* Handle the case where root is currently full */
	/* Grow one level, shift everything down, and create new root */
	if (l == -1)
	{			
		initBufferPage(state->buffer, 0);

		/* Copy record onto page (minkey, prevPageNum) */
		memcpy(state->writeBuffer + state->headerSize, minkey, state->keySize);		
		memcpy(state->writeBuffer + state->headerSize + state->keySize * state->maxInteriorRecordsPerPage, &prevPageNum, sizeof(id_t));
		
		/* Copy greater than record on to page. Note: Basically child pointer and infinity for key */		
		memcpy(state->writeBuffer + state->keySize * state->maxInteriorRecordsPerPage + state->headerSize + sizeof(id_t), &state->activePath[0], sizeof(id_t));		

		/* Update count */
		SBTREE_INC_COUNT(state->writeBuffer);	
		SBTREE_SET_ROOT(state->writeBuffer);
		
		for (l=state->levels; l > 0; l--)
			state->activePath[l] = state->activePath[l-1]; 
		state->activePath[0] = writePage(state->buffer, state->writeBuffer);	/* Store root location */			
		state->levels++;
	}
	return 0;
}

/**
@brief     	Puts a given key, data pair into structure.
@param     	state
                SBTree algorithm state structure
@param     	key
                Key for record
@param     	data
                Data for record
@return		Return 0 if success. Non-zero value if error.
*/
int8_t sbtreePut(sbtreeState *state, void* key, void *data)
{		
	int16_t count =  SBTREE_GET_COUNT(state->writeBuffer); 

	/* Write current page if full */
	if (count >= state->maxRecordsPerPage)
	{	
		/* Write page first so can use buffer for updating tree structure */
		int32_t pageNum = writePage(state->buffer, state->writeBuffer);				

		/* Add pointer to page to B-tree structure */
		/* Second pointer parameter is minimum key in currently full leaf node of data */
		/* Need to copy key from current write buffer as will reuse buffer */
		memcpy(state->tempKey, (void*) (state->writeBuffer+state->headerSize), state->keySize); 
		if (sbtreeUpdateIndex(state, state->tempKey, key, pageNum))
			return -1;

		count = 0;			
		initBufferPage(state->buffer, 0);					
	}

	/* Copy record onto page */
	memcpy(state->writeBuffer + state->recordSize * count + state->headerSize, key, state->keySize);
	memcpy(state->writeBuffer + state->recordSize * count + state->headerSize + state->keySize, data, state->dataSize);

	/* Update count */
	SBTREE_INC_COUNT(state->writeBuffer);	

	return 0;
}

/**
@brief     	Given a key, searches the node for the key.
			If interior node, returns child record number containing next page id to follow.
			If leaf node, returns if of first record with that key or (<= key).
			Returns -1 if key is not found.			
@param     	state
                SBTree algorithm state structure
@param     	buffer
                Pointer to in-memory buffer holding node
@param     	key
                Key for record
@param		pageId
				Page if for page being searched
@param		range
				1 if range query so return pointer to first record <= key, 0 if exact query so much return first exact match record
*/
id_t sbtreeSearchNode(sbtreeState *state, void *buffer, void* key, id_t pageId, int8_t range)
{
	int16_t first, last, middle, count;
	int8_t compare, interior;
	void *mkey;
	
	count = SBTREE_GET_COUNT(buffer);  
	interior = SBTREE_IS_INTERIOR(buffer);

	if (interior)
	{
		if (count == 0)	/* Only one child pointer */
			return 0;
		if (count == 1)	/* One key and two children pointers */
		{
			mkey = buffer+state->headerSize;   /* Key at index 0 */
			compare = state->compareKey(key, mkey);
			if (compare < 0)
				return 0;
			return 1;		
		}
		
		first = 0;	
  		last =  count;
		if (last > state->maxInteriorRecordsPerPage)
			last = state->maxInteriorRecordsPerPage;
  		middle = (first+last)/2;
		while (first < last) 
		{			
			mkey = buffer+state->headerSize+state->keySize*middle;
			compare = state->compareKey(key,mkey);
			if (compare > 0)
				first = middle + 1;
			else if (compare == 0) 
			{	last = middle+1; /* Return the child pointer just after */
				break;
			}				
			else
				last = middle;  /* Note: Not -1 as always want last pointer to be <= key so that will use it if necessary */

			middle = (first + last)/2;
		}
		return last;		
	}
	else
	{
		first = 0;	
  		last =  count - 1;
  		middle = (first+last)/2;	

		while (first <= last) 
		{			
			mkey = buffer+state->headerSize+state->recordSize*middle;
			compare = state->compareKey(mkey, key);
			if (compare < 0)
				first = middle + 1;
			else if (compare == 0) 
				return middle;							
			else
				last = middle - 1;

			middle = (first + last)/2;
		}
		if (range)
			return middle;
		return -1;
	}
}

/**
@brief     	Given a child link, returns the proper physical page id.
			This method handles the mapping of the active path where the pointer in the
			node is not actually pointing to the most up to date block.			
@param     	state
                SBTree algorithm state structure
@param		buf
				Buffer containing node
@param     	pageId
                Page id for node
@param     	level
                Level of node in tree
@param		childNum
				Child pointer index
@return		Return pageId if success or -1 if not valid.
*/
id_t getChildPageId(sbtreeState *state, void *buf, id_t pageId, int8_t level, id_t childNum)
{
	if (childNum == (SBTREE_GET_COUNT(buf)) && level < state->levels-1 && pageId == state->activePath[level])
	{	/* Search node was on active path and accessing last child pointer. Use current mapping */
		return state->activePath[level+1];	
	}
	
	/* Retrieve page number for child */
	id_t nextId;
	memcpy(&nextId, (id_t*) (buf + state->headerSize + state->keySize*state->maxInteriorRecordsPerPage + sizeof(id_t)*childNum), sizeof(id_t));
	if (nextId == 0 && childNum==(SBTREE_GET_COUNT(buf)))	/* Last child which is empty */
		return -1;
	return nextId;
}

/**
@brief     	Given a key, returns data associated with key.
			Note: Space for data must be already allocated.
			Data is copied from database into data buffer.
@param     	state
                SBTree algorithm state structure
@param     	key
                Key for record
@param     	data
                Pre-allocated memory to copy data for record
@return		Return 0 if success. Non-zero value if error.
*/
int8_t sbtreeGet(sbtreeState *state, void* key, void *data)
{
	/* Starting at root search for key */
	int8_t 	l;
	void	*buf;
	id_t 	childNum, nextId = state->activePath[0];
	
	for (l=0; l < state->levels; l++)
	{		
		buf = readPage(state->buffer, nextId);		

		/* Find the key within the node. Sorted by key. Use binary search. */
		childNum = sbtreeSearchNode(state, buf, key, nextId, 0);
		
		nextId = getChildPageId(state, buf, nextId, l, childNum);
		if (nextId == -1)
			return -1;		
	}

	/* Search the leaf node and return search result */
	buf = readPage(state->buffer, nextId);	
	nextId = sbtreeSearchNode(state, buf, key, nextId, 0);
	if (nextId != -1)
	{	/* Key found */
		memcpy(data, (void*) (buf+state->headerSize+state->recordSize*nextId+state->keySize), state->dataSize);
		return 0;
	}
	return -1;
}

/**
@brief     	Flushes output buffer.
@param     	state
                SBTREE algorithm state structure
*/
int8_t sbtreeFlush(sbtreeState *state)
{
	int32_t pageNum = writePage(state->buffer, state->writeBuffer);	

	/* Add pointer to page to B-tree structure */		
	/* So do not have to allocate memory. Use the next key value in the buffer temporarily to store a MAX_KEY of all 1 bits */	
	/* Need to copy key from current write buffer as will reuse buffer */
	/*
	memcpy(state->tempKey, (void*) (state->buffer+state->headerSize), state->keySize); 	
	void *maxkey = state->buffer + state->recordSize * SBTREE_GET_COUNT(state->buffer) + state->headerSize;
	memset(maxkey, 1, state->keySize);
	 sbtreeUpdateIndex(state, state->tempKey, maxkey, pageNum);
	*/
	// TODO: Look at what the key should be when flush. Needs to be one bigger than data set 

	void *maxkey = state->writeBuffer + state->recordSize * (SBTREE_GET_COUNT(state->writeBuffer)-1) + state->headerSize;
	int32_t mkey;
	memcpy(&mkey, (int32_t*) maxkey, sizeof(int32_t));
	mkey++;
	maxkey = state->writeBuffer + state->headerSize;
	int32_t minKey;
	memcpy(&minKey, (int32_t*) maxkey, sizeof(int32_t));
	if (sbtreeUpdateIndex(state, &minKey, &mkey, pageNum) != 0)
		return -1;
		
	state->buffer->storage->flush(state->buffer->storage);

	/* Reinitialize buffer */
	initBufferPage(state->buffer, 0);
	return 0;
}


/**
@brief     	Initialize iterator on SBTree structure.
@param     	state
                SBTree algorithm state structure
@param     	it
                SBTree iterator state structure
*/
void sbtreeInitIterator(sbtreeState *state, sbtreeIterator *it)
{	
	/* Find start location */
	/* Starting at root search for key */
	int8_t 	l;
	void	*buf;	
	id_t 	childNum, nextId = state->activePath[0];

	it->currentBuffer = NULL;

	for (l=0; l < state->levels; l++)
	{		
		it->activeIteratorPath[l] = nextId;		
		buf = readPage(state->buffer, nextId);		

		/* Find the key within the node. Sorted by key. Use binary search. */
		childNum = sbtreeSearchNode(state, buf, it->minKey, nextId, 1);
		nextId = getChildPageId(state, buf, nextId, l, childNum);
		if (nextId == -1)
			return;	
		
		it->lastIterRec[l] = childNum;
	}

	/* Search the leaf node and return search result */
	it->activeIteratorPath[l] = nextId;	
	buf = readPage(state->buffer, nextId);
	it->currentBuffer = buf;
	childNum = sbtreeSearchNode(state, buf, it->minKey, nextId, 1);		
	it->lastIterRec[l] = childNum;
}


/**
@brief     	Requests next key, data pair from iterator.
@param     	state
                SBTree algorithm state structure
@param     	it
                SBTree iterator state structure
@param     	key
                Key for record (pointer returned)
@param     	data
                Data for record (pointer returned)
*/
int8_t sbtreeNext(sbtreeState *state, sbtreeIterator *it, void **key, void **data)
{	
	void *buf = it->currentBuffer;
	int8_t l=state->levels;
	id_t nextPage;

	/* No current page to search */
	if (buf == NULL)
		return 0;

	/* Iterate until find a record that matches search criteria */
	while (1)
	{	
		if (it->lastIterRec[l] >= SBTREE_GET_COUNT(buf))
		{	/* Read next page */						
			it->lastIterRec[l] = 0;

			while (1)
			{
				/* Advance to next page. Requires examining active path. */
				for (l=state->levels-1; l >= 0; l--)
				{	
					buf = readPage(state->buffer, it->activeIteratorPath[l]);
					if (buf == NULL)
						return 0;						

					int8_t count = SBTREE_GET_COUNT(buf);
					if (l == state->levels-1)
						count--;
					if (it->lastIterRec[l] < count)
					{
						it->lastIterRec[l]++;
						break;
					}
					it->lastIterRec[l] = 0;
				}
				if (l == -1)
					return 0;		/* Exhausted entire tree */

				for ( ; l < state->levels; l++)
				{						
					nextPage = it->activeIteratorPath[l];
					nextPage = getChildPageId(state, buf, nextPage, l, it->lastIterRec[l]);
					if (nextPage == -1)
						return 0;	
					
					it->activeIteratorPath[l+1] = nextPage;
					buf = readPage(state->buffer, nextPage);
					if (buf == NULL)
						return 0;	
				}
				it->currentBuffer = buf;
				break;				
			}
		}
		
		/* Get record */	
		*key = buf+state->headerSize+it->lastIterRec[l]*state->recordSize;
		*data = *key+state->keySize;
		it->lastIterRec[l]++;
		
		/* Check that record meets filter constraints */
		if (it->minKey != NULL && state->compareKey(*key, it->minKey) < 0)
			continue;
		if (it->maxKey != NULL && state->compareKey(*key, it->maxKey) > 0)
			return 0;	/* Passed maximum range */
		return 1;
	}
}
