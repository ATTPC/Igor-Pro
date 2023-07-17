/*	Merge.c -- Functions to speed up merging of graw data
 
    DB, 2019-05-06
        Wrote MergeReadGraw function which is bottle neck in Igor code
        Also MergeOpenGraw and MergeCloseGraw to open and close graw files in the C code
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "Merge.h"

/* Global Variables (none) */

#pragma pack(2)        // All structures passed to Igor are two-byte aligned.
struct MergeOpenGrawParams  {
    Handle path;
    double index;
    double result;
};
typedef struct MergeOpenGrawParams MergeOpenGrawParams;
#pragma pack()        // Reset structure alignment to default.

extern "C" int
MergeOpenGraw(MergeOpenGrawParams* p)
{
    char* fullFilePath;
    long len;
    XOP_FILE_REF fileRef;
    len = WMGetHandleSize(p->path); // Get Handle size
    WMSetHandleSize(p->path, len+1); // Add 1 for \0
    *(*p->path+len) = 0; // Add \0
    fullFilePath = *(p->path);
    int res = XOPOpenFile(fullFilePath, 0, &fileRef);
    grawfiles[(int)p->index] = fileRef;
    p->result = (double)res;
    WMSetHandleSize(p->path, len); // Remove \0
    return(res);                    /* XFunc error code */
}


#pragma pack(2)        // All structures passed to Igor are two-byte aligned.
struct MergeCloseGrawParams  {
    double index;
    double result;
};
typedef struct MergeCloseGrawParams MergeCloseGrawParams;
#pragma pack()        // Reset structure alignment to default.

extern "C" int
MergeCloseGraw(MergeCloseGrawParams* p)
{
    int res = XOPCloseFile(grawfiles[(int)p->index]);
    p->result = (double)res;
    return(res);                    /* XFunc error code */
}


#pragma pack(2)		// All structures passed to Igor are two-byte aligned.
struct MergeReadGrawParams  { // PARAMETERS ARE PASSED IN REVERSE ORDER !!!!!!
    waveHndl traces; // array containing traces on output
    waveHndl header; // array containing header on output
    waveHndl filepos; // array containing current position of files
    double fc; // current file pointer
    double refnum; // reference number of file to read from
    UserFunctionThreadInfoPtr tp;
    double result;
};
typedef struct MergeReadGrawParams MergeReadGrawParams;
#pragma pack()		// Reset structure alignment to default.

extern "C" int
MergeReadGraw(MergeReadGrawParams* p)
{
    int i=0;
    IndexInt indices[MAX_DIMENSIONS];
    UInt64 value64[2];
    UInt16 value16[2];

    // First read header data
    UInt8 h[256];
    UInt32 numBytesRead;
    XOPReadFile(grawfiles[(int)p->refnum], 256, (void*)h, &numBytesRead);
//    read((int)p->refnum, (void*)h, 256); // read header data
    UInt8 metaType = h[i]; i++;
    UInt64 data;
    UInt32 frameSize;
    frameSize = h[i+2]+(h[i+1]<<8)+(h[i]<<16); i+=3;
    UInt8 dataSource = h[i]; i++;
    UInt16 frameType = h[i+1]+(h[i]<<8); i+=2;
    UInt8 revision = h[i]; i++;
    UInt16 headerSize = h[i+1]+(h[i]<<8); i+=2;
    UInt16 itemSize = h[i+1]+(h[i]<<8); i+=2;
    UInt32 nItems = h[i+3]+(h[i+2]<<8)+(h[i+1]<<16)+(h[i]<<24); i+=4;
//    UInt16 ttop = h[i+1]+(h[i]<<8); i+=2;
//    UInt32 tbot = h[i+3]+(h[i+2]<<8)+(h[i+1]<<16)+(h[i]<<24); i+=4;
    UInt64 eventTime = h[i+5]+(UInt64(h[i+4])<<8)+(UInt64(h[i+3])<<16)+(UInt64(h[i+2])<<24)+(UInt64(h[i+1])<<32)+(UInt64(h[i])<<40); i+=6;
    UInt64 eventIdx = h[i+3]+(h[i+2]<<8)+(h[i+1]<<16)+(h[i]<<24); i+=4;
    indices[0] = 0;
    value64[0] = eventIdx;
    MDSetNumericWavePointValueUInt64(p->header, indices, value64);
    indices[0] = 1;
    value64[0] = eventTime;
    MDSetNumericWavePointValueUInt64(p->header, indices, value64);
    UInt8 coboIdx = h[i]; i++;
    UInt8 asadIdx = h[i]; i++;
    UInt16 readOffset = h[i+1]+(h[i]<<8); i+=2;
    UInt8 status = h[i]; i+=1;
    i+=9; // Skip hit pattern 0
    i+=9; // Skip hit pattern 1
    i+=9; // Skip hit pattern 2
    i+=9; // Skip hit pattern 3
    i+=2; // Skip sample multiplicity 0
    i+=2; // Skip sample multiplicity 1
    i+=2; // Skip sample multiplicity 2
    i+=2; // Skip sample multiplicity 3
    UInt32 windowOut = h[i+3]+(h[i+2]<<8)+(h[i+1]<<16)+(h[i]<<24); i+=4; // Read integrated multiplicity
    indices[0] = 2;
    value64[0] = windowOut;
    MDSetNumericWavePointValueUInt64(p->header, indices, value64);
    
    // Now read trace data
    UInt32 fs = (frameSize-headerSize)*256; // Number of bytes
    UInt8 f[fs];
    UInt16 value, bucket, channel, aget, j;
    BCInt offset;
    int numDimensions;
    CountInt dimensionSizes[MAX_DIMENSIONS+1];
    CountInt numRows, numColumns;
    MDGetWaveDimensions(p->traces, &numDimensions, dimensionSizes);
    numRows = dimensionSizes[ROWS];
    numColumns = dimensionSizes[COLUMNS];
    MDAccessNumericWaveData(p->traces, kMDWaveAccessMode0, &offset);
    UInt16 *ptr;
//    read((int)p->refnum, (void*)f, fs); // read trace data
    XOPReadFile(grawfiles[(int)p->refnum], fs, (void*)f, &numBytesRead);
    i = 0;
    if (frameType == 1) { // Partial readout mode data format
        do {
            data = f[i+3]|(f[i+2]<<8)|(f[i+1]<<16)|(f[i]<<24); i+=4; // PPC data is big endian
            value = data&0xFFF;
            bucket = (data>>14)&0x1FF;
            channel = (data>>23)&0x7F;
            aget = (data>>30)&0x3;
            ptr = (UInt16*) ((char*)(*p->traces)+offset+(aget*68+channel+bucket*numRows)*sizeof(UInt16));
            *ptr = value;
//            traces[aget*68+channel][bucket] = value
        } while (i < fs);
    }
    if (frameType == 2) {// Full readout mode data format
        bucket = 0;
        channel = 0;
        do {
            for (j=0; j<4; j++) {// loop over 4 items
                data = f[i+1]|(f[i]<<8); i+=2;
                aget = (data>>14)&0x3;
                value = data&0xfff;
                ptr = (UInt16*) ((char*)(*p->traces)+offset+(aget*68+channel+bucket*numRows)*sizeof(UInt16));
                *ptr = value;
//                traces[aget*68+channel][bucket] = value;
                data = f[i+1]|f[i]<<8; i+=2;
                value = data&0xfff;
                ptr = (UInt16*) ((char*)(*p->traces)+offset+(aget*68+channel+1+bucket*numRows)*sizeof(UInt16));
                *ptr = value;
//                traces[aget*68+channel+1][bucket] = value;
            }
            channel += 2;
            if (channel >= 68) {// next bucket
                channel = 0;
                bucket += 1;
            }
            if (bucket == 512) {
                break;
            }
        } while (i < fs);
    }

    // Finally update the file pointer array
    indices[0] = (int)p->fc;
    double valued[2];
    MDGetNumericWavePointValue(p->filepos, indices, valued);
    valued[0] += frameSize*256;
    MDSetNumericWavePointValue(p->filepos, indices, valued);
    
    p->result = frameSize*256; // return number bytes read
	
	return(0);					/* XFunc error code */
}


#pragma pack(2)        // All structures passed to Igor are two-byte aligned.
struct MergeCopyParams  {
    waveHndl kept;
    waveHndl traces;
    waveHndl stored;
    waveHndl index;
    double kk;
    UserFunctionThreadInfoPtr tp;
    double result;
};
typedef struct MergeCopyParams MergeCopyParams;
#pragma pack()        // Reset structure alignment to default.

extern "C" int
MergeCopy(MergeCopyParams* p)
{
    int i, j;
    BCInt ioffset, soffset, koffset, toffset;
    MDAccessNumericWaveData(p->index, kMDWaveAccessMode0, &ioffset);
    MDAccessNumericWaveData(p->stored, kMDWaveAccessMode0, &soffset);
    MDAccessNumericWaveData(p->traces, kMDWaveAccessMode0, &toffset);
    MDAccessNumericWaveData(p->kept, kMDWaveAccessMode0, &koffset);
    UInt16 *nPtr, *iPtr, *sPtr, *kPtr, trace;
    nPtr = (UInt16*) ((char*)(*p->index)+ioffset);
    int numDimensions;
    CountInt dimensionSizes[MAX_DIMENSIONS+1];
    CountInt knumRows, knumColumns;
    CountInt snumRows, snumColumns;
    CountInt tnumRows, tnumColumns;
    MDGetWaveDimensions(p->kept, &numDimensions, dimensionSizes);
    knumRows = dimensionSizes[ROWS];
    knumColumns = dimensionSizes[COLUMNS];
    MDGetWaveDimensions(p->stored, &numDimensions, dimensionSizes);
    snumRows = dimensionSizes[ROWS];
    snumColumns = dimensionSizes[COLUMNS];
    MDGetWaveDimensions(p->traces, &numDimensions, dimensionSizes);
    tnumRows = dimensionSizes[ROWS];
    tnumColumns = dimensionSizes[COLUMNS];
    int lkk = (int)p->kk;
    for (i=0; i<*nPtr; i++) { // First element of index contains number of traces to copy
        iPtr = (UInt16*) ((char*)(*p->index)+ioffset+(i+1)*sizeof(UInt16));
        trace = *iPtr; // Following elements contain trace indexes
        for (j=0; j<knumColumns; j++) {
            if (j < 5) // Elements 0 to 4 contain cobo, asad, aget, channel and pad information from stored
                sPtr = (UInt16*) ((char*)(*p->stored)+soffset+(trace+j*snumRows)*sizeof(UInt16));
            else // Following elements contain trace data from traces
                sPtr = (UInt16*) ((char*)(*p->traces)+toffset+(trace+(j-5)*tnumRows)*sizeof(UInt16));
            kPtr = (UInt16*) ((char*)(*p->kept)+koffset+(lkk+j*knumRows)*sizeof(UInt16));
            *kPtr = *sPtr; // Copy element to kept
        }
        lkk++;
    }
    p->result = (double)lkk;
    return(0);                    /* XFunc error code */
}


#pragma pack(2)        // All structures passed to Igor are two-byte aligned.
struct MergeStoreParams  {
    waveHndl index; // indexes of valid traces
    waveHndl read; // array of valid traces only
    waveHndl traces; // array of all traces
    UserFunctionThreadInfoPtr tp;
    double result;
};
typedef struct MergeStoreParams MergeTransferParams;
#pragma pack()        // Reset structure alignment to default.

extern "C" int
MergeStore(MergeStoreParams* p)
{
    int i, j;
    int numDimensions;
    CountInt tSizes[MAX_DIMENSIONS+1];
    MDGetWaveDimensions(p->traces, &numDimensions, tSizes);
    double value[2];
    IndexInt ind[MAX_DIMENSIONS];
    ind[0] = 0;
    BCInt source, dest;
    MDAccessNumericWaveData(p->traces, kMDWaveAccessMode0, &source);
    MDAccessNumericWaveData(p->read, kMDWaveAccessMode0, &dest);
    UInt16 *ts, *td, total=0;
    for (i=0; i<tSizes[0]; i++) { // loop on channel number
        ts = (UInt16*) ((char*)(*p->traces)+source+i*sizeof(UInt16)); // first time bucket
        if (*ts != 0xffff) { // if not 0xffff a trace was read
            for (j=0; j<tSizes[1]; j++) { // loop on time bucket
                ts = (UInt16*) ((char*)(*p->traces)+source+(i+j*tSizes[0])*sizeof(UInt16));
                td = (UInt16*) ((char*)(*p->read)+dest+(total+j*tSizes[0])*sizeof(UInt16));
                *td = *ts; // copy data
            }
            ind[0] = total+1;
            value[0] = (double)i;
            MDSetNumericWavePointValue(p->index, ind, value); // set index to channel number
            total++; // increment counter
        }
    }
    ind[0] = 0;
    value[0] = (double)total;
    MDSetNumericWavePointValue(p->index, ind, value); // set first element to number of traces
    p->result = (double)total;
    return(0);                    /* XFunc error code */
}


static XOPIORecResult
RegisterFunction()
{
	int funcIndex;

	funcIndex = (int)GetXOPItem(0);	/* which function invoked ? */
	switch (funcIndex) {
        case 0:                        /* MergeOpen */
            return (XOPIORecResult)MergeOpenGraw;
            break;
        case 1:                        /* MergeClose */
            return (XOPIORecResult)MergeCloseGraw;
            break;
        case 2:                        /* MergeReadGraw */
            return (XOPIORecResult)MergeReadGraw;
            break;
        case 3:                        /* MergeCopy */
            return (XOPIORecResult)MergeCopy;
            break;
        case 4:                        /* MergeStore */
            return (XOPIORecResult)MergeStore;
            break;
	}
	return 0;
}

/*	DoFunction()

	This will actually never be called because all of the functions use the direct method.
	It would be called if a function used the message method. See the XOP manual for
	a discussion of direct versus message XFUNCs.
*/
static int
DoFunction()
{
	int funcIndex;
	void *p;				/* pointer to structure containing function parameters and result */
	int err;

	funcIndex = (int)GetXOPItem(0);	/* which function invoked ? */
	p = (void*)GetXOPItem(1);		/* get pointer to params/result */
	switch (funcIndex) {
		case 0:						/* MergeOpenGraw */
			err = MergeOpenGraw((MergeOpenGrawParams*)p);
			break;
        case 1:                        /* MergeCloseGraw */
            err = MergeCloseGraw((MergeCloseGrawParams*)p);
            break;
        case 2:                        /* MergeReadGraw */
            err = MergeReadGraw((MergeReadGrawParams*)p);
            break;
        case 3:                        /* MergeCopy */
            err = MergeCopy((MergeCopyParams*)p);
            break;
        case 4:                        /* MergeStore */
            err = MergeStore((MergeStoreParams*)p);
            break;
	}
	return(err);
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP for all messages after the
	INIT message.
*/

extern "C" void
XOPEntry(void)
{	
	XOPIORecResult result = 0;

	switch (GetXOPMessage()) {
		case FUNCTION:								/* our external function being invoked ? */
			result = DoFunction();
			break;

		case FUNCADDRS:
			result = RegisterFunction();
			break;
	}
	SetXOPResult(result);
}

/*	XOPMain(ioRecHandle)

	This is the initial entry point at which the host application calls XOP.
	The message sent by the host must be INIT.
	
	XOPMain does any necessary initialization and then sets the XOPEntry field of the
	ioRecHandle to the address to be called for future messages.
*/

HOST_IMPORT int
XOPMain(IORecHandle ioRecHandle)
{	
	XOPInit(ioRecHandle);					// Do standard XOP initialization
	SetXOPEntry(XOPEntry);					// Set entry point for future calls
	
	if (igorVersion < 800) {				// XOP Toolkit 8.00 or later requires Igor Pro 8.00 or later
		SetXOPResult(OLD_IGOR);
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
