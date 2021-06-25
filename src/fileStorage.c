/******************************************************************************/
/**
@file		fileStorage.c
@author		Ramon Lawrence
@brief		File storage implementation for reading and writing pages of data.
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

#include "fileStorage.h"

/**
@brief     	Initializes storage. Opens file.
@param		state
                File storage state structure
@return		 Returns 0 if success, non-zero if failure.
*/
int8_t fileStorageInit(storageState *storage)
{	 
	fileStorageState *fs = (fileStorageState*) storage;
	
	fs->file = fopen(fs->fileName, "w+b");
    if (NULL == fs->file) 
		return -1;

	fs->storage.init = fileStorageInit;
	fs->storage.close = fileStorageClose;
	fs->storage.readPage = fileStorageReadPage;
	fs->storage.writePage = fileStorageWritePage;
	fs->storage.flush = fileStorageFlush;

	return 0;	
}

/**
@brief      Reads page from storage into buffer. Returns 0 if success, non-zero if failure.
@param     	state
                File storage state structure
@param     	pageNum
                Physical page id (number)
@param		pageSize
				Size of page to read in bytes
@param		buffer
				Pointer to buffer to copy data into
@return		 Returns 0 if success, non-zero if failure.
*/
int8_t fileStorageReadPage(storageState *storage, id_t pageNum, count_t pageSize, void *buffer)
{	
	fileStorageState *fs = (fileStorageState*) storage;

	SD_FILE *fp = fs->file;
  
    /* Seek to page location in file */
    fseek(fp, pageNum*pageSize, SEEK_SET);

    /* Read page into start of buffer 1 */   
    if (0 ==  fread(buffer, pageSize, 1, fp))
    	return -1;           
	   
	return 0;
}

/**
@brief      Writes page from buffer into storage. Returns 0 if success, non-zero if failure.
@param     	state
                File storage state structure
@param     	pageNum
                Physical page id (number)
@param		pageSize
				Size of page to write in bytes
@param		buffer
				Pointer to buffer to copy data into
@return		 Returns 0 if success, non-zero if failure.
*/
int8_t fileStorageWritePage(storageState *storage, id_t pageNum, count_t pageSize, void *buffer)
{    
	fileStorageState *fs = (fileStorageState*) storage;

	/* Seek to page location in file */
    fseek(fs->file, pageNum*pageSize, SEEK_SET);

	fwrite(buffer, pageSize, 1, fs->file);
	
	return 0;
}


/**
@brief     	Flush storage and ensure all data is written.
@param     	state
                File storage state structure
*/
void fileStorageFlush(storageState *storage)
{
	fileStorageState *fs = (fileStorageState*) storage;
	fflush(fs->file);
}


/**
@brief     	Closes storage and performs any needed cleanup.
@param     	state
                File storage state structure
*/
void fileStorageClose(storageState *storage)
{	
	fileStorageState *fs = (fileStorageState*) storage;
	fclose(fs->file);
}

