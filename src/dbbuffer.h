/******************************************************************************/
/**
@file		dbbuffer.h
@author		Ramon Lawrence
@brief		Light-weight buffer implementation for small embedded devices.
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
#ifndef DBBUFFER_H
#define DBBUFFER_H

#include <stdint.h>
#include <stdio.h>

#if defined(ARDUINO)
#include "file/serial_c_iface.h"
#endif

#include "storage.h"

#define BUFFER_EMPTY_ID		2147483647

#define NOT_MODIFIED_VAL	100

/* Define type for page ids (physical and logical). */
typedef uint32_t id_t;

/* Define type for page record count. */
typedef uint16_t count_t;

typedef struct {
	id_t*  	status;					/* Contents of buffer (physical page id)  */    
	void*  	buffer;					/* Allocated memory for buffer */
	count_t	pageSize;				/* Size of buffer page */
	count_t	numPages;				/* Number of buffer pages */    
	storageState* storage;			/* Storage information for reading/writing pages */
	id_t 	nextPageId;				/* Next logical page id. Page id is an incrementing value and may not always be same as physical page id. */
	id_t 	nextPageWriteId;		/* Physical page id of next page to write. */	
	id_t 	numWrites;				/* Number of page writes */
	id_t 	numReads;				/* Number of page reads */
	id_t 	bufferHits;				/* Number of pages returned from buffer rather than storage */
	count_t lastHit;				/* Buffer id of last buffer page hit */
	count_t nextBufferPage;			/* Next page buffer id to use. Round robin */
	id_t* 	activePath;				/* Active path on insert. Also contains root. Helps to prioritize. */
	uint8_t* modified;				/* Flag to indicate if buffer has been modified and contains node of active path */
} dbbuffer;

/**
@brief     	Initializes buffer given page size and number of pages.
@param     	state
                DBbuffer state structure
*/
void dbbufferInit(dbbuffer *state);


/**
@brief      Reads page either from buffer or from storage. Returns pointer to buffer if success.
@param     	state
                DBbuffer state structure
@param     	pageNum
                Physical page id (number)
@return		Returns pointer to buffer page or NULL if error.
*/
void* readPage(dbbuffer *state, id_t pageNum);

/**
@brief      Reads page to a particular buffer number. Returns pointer to buffer if success.
@param     	state
                DBbuffer state structure
@param     	pageNum
                Physical page id (number)
@param		bufferNum
				Buffer to read into
@return		Returns pointer to buffer page or NULL if error.
*/
void* readPageBuffer(dbbuffer *state, id_t pageNum, count_t bufferNum);

/**
@brief      Writes page to storage. Returns physical page id if success. -1 if failure.
@param     	state
                DBbuffer state structure
@param     	buffer
                In memory buffer containing page
@return		
*/
int32_t writePage(dbbuffer *state, void* buffer);


/**
@brief     	Initialize in-memory buffer page.
@param     	state
                DBbuffer state structure
@param     	pageNum
                In memory buffer page id (number)
@return		pointer to initialized page
*/
void* initBufferPage(dbbuffer *state, int pageNum);

/**
@brief     	Closes buffer.
@param     	state
                DBbuffer state structure
*/
void closeBuffer(dbbuffer *state);

/**
@brief     	Prints statistics.
@param     	state
                DBbuffer state structure
*/
void printStats(dbbuffer *state);

/**
@brief     	Clears statistics.
@param     	state
                DBbuffer state structure
*/
void dbbufferClearStats(dbbuffer *state);

/**
@brief      Sets page buffer to be modified and associated active path level.
@param     	state
                DBbuffer state structure
@param     	buffer
                In memory buffer containing page
@param		level
				Active path level page stored in buffer
@return		
*/
void dbbufferSetModified(dbbuffer *state, void* buffer, uint8_t level);

/**
@brief      Clear modified flag if page is present in a buffer.
@param     	state
                DBbuffer state structure
@param     	pageNum
                page number
@return		
*/
void dbbufferClearModified(dbbuffer *state, id_t pageNum);

#endif
