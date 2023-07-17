/*
	Merge.h -- equates for Merge XOP
*/

/* Merge custom error codes */
#define OLD_IGOR 1 + FIRST_XOP_ERR

/* Prototypes */
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);

XOP_FILE_REF grawfiles[1000]; // Not likely we have more than 1000 files to open
