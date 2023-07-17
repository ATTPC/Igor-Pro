/*	ATTPCUtil.c -- Utilities for AT-TPC analysis

*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "ATTPCUtil.h"

#pragma pack(2)		// All structures passed to Igor are two-byte aligned.

struct dmatrixParams {
    waveHndl matrix; // array containing the DMatrix
    waveHndl triplets; // array containing the triplets
    double scale; // distance scale factor in metrics
    double ret; // return value
};

extern "C" int
calculateDmatrix(dmatrixParams *p)
{
    int numDim;
    CountInt dimMatrix[MAX_DIMENSIONS+1], dimTriplets[MAX_DIMENSIONS+1];
    MDGetWaveDimensions(p->triplets,&numDim,dimTriplets);
    MDGetWaveDimensions(p->matrix,&numDim,dimMatrix);
    BCInt tripletsOffset, matrixOffset;
    float *tPtr, *mPtr; // It is assumed the waves contain 32 bit floats
    MDAccessNumericWaveData(p->triplets, kMDWaveAccessMode0, &tripletsOffset);
    MDAccessNumericWaveData(p->matrix, kMDWaveAccessMode0, &matrixOffset);
    tPtr = (float*)((char*)(*p->triplets)+tripletsOffset);
    mPtr = (float*)((char*)(*p->matrix)+matrixOffset);
    int i,j;
    double d1,d2,phi,xx,yy,zz,inner,metrics;
    for (i=0;i<dimTriplets[0];i++) {
        for (j=i+1;j<dimTriplets[0];j++){
            // calculate d1 (formula 7)
            inner = (*(tPtr+i+4*dimTriplets[0]) - *(tPtr+j+4*dimTriplets[0]))* *(tPtr+j+7*dimTriplets[0]);
            inner += (*(tPtr+i+5*dimTriplets[0]) - *(tPtr+j+5*dimTriplets[0]))* *(tPtr+j+8*dimTriplets[0]);
            inner += (*(tPtr+i+6*dimTriplets[0]) - *(tPtr+j+6*dimTriplets[0]))* *(tPtr+j+9*dimTriplets[0]);
            xx = *(tPtr+j+4*dimTriplets[0]) - *(tPtr+i+4*dimTriplets[0]) + inner * *(tPtr+j+7*dimTriplets[0]);
            yy = *(tPtr+j+5*dimTriplets[0]) - *(tPtr+i+5*dimTriplets[0]) + inner * *(tPtr+j+8*dimTriplets[0]);
            zz = *(tPtr+j+6*dimTriplets[0]) - *(tPtr+i+6*dimTriplets[0]) + inner * *(tPtr+j+9*dimTriplets[0]);
            d1 = sqrt(xx*xx + yy*yy + zz*zz);
            // calculate d2 (formula 8)
            inner = (*(tPtr+j+4*dimTriplets[0]) - *(tPtr+i+4*dimTriplets[0]))* *(tPtr+i+7*dimTriplets[0]);
            inner += (*(tPtr+j+5*dimTriplets[0]) - *(tPtr+i+5*dimTriplets[0]))* *(tPtr+i+8*dimTriplets[0]);
            inner += (*(tPtr+j+6*dimTriplets[0]) - *(tPtr+i+6*dimTriplets[0]))* *(tPtr+i+9*dimTriplets[0]);
            xx = *(tPtr+i+4*dimTriplets[0]) - *(tPtr+j+4*dimTriplets[0]) + inner * *(tPtr+i+7*dimTriplets[0]);
            yy = *(tPtr+i+5*dimTriplets[0]) - *(tPtr+j+5*dimTriplets[0]) + inner * *(tPtr+i+8*dimTriplets[0]);
            zz = *(tPtr+i+6*dimTriplets[0]) - *(tPtr+j+6*dimTriplets[0]) + inner * *(tPtr+i+9*dimTriplets[0]);
            d2 = sqrt(xx*xx + yy*yy + zz*zz);
            // calculate phi (formula 9)
            phi = *(tPtr+i+7*dimTriplets[0]) * *(tPtr+j+7*dimTriplets[0]);
            phi += *(tPtr+i+8*dimTriplets[0]) * *(tPtr+j+8*dimTriplets[0]);
            phi += *(tPtr+i+9*dimTriplets[0]) * *(tPtr+j+9*dimTriplets[0]);
            if (phi > 1) phi = 1;
            phi = acos(fabs(phi));
            // formula 10
            if (d1 > d2) metrics = d1/p->scale + fabs(tan(phi));
            else metrics = d2/p->scale + fabs(tan(phi));
            if (isnan(metrics)) metrics = 0;
            *(mPtr+j+i*dimMatrix[0]) = metrics;
            *(mPtr+i+j*dimMatrix[0]) = metrics;
       }
    }
    return 0;
};

//Variable d1, d2, phi, xx, yy, zz, inner, im=0, ret
// calculate d1 (formula 7)
//inner = (tr[i][4]-tr[j][4])*tr[j][7]
//inner += (tr[i][5]-tr[j][5])*tr[j][8]
//inner += (tr[i][6]-tr[j][6])*tr[j][9]
//xx = tr[j][4] - tr[i][4] + inner * tr[j][7]
//yy = tr[j][5] - tr[i][5] + inner * tr[j][8]
//zz = tr[j][6] - tr[i][6] + inner * tr[j][9]
//d1 = sqrt(xx^2 + yy^2 + zz^2)
// calculate d2 (formula 8)
//inner = (tr[j][4]-tr[i][4])*tr[i][7]
//inner += (tr[j][5]-tr[i][5])*tr[i][8]
//inner += (tr[j][6]-tr[i][6])*tr[i][9]
//xx = tr[i][4] - tr[j][4] + inner * tr[i][7]
//yy = tr[i][5] - tr[j][5] + inner * tr[i][8]
//zz = tr[i][6] - tr[j][6] + inner * tr[i][9]
//d2 = sqrt(xx^2 + yy^2 + zz^2)
// calculate phi (formula 9)
//phi = tr[i][7]*tr[j][7] + tr[i][8]*tr[j][8] + tr[i][9]*tr[j][9]
//if (phi > 1)
//    phi = 1
//endif
//phi = acos(abs(phi))
//ret = max(d1,d2)/sc + abs(tan(phi)) // formula 10


//*** Function to calculate neighborhood smoothing ***//
struct neighborSmoothParams {
    waveHndl smoothed; // array containing smoothed cloud
    double threshold; // lower limit for performing smoothing (%)
    double maxdistance; // radius of bubble used to smooth
    waveHndl plook; // matrix containing LUT of distances
    waveHndl cloud; // input cloud
    double size; // returned number of points in smoothed cloud
};

extern "C" int
neighborSmooth(neighborSmoothParams *p)
{
    int numDim;
    CountInt dimLUT[MAX_DIMENSIONS+1], dimCloud[MAX_DIMENSIONS+1];
    MDGetWaveDimensions(p->plook,&numDim,dimLUT);
    MDGetWaveDimensions(p->cloud,&numDim,dimCloud);
    BCInt cloudOffset, LUTOffset, smoothOffset;
    float *cloudPtr, *LUTPtr, *smoothPtr; // It is assumed the waves contain 32 bit floats
    MDAccessNumericWaveData(p->cloud, kMDWaveAccessMode0, &cloudOffset);
    MDAccessNumericWaveData(p->plook, kMDWaveAccessMode0, &LUTOffset);
    MDAccessNumericWaveData(p->smoothed, kMDWaveAccessMode0, &smoothOffset);
    cloudPtr = (float*)((char*)(*p->cloud)+cloudOffset);
    LUTPtr = (float*)((char*)(*p->plook)+LUTOffset);
    smoothPtr = (float*)((char*)(*p->smoothed)+smoothOffset);
    int i,j,k,s=0;
    double xs,ys,zs,cs,is,charge=0, limit;
    for (i=0;i<dimCloud[0];i++) charge += *(cloudPtr+4*dimCloud[0]+i);
    limit = p->threshold*charge/dimCloud[0]/100; // convert % threshold to charge units
    for (i=0;i<dimCloud[0];i++) {
        charge = *(cloudPtr+4*dimCloud[0]+i);
        if (charge > limit) { // if above threshold
            k=0; xs=0; ys=0; zs=0; cs=0; is=0;
            for (j=0;j<dimCloud[0];j++) {
                if (*(LUTPtr+i*dimCloud[0]+j) < p->maxdistance) {
                    charge = *(cloudPtr+4*dimCloud[0]+j);
                    xs += *(cloudPtr+j) * charge;
                    ys += *(cloudPtr+1*dimCloud[0]+j) * charge;
                    zs += *(cloudPtr+2*dimCloud[0]+j) * charge;
                    cs += *(cloudPtr+3*dimCloud[0]+j);
                    is += charge;
                    k++;
                }
            }
            *(smoothPtr+0*dimCloud[0]+s) = xs/is;
            *(smoothPtr+1*dimCloud[0]+s) = ys/is;
            *(smoothPtr+2*dimCloud[0]+s) = zs/is;
            *(smoothPtr+3*dimCloud[0]+s) = cs/k;
            *(smoothPtr+4*dimCloud[0]+s) = is/k;
            s++;
        }
    }
    p->size = s;
    return 0;
};

//*** Function to calculate Lookup table of distances between points in cloud ***//
struct lutDistancesParams {
    waveHndl plook; // table containing distances
    waveHndl cloud; // input cloud
    double ret;
};

typedef struct lutDistancesParams lutDistancesParams;

extern "C" int
lutDistances(lutDistancesParams *p)
{
    int numDim;
    CountInt dimLUT[MAX_DIMENSIONS+1], dimCloud[MAX_DIMENSIONS+1];
    MDGetWaveDimensions(p->plook,&numDim,dimLUT);
    MDGetWaveDimensions(p->cloud,&numDim,dimCloud);
    BCInt cloudOffset, LUTOffset;
    float *cloudPtr, *LUTPtr; // It is assumed the waves contain 32 bit floats
    MDAccessNumericWaveData(p->cloud, kMDWaveAccessMode0, &cloudOffset);
    MDAccessNumericWaveData(p->plook, kMDWaveAccessMode0, &LUTOffset);
    cloudPtr = (float*)((char*)(*p->cloud)+cloudOffset);
    LUTPtr = (float*)((char*)(*p->plook)+LUTOffset);
    int i,j;
    double xd, yd, zd, distance;
    for (i=0;i<dimCloud[0];i++) {
        for (j=i;j<dimCloud[0];j++) {
            xd = *(cloudPtr+i) - *(cloudPtr+j);
            yd = *(cloudPtr+i+1*dimCloud[0]) - *(cloudPtr+j+1*dimCloud[0]);
            zd = *(cloudPtr+i+2*dimCloud[0]) - *(cloudPtr+j+2*dimCloud[0]);
            distance = sqrt(xd*xd+yd*yd+zd*zd);
            *(LUTPtr+i+j*dimLUT[0]) = distance;
            *(LUTPtr+j+i*dimLUT[1]) = distance;
        }
    }
    return(0);
};

//*** Function to calculate weighted average of distance between cloud and vec ***//
struct closestRangeParams {
    waveHndl weight; // weight applied to each iteration
    waveHndl vec; // simulated point cloud
    waveHndl cloud; // data point cloud
    double dmin; // weighted average of distance between clouds
};

typedef struct closestRangeParams closestRangeParams;

extern "C" int
closestRange(closestRangeParams *p)
{
    int numDim;
    CountInt dimVec[MAX_DIMENSIONS+1], dimCloud[MAX_DIMENSIONS+1];
    MDGetWaveDimensions(p->vec,&numDim,dimVec);
    MDGetWaveDimensions(p->cloud,&numDim,dimCloud);
    // Prepare pointers for direct memory access
    BCInt cloudOffset, vecOffset, weightOffset;
    float *cloudPtr, *vecPtr, *weightPtr; // It is assumed the waves contain 32 bit floats
    MDAccessNumericWaveData(p->cloud, kMDWaveAccessMode0, &cloudOffset);
    MDAccessNumericWaveData(p->vec, kMDWaveAccessMode0, &vecOffset);
    MDAccessNumericWaveData(p->weight, kMDWaveAccessMode0, &weightOffset);
    cloudPtr = (float*)((char*)(*p->cloud)+cloudOffset);
    vecPtr = (float*)((char*)(*p->vec)+vecOffset);
    weightPtr = (float*)((char*)(*p->weight)+weightOffset);
    int i,j;
    // Loop on data points and culumate minimum distances
    double result=0, distance, dmin, xdist, ydist, zdist;
    for (i=0;i<dimCloud[0];i++) {
        dmin = 1e6;
        for (j=0;j<dimVec[0];j++) {
            xdist = *(cloudPtr+i) - *(vecPtr+j+3*dimVec[0]); // direct access x
            ydist = *(cloudPtr+i+1*dimCloud[0]) - *(vecPtr+j+4*dimVec[0]); // direct access y
            zdist = *(cloudPtr+i+2*dimCloud[0]) - *(vecPtr+j+5*dimVec[0]); // direct access z
            distance = sqrt(xdist*xdist+ydist*ydist+zdist*zdist);
            if (distance < dmin) dmin = distance;
        }
        result += dmin * *(weightPtr+i);
    }
    p->dmin = result/dimCloud[0];
    return(0);
};


static XOPIORecResult
RegisterFunction()
{
	int funcIndex;

	funcIndex = (int)GetXOPItem(0);			/* which function invoked ? */
	switch (funcIndex) {
        case 0:
            return (XOPIORecResult)closestRange; // Direct call method
            break;
        case 1:
            return (XOPIORecResult)lutDistances; // Direct call method
            break;
        case 2:
            return (XOPIORecResult)neighborSmooth; // Direct call method
            break;
        case 3:
            return (XOPIORecResult)calculateDmatrix; // Direct call method
            break;
	}
	return 0;
}

static int
DoFunction()
{
	int funcIndex;
	void *p;				/* pointer to structure containing function parameters and result */
	int err;				/* error code returned by function */

	funcIndex = (int)GetXOPItem(0);	/* which function invoked ? */
	p = (void*)GetXOPItem(1);		/* get pointer to params/result */
	switch (funcIndex) {
		default:
			err = UNKNOWN_XFUNC;
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
	XOPInit(ioRecHandle);				// Do standard XOP initialization
	SetXOPEntry(XOPEntry);				// Set entry point for future calls

	if (igorVersion < 800) {			// XOP Toolkit 8.00 or later requires Igor Pro 8.00 or later
		SetXOPResult(OLD_IGOR);
		return EXIT_FAILURE;
	}
	
	SetXOPResult(0);
	return EXIT_SUCCESS;
}
