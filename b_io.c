/**************************************************************
* Class:  CSC-415-0# Spring 2023
* Name:Esau Bojorquez Medina
* GitHub UserID: Esau 4119
* Project: Assignment 5 – Buffered I/O
*
* File: b_io.c
*
* Description:The assignment is to write a C program that 
* implements a Buffer I/O system. We have functions 
* such as open, read and close that we can use in our file system. 
*
*
**************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "b_io.h"
#include "fsLowSmall.h"

#define MAXFCBS 20	//The maximum number of files open at one time


// This structure is all the information needed to maintain an open file
// It contains a pointer to a fileInfo strucutre and any other information
// that you need to maintain your open file.
typedef struct b_fcb{
	fileInfo * fi;	//holds the low level systems file info
	int bufferPos; // Buffer's position. 
	char* buffer; // buffer is needed for reading
	
	} b_fcb;
	
//static array of file control blocks
b_fcb fcbArray[MAXFCBS];

// Indicates that the file control block array has not been initialized
int startup = 0;	

// Method to initialize our file system / file control blocks
// Anything else that needs one time initialization can go in this routine
void b_init (){
	if (startup)
		return;			//already initialized

	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++){
		//indicates a free fcbArray
		fcbArray[i].fi = NULL; 
		//unallocated buffer
		fcbArray[i].buffer = NULL; 
		//Marking start of file
        fcbArray[i].bufferPos = 0; 
		}
		
	startup = 1;
	}

//Method to get a free File Control Block FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++){
		if (fcbArray[i].fi == NULL){
			// used but not assigned
			fcbArray[i].fi = (fileInfo *)-2; 
			//Not thread safe but okay for this project
			return i;		
		}
	}

	return (-1);  //all in use
	}

// b_open is called by the "user application" to open a file.  This routine is 
// similar to the Linux open function.  	
// You will create your own file descriptor which is just an integer index into an
// array of file control blocks (fcbArray) that you maintain for each open file.  
// For this assignment the flags will be read only and can be ignored.

b_io_fd b_open (char * filename, int flags){
	//Initialize our system
	if (startup == 0) b_init();  

	// This is where you are going to want to call GetFileInfo and b_getFCB
    b_io_fd fileDes = b_getFCB();
	fcbArray[fileDes].fi = GetFileInfo(filename);

	//allocate memory for our buffer  
	fcbArray[fileDes].buffer = (char*) malloc(B_CHUNK_SIZE * sizeof(char));
	
	//reset the current buffer's position
	fcbArray[fileDes].bufferPos = 0; 
    if (fileDes < 0) {
        return -1; 
    }

	//if the file or buffer is not found not found 
    if (fcbArray[fileDes].fi == NULL ||fcbArray[fileDes].buffer == NULL) {
        return -1; 
    }


	return fileDes;

	}



// b_read functions just like its Linux counterpart read.  The user passes in
// the file descriptor (index into fcbArray), a buffer where thay want you to 
// place the data, and a count of how many bytes they want from the file.
// The return value is the number of bytes you have copied into their buffer.
// The return value can never be greater then the requested count, but it can
// be less only when you have run out of bytes to read.  i.e. End of File	
int b_read (b_io_fd fileDes, char * buffer, int count)
	{
	//*** TODO ***//  
	// Write buffered read function to return the data and # bytes read
	// You must use LBAread and you must buffer the data in B_CHUNK_SIZE byte chunks.
		
	if (startup == 0) b_init();  //Initialize our system

	// check that fileDes is between 0 and (MAXFCBS-1)
	if ((fileDes < 0) || (fileDes >= MAXFCBS)){
		return (-1); 					//invalid file descriptor
		}

	// and check that the specified FCB is actually in use	
	if (fcbArray[fileDes].fi == NULL){		//File not open for this descriptor
		
		return -1;
		}	

	// Your Read code here - the only function you call to get data is LBAread.
	// Track which byte in the buffer you are at, and which block in the file
	int bRead = 0; //track the number of bytes read so far
	
	//Read until the end of the file
	while (fcbArray[fileDes].bufferPos < fcbArray[fileDes].fi->fileSize 
			&& bRead < count  ) {
		// calculate the bytes left in the current buffer
		int leftOver = 
			B_CHUNK_SIZE - fcbArray[fileDes].bufferPos % B_CHUNK_SIZE;
		// track how many bytes are left to be read,
		int bytesToRead = count - bRead;

		//Loading the next chunck if we are at the end. 
		// If not at the end, we track the remaining bytes to read. 
		if (leftOver == 0) {
			//the only function you call to get data is LBAread.
		  LBAread(fcbArray[fileDes].buffer,
		   fcbArray[fileDes].fi->location + fcbArray[fileDes].bufferPos / B_CHUNK_SIZE,1);
		}else if(bytesToRead > leftOver){
			bytesToRead = leftOver;
		}

	  // memcopy data that is in our buffer to provided buffer
		memcpy(buffer + bRead,
		 fcbArray[fileDes].buffer + fcbArray[fileDes].bufferPos % B_CHUNK_SIZE,
		 bytesToRead);
		fcbArray[fileDes].bufferPos= fcbArray[fileDes].bufferPos + bytesToRead;
		bRead = bRead + bytesToRead; 
	}

	// if we've read to the end of the file, return 0
	if (fcbArray[fileDes].bufferPos == fcbArray[fileDes].fi->fileSize)return 0;
	return bRead;

	}
	


// b_close frees and allocated memory and places the file control block back   
// into the unused pool of file control blocks.
int b_close (b_io_fd fileDes)
	{
	//*** TODO ***//  Release any resources
		if (startup == 0) {
		b_init(); //Initialize our system if it's not initialized
	}

	if (fileDes < 0 || fileDes >= MAXFCBS) {
		return -1;
	}

	// and check that the specified FCB is actually in use
	if (fcbArray[fileDes].fi == NULL) { //File not open for this descriptor
		return -1;
	}
	// Reset info
	fcbArray[fileDes].fi = NULL;
	// Reset Buffer
	fcbArray[fileDes].buffer = NULL;
	// Freeing buffer
	free(fcbArray[fileDes].buffer);
	// resetting buffer position 
	fcbArray[fileDes].bufferPos = 0;

	return 0;
	}
	
