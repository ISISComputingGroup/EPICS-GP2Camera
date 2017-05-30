#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <exception>
#include <iostream>
#include <map>
#include <iomanip>
#include <sys/timeb.h>
#include <numeric>
#include <boost/algorithm/string.hpp>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include <macLib.h>
#include <epicsGuard.h>

#include "envDefs.h"
#include "errlog.h"

#include "utilities.h"
#include "pugixml.hpp"

#include "GP2CameraDriver.h"

#include "asynPortClient.h"

#include <epicsExport.h>

static epicsThreadOnceId onceId = EPICS_THREAD_ONCE_INIT;

static const char *driverName="GP2CameraDriver";

void GP2CameraDriver::setADAcquire(int acquire)
{
    int adstatus;
    int acquiring;
    int imageMode;
    asynStatus status = asynSuccess;

    /* Ensure that ADStatus is set correctly before we set ADAcquire.*/
    getIntegerParam(ADStatus, &adstatus);
    getIntegerParam(ADAcquire, &acquiring);
    getIntegerParam(ADImageMode, &imageMode);
      if (acquire && !acquiring) {
        setStringParam(ADStatusMessage, "Acquiring data");
        setIntegerParam(ADStatus, ADStatusAcquire); 
        setIntegerParam(ADAcquire, 1); 
      }
      if (!acquire && acquiring) {
        setIntegerParam(ADAcquire, 0); 
        setStringParam(ADStatusMessage, "Acquisition stopped");
        if (imageMode == ADImageContinuous) {
          setIntegerParam(ADStatus, ADStatusIdle);
        } else {
          setIntegerParam(ADStatus, ADStatusAborted);
        }
      }
}

#if 0

asynStatus isisdaeDriver::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
	int function = pasynUser->reason;
	if (function < FIRST_GP2CAM_PARAM)
	{
		return ADDriver::readInt32Array(pasynUser, value, nElements, nIn);
	}
    asynStatus stat = readArray(pasynUser, "readInt32Array", value, nElements, nIn);
	callParamCallbacks(); // this flushes P_ErrMsgs
	doCallbacksInt32Array(value, *nIn, function, 0);
    return stat;
}


#endif

class NSVDataClient : public asynInt16ArrayClient
{
	GP2CameraDriver* m_gp2camera;

    public:

	NSVDataClient(GP2CameraDriver* gp2camera, const char *portName, int addr, const char *drvInfo, double timeout=DEFAULT_TIMEOUT) : asynInt16ArrayClient(portName, addr, drvInfo, timeout), m_gp2camera(gp2camera)
	{
		registerInterruptUser(interruptCallback);
	}
	
    static void interruptCallback(void *userPvt, asynUser *pasynUser, epicsInt16 *value, size_t nelements)
	{
		NSVDataClient* client = (NSVDataClient*)userPvt;
        client->doGP2CameraCallback(pasynUser, value, nelements);		
    }
	
	void doGP2CameraCallback(asynUser *pasynUser, epicsInt16 *value, size_t nelements)
	{
		m_gp2camera->nsvDataInterruptCallback(pasynUser, value, nelements);
	}
};

void GP2CameraDriver::nsvDataInterruptCallback(asynUser *pasynUser, epicsInt16 *value, size_t nelements) 
{
	if (m_options & GP2NSVEpicsCallback)
	{
		setTimeStamp(&(pasynUser->timestamp));
	    doCallbacksInt16Array(value, nelements, P_nsvData, 0);
	}
	epicsInt16 *value_c = new epicsInt16[nelements];
	memcpy(value_c, value, nelements * sizeof(epicsInt16));
	DataQueueMessage message(value_c, nelements, pasynUser->timestamp);
	if (m_data_queue.trySend(&message, sizeof(DataQueueMessage)) != 0)
	{
		std::cerr << "Unable to queue message" << std::endl;
	}
}

/// Constructor for the isisdaeDriver class.
/// Calls constructor for the asynPortDriver base class.
/// \param[in] dcomint DCOM interface pointer created by lvDCOMConfigure()
/// \param[in] portName @copydoc initArg0
GP2CameraDriver::GP2CameraDriver(const char *portName, const char* nsvPortName, const char* nsvParam, int options)
   : ADDriver(portName, 
                    2, /* maxAddr */ 
                    NUM_GP2CAM_PARAMS,
					0, // maxBuffers
					0, // maxMemory
                    asynInt32Mask | asynInt16ArrayMask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask, /* Interface mask */
                    asynInt32Mask | asynInt16ArrayMask| asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,  /* Interrupt mask */
                    ASYN_CANBLOCK, /* asynFlags.  This driver can block but it is not multi-device */
					// need to think about ASYN_MULTIDEVICE for future multiple AD views
                    1, /* Autoconnect */
                    0, /* Default priority */
                    0),	/* Default stack size*/
					m_pRaw(NULL), m_options(options), m_old_acquiring(0), m_data_queue(20, sizeof(DataQueueMessage)),
					m_outfile(NULL)
{					
	int status;
    const char *functionName = "GP2CameraDriver";

	m_nsvDataClient = new NSVDataClient(this, nsvPortName, 0, nsvParam);
	createParam(P_nsvDataString, asynParamInt16Array, &P_nsvData);
	createParam(P_testFileNameString, asynParamOctet, &P_testFileName);
    setStringParam(P_testFileName, "test.out");
    // area detector defaults
//	int maxSizeX = 128, maxSizeY = 128;
//	int maxSizeX = 8, maxSizeY = 8;
	int maxSizeX = 324, maxSizeY = 324;
	NDDataType_t dataType = NDUInt8; // data type for each frame
    status =  setStringParam (ADManufacturer, "GP2");
    status |= setStringParam (ADModel, "GP2");
    status |= setIntegerParam(ADMaxSizeX, maxSizeX);
    status |= setIntegerParam(ADMaxSizeY, maxSizeY);
    status |= setIntegerParam(ADMinX, 0);
    status |= setIntegerParam(ADMinY, 0);
    status |= setIntegerParam(ADBinX, 1);
    status |= setIntegerParam(ADBinY, 1);
    status |= setIntegerParam(ADReverseX, 0);
    status |= setIntegerParam(ADReverseY, 0);
    status |= setIntegerParam(ADSizeX, maxSizeX);
    status |= setIntegerParam(ADSizeY, maxSizeY);
    status |= setIntegerParam(NDArraySizeX, maxSizeX);
    status |= setIntegerParam(NDArraySizeY, maxSizeY);
    status |= setIntegerParam(NDArraySize, 0);
    status |= setIntegerParam(NDDataType, dataType);
    status |= setIntegerParam(ADImageMode, ADImageContinuous);
    status |= setDoubleParam (ADAcquireTime, .001);
    status |= setDoubleParam (ADAcquirePeriod, .005);
    status |= setIntegerParam(ADNumImages, 100);

    if (status) {
        printf("%s: unable to set DAE parameters\n", functionName);
        return;
    }

    // Create the thread for background tasks (not used at present, could be used for I/O intr scanning) 
    if (epicsThreadCreate("GP2CameraPoller1",
                          epicsThreadPriorityMedium,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          (EPICSTHREADFUNC)pollerThreadC1, this) == 0)
    {
        printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
        return;
    }
}

void GP2CameraDriver::pollerThreadC1(void* arg)
{
	epicsThreadSleep(1.0);	// let constructor complete
    GP2CameraDriver* driver = (GP2CameraDriver*)arg;
	if (driver != NULL)
	{
	    driver->pollerThread1();
	}
}

void GP2CameraDriver::pollerThread1()
{
    static const char* functionName = "GP2CameraDriverPoller1";
	DataQueueMessage message;
	int acquiring;
	while(true)
	{
	    if ( m_data_queue.receive(&message, sizeof(DataQueueMessage), 1.0) == sizeof(DataQueueMessage) )
		{
			processCameraData(message.value, message.nelements, &(message.ts));
			delete []message.value;
		}
		else
		{
			lock();
			getIntegerParam(ADAcquire, &acquiring);
			if (acquiring == 0 && m_outfile != NULL)
			{
				fclose(m_outfile);
				m_outfile = NULL;
			}
			unlock();
		}			
	}
}

asynStatus GP2CameraDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	int function = pasynUser->reason;
	if (function < FIRST_GP2CAM_PARAM)
	{
		return ADDriver::writeInt32(pasynUser, value);
	}
	return asynSuccess;	
}

void GP2CameraDriver::processCameraData(epicsInt16 *value, size_t nelements, epicsTimeStamp* epicsTS)
{
    static const char* functionName = "processCameraData";
	int acquiring = 0;
    int status = asynSuccess;
    int imageCounter;
    int numImages, numImagesCounter;
    int imageMode;
    int arrayCallbacks;
    NDArray *pImage, *pEventData;
    double acquireTime, acquirePeriod, delay;
    epicsTimeStamp startTime, endTime;
    double elapsedTime;
	char filename[256];

		lock();
		getIntegerParam(ADAcquire, &acquiring);
        getDoubleParam(ADAcquirePeriod, &acquirePeriod);
		getStringParam(P_testFileName, sizeof(filename), filename);
		if (acquiring == 0)
		{
			m_old_acquiring = acquiring;
			if (m_outfile != NULL)
			{
				fclose(m_outfile);
				m_outfile = NULL;
			}
			unlock();
//			epicsThreadSleep( acquirePeriod + (enable == 0 ? 1.0 : 0.0) );
			return;
		}
		if (m_old_acquiring == 0)
		{
            setIntegerParam(ADNumImagesCounter, 0);
			m_old_acquiring = acquiring;
			m_outfile = fopen(filename, "wb");
        }
        setIntegerParam(ADStatus, ADStatusAcquire); 
		epicsTimeGetCurrent(&startTime);
        getIntegerParam(ADImageMode, &imageMode);

        /* Get the exposure parameters */
        getDoubleParam(ADAcquireTime, &acquireTime);  // not really used

        setShutter(ADShutterOpen);
        callParamCallbacks();
            
        /* Update the image */
        status = computeImage(value, nelements);
//        if (status) continue;

		// could sleep to make up to acquireTime
		
        /* Close the shutter */
        setShutter(ADShutterClosed);
        
        setIntegerParam(ADStatus, ADStatusReadout);
        /* Call the callbacks to update any changes */
        callParamCallbacks();

        pImage = this->pArrays[0];
	    pEventData = this->pArrays[1];
		// setTimeStamp(epicsTS);   ??????
		
        /* Get the current parameters */
        getIntegerParam(NDArrayCounter, &imageCounter);
        getIntegerParam(ADNumImages, &numImages);
        getIntegerParam(ADNumImagesCounter, &numImagesCounter);
        getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
        imageCounter++;
        numImagesCounter++;
        setIntegerParam(NDArrayCounter, imageCounter);
        setIntegerParam(ADNumImagesCounter, numImagesCounter);

        /* Put the frame number and time stamp into the buffer */
        pImage->uniqueId = imageCounter;
		pImage->epicsTS = *epicsTS;
        pImage->timeStamp = epicsTS->secPastEpoch + epicsTS->nsec / 1.e9;
        pEventData->uniqueId = imageCounter;
		pEventData->epicsTS = *epicsTS;
        pEventData->timeStamp = epicsTS->secPastEpoch + epicsTS->nsec / 1.e9;
        //getTimeStamp(&pImage->epicsTS);

        /* Get any attributes that have been defined for this driver */
        this->getAttributes(pImage->pAttributeList);

		if (m_outfile != NULL)
		{
			fwrite(epicsTS, sizeof(epicsTimeStamp), 1, m_outfile);
			fwrite(&imageCounter, sizeof(int), 1, m_outfile);
			fwrite(&numImagesCounter, sizeof(int), 1, m_outfile);
			fwrite(&nelements, sizeof(size_t), 1, m_outfile);
			fwrite(value, sizeof(epicsInt16), nelements, m_outfile);
			if (numImagesCounter % 10 == 0)
			{
				fflush(m_outfile);
			}
		}

        if (arrayCallbacks) {
          /* Call the NDArray callback */
          /* Must release the lock here, or we can get into a deadlock, because we can
           * block on the plugin lock, and the plugin can be calling us */
          this->unlock();
          asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: calling imageData callback\n", driverName, functionName);
          doCallbacksGenericPointer(pImage, NDArrayData, 0);
          doCallbacksGenericPointer(pEventData, NDArrayData, 1);
          this->lock();
        }

        /* Call the callbacks to update any changes */
        callParamCallbacks();
        /* sleep for the acquire period minus elapsed time. */
        epicsTimeGetCurrent(&endTime);
        elapsedTime = epicsTimeDiffInSeconds(&endTime, &startTime);
        delay = acquirePeriod - elapsedTime;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: delay=%f\n",
                    driverName, functionName, delay);
//        if (delay >= 0.0) {
 //           /* We set the status to waiting to indicate we are in the period delay */
//            setIntegerParam(ADStatus, ADStatusWaiting);
//            callParamCallbacks();
//            this->unlock();
//            epicsThreadSleep(delay);
//            this->lock();
//            setIntegerParam(ADStatus, ADStatusIdle);
//            callParamCallbacks();  
//        }
        this->unlock();
}

/** Computes the new image data */
int GP2CameraDriver::computeImage(epicsInt16 *value, size_t nelements)
{
    int status = asynSuccess;
    NDDataType_t dataType;
    int itemp;
    int binX, binY, minX, minY, sizeX, sizeY, reverseX, reverseY;
    int xDim=0, yDim=1, colorDim=-1;
    int maxSizeX, maxSizeY;
    int colorMode;
    int ndims=0;
    NDDimension_t dimsOut[3];
    size_t dims[3], dims2[2];
    NDArrayInfo_t arrayInfo;
    NDArray *pImage;
    const char* functionName = "computeImage";

    /* NOTE: The caller of this function must have taken the mutex */

    status |= getIntegerParam(ADBinX,         &binX);
    status |= getIntegerParam(ADBinY,         &binY);
    status |= getIntegerParam(ADMinX,         &minX);
    status |= getIntegerParam(ADMinY,         &minY);
    status |= getIntegerParam(ADSizeX,        &sizeX);
    status |= getIntegerParam(ADSizeY,        &sizeY);
    status |= getIntegerParam(ADReverseX,     &reverseX);
    status |= getIntegerParam(ADReverseY,     &reverseY);
    status |= getIntegerParam(ADMaxSizeX,     &maxSizeX);
    status |= getIntegerParam(ADMaxSizeY,     &maxSizeY);
    status |= getIntegerParam(NDColorMode,    &colorMode);
    status |= getIntegerParam(NDDataType,     &itemp); 
	dataType = (NDDataType_t)itemp;
    if (status) asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: error getting parameters\n",
                    driverName, functionName);

    /* Make sure parameters are consistent, fix them if they are not */
    if (binX < 1) {
        binX = 1;
        status |= setIntegerParam(ADBinX, binX);
    }
    if (binY < 1) {
        binY = 1;
        status |= setIntegerParam(ADBinY, binY);
    }
    if (minX < 0) {
        minX = 0;
        status |= setIntegerParam(ADMinX, minX);
    }
    if (minY < 0) {
        minY = 0;
        status |= setIntegerParam(ADMinY, minY);
    }
    if (minX > maxSizeX-1) {
        minX = maxSizeX-1;
        status |= setIntegerParam(ADMinX, minX);
    }
    if (minY > maxSizeY-1) {
        minY = maxSizeY-1;
        status |= setIntegerParam(ADMinY, minY);
    }
    if (minX+sizeX > maxSizeX) {
        sizeX = maxSizeX-minX;
        status |= setIntegerParam(ADSizeX, sizeX);
    }
    if (minY+sizeY > maxSizeY) {
        sizeY = maxSizeY-minY;
        status |= setIntegerParam(ADSizeY, sizeY);
    }

    switch (colorMode) {
        case NDColorModeMono:
            ndims = 2;
            xDim = 0;
            yDim = 1;
            break;
        case NDColorModeRGB1:
            ndims = 3;
            colorDim = 0;
            xDim     = 1;
            yDim     = 2;
            break;
        case NDColorModeRGB2:
            ndims = 3;
            colorDim = 1;
            xDim     = 0;
            yDim     = 2;
            break;
        case NDColorModeRGB3:
            ndims = 3;
            colorDim = 2;
            xDim     = 0;
            yDim     = 1;
            break;
    }

// we could be more efficient
//    if (resetImage) {
    /* Free the previous raw buffer */
        if (m_pRaw) m_pRaw->release();
        /* Allocate the raw buffer we use to compute images. */
        dims[xDim] = maxSizeX;
        dims[yDim] = maxSizeY;
        if (ndims > 2) dims[colorDim] = 3;
        m_pRaw = this->pNDArrayPool->alloc(ndims, dims, dataType, 0, NULL);

        if (!m_pRaw) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s:%s: error allocating raw buffer\n",
                      driverName, functionName);
            return(status);
        }
//    }

    switch (dataType) {
        case NDInt8:
            status |= computeArray<epicsInt8>(value, nelements, maxSizeX, maxSizeY);
            break;
        case NDUInt8:
            status |= computeArray<epicsUInt8>(value, nelements, maxSizeX, maxSizeY);
            break;
        case NDInt16:
            status |= computeArray<epicsInt16>(value, nelements, maxSizeX, maxSizeY);
            break;
        case NDUInt16:
            status |= computeArray<epicsUInt16>(value, nelements, maxSizeX, maxSizeY);
            break;
        case NDInt32:
            status |= computeArray<epicsInt32>(value, nelements, maxSizeX, maxSizeY);
            break;
        case NDUInt32:
            status |= computeArray<epicsUInt32>(value, nelements, maxSizeX, maxSizeY);
            break;
        case NDFloat32:
            status |= computeArray<epicsFloat32>(value, nelements, maxSizeX, maxSizeY);
            break;
        case NDFloat64:
            status |= computeArray<epicsFloat64>(value, nelements, maxSizeX, maxSizeY);
            break;
    }

    /* Extract the region of interest with binning.
     * If the entire image is being used (no ROI or binning) that's OK because
     * convertImage detects that case and is very efficient */
    m_pRaw->initDimension(&dimsOut[xDim], sizeX);
    m_pRaw->initDimension(&dimsOut[yDim], sizeY);
    if (ndims > 2) m_pRaw->initDimension(&dimsOut[colorDim], 3);
    dimsOut[xDim].binning = binX;
    dimsOut[xDim].offset  = minX;
    dimsOut[xDim].reverse = reverseX;
    dimsOut[yDim].binning = binY;
    dimsOut[yDim].offset  = minY;
    dimsOut[yDim].reverse = reverseY;
    /* We save the most recent image buffer so it can be used in the read() function.
     * Now release it before getting a new version. */
    if (this->pArrays[0]) this->pArrays[0]->release();
    status = this->pNDArrayPool->convert(m_pRaw,
                                         &this->pArrays[0],
                                         dataType,
                                         dimsOut);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: error allocating buffer in convert()\n",
                    driverName, functionName);
        return(status);
    }
    pImage = this->pArrays[0];
    pImage->getInfo(&arrayInfo);
    status = asynSuccess;
    status |= setIntegerParam(NDArraySize,  (int)arrayInfo.totalBytes);
    status |= setIntegerParam(NDArraySizeX, (int)pImage->dims[xDim].size);
    status |= setIntegerParam(NDArraySizeY, (int)pImage->dims[yDim].size);
    if (status) asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: error setting parameters\n",
                    driverName, functionName);
    if (this->pArrays[1]) this->pArrays[1]->release();
	dims2[0] = 3;
	dims2[1] = nelements / 3;
    this->pArrays[1] = this->pNDArrayPool->alloc(2, dims2, NDUInt16, 0, NULL);
	memcpy(this->pArrays[1]->pData, value, nelements * sizeof(epicsInt16));
    return(status);
}

// supplied array of x,y,t
template <typename epicsType> 
int GP2CameraDriver::computeArray(epicsInt16* value, size_t nelements, int sizeX, int sizeY)
{
    epicsType *pMono=NULL, *pRed=NULL, *pGreen=NULL, *pBlue=NULL;
    int columnStep=0, rowStep=0, colorMode;
    int status = asynSuccess;
    double exposureTime, gain;
    int i, k;
	
    status = getDoubleParam (ADGain,        &gain);
    status = getIntegerParam(NDColorMode,   &colorMode);
    status = getDoubleParam (ADAcquireTime, &exposureTime);

    switch (colorMode) {
        case NDColorModeMono:
            pMono = (epicsType *)m_pRaw->pData;
            break;
        case NDColorModeRGB1:
            columnStep = 3;
            rowStep = 0;
            pRed   = (epicsType *)m_pRaw->pData;
            pGreen = (epicsType *)m_pRaw->pData+1;
            pBlue  = (epicsType *)m_pRaw->pData+2;
            break;
        case NDColorModeRGB2:
            columnStep = 1;
            rowStep = 2 * sizeX;
            pRed   = (epicsType *)m_pRaw->pData;
            pGreen = (epicsType *)m_pRaw->pData + sizeX;
            pBlue  = (epicsType *)m_pRaw->pData + 2*sizeX;
            break;
        case NDColorModeRGB3:
            columnStep = 1;
            rowStep = 0;
            pRed   = (epicsType *)m_pRaw->pData;
            pGreen = (epicsType *)m_pRaw->pData + sizeX*sizeY;
            pBlue  = (epicsType *)m_pRaw->pData + 2*sizeX*sizeY;
            break;
    }
    m_pRaw->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
    int xi, yi;
	memset(m_pRaw->pData, 0, m_pRaw->dataSize);
	for(i=0; i< nelements; i += 3)
	{
		xi = value[i];
		yi = value[i+1];
		if (xi < sizeX && yi < sizeY)
		{
			switch (colorMode) {
			case NDColorModeMono:
				++(pMono[yi * sizeX + xi]);
				break;
			case NDColorModeRGB1:
			case NDColorModeRGB2:
			case NDColorModeRGB3:
				k = columnStep * (yi * sizeX + xi) + yi * rowStep;
				++(pRed[k]);
				++(pGreen[k]);
				++(pBlue[k]);
				break;
			}
		}
	}
    return(status);
}


/** Controls the shutter */
void GP2CameraDriver::setShutter(int open)
{
    int shutterMode;

    getIntegerParam(ADShutterMode, &shutterMode);
    if (shutterMode == ADShutterModeDetector) {
        /* Simulate a shutter by just changing the status readback */
        setIntegerParam(ADShutterStatus, open);
    } else {
        /* For no shutter or EPICS shutter call the base class method */
        ADDriver::setShutter(open);
    }
}


/** Report status of the driver.
  * Prints details about the driver if details>0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >0 then driver details are printed.
  */
void GP2CameraDriver::report(FILE *fp, int details)
{
    fprintf(fp, "GP2 Camera driver %s\n", this->portName);
    if (details > 0) {
        int nx, ny, dataType;
        getIntegerParam(ADSizeX, &nx);
        getIntegerParam(ADSizeY, &ny);
        getIntegerParam(NDDataType, &dataType);
        fprintf(fp, "  NX, NY:            %d  %d\n", nx, ny);
        fprintf(fp, "  Data type:         %d\n", dataType);
    }
    /* Invoke the base class method */
    ADDriver::report(fp, details);
}
		
extern "C" {

/// EPICS iocsh callable function to call constructor of lvDCOMInterface().
/// \param[in] portName @copydoc initArg0
/// \param[in] configSection @copydoc initArg1
/// \param[in] configFile @copydoc initArg2
/// \param[in] host @copydoc initArg3
/// \param[in] options @copydoc initArg4
/// \param[in] progid @copydoc initArg5
/// \param[in] username @copydoc initArg6
/// \param[in] password @copydoc initArg7
int GP2CameraConfigure(const char *portName, const char *nsvPortName, const char* nsvParam, int options)
{
	try
	{
		GP2CameraDriver* driver = new GP2CameraDriver(portName, nsvPortName, nsvParam, options);
		if (driver == NULL)
		{
		    errlogSevPrintf(errlogMajor, "GP2CameraConfigure failed (NULL)\n");
			return(asynError);
		}
		return(asynSuccess);
	}
	catch(const std::exception& ex)
	{
		errlogSevPrintf(errlogMajor, "GP2CameraConfigure failed: %s\n", ex.what());
		return(asynError);
	}
}

// EPICS iocsh shell commands 

static const iocshArg initArg0 = { "portName", iocshArgString};			///< The name of the asyn driver port we will create
static const iocshArg initArg1 = { "nsvPortName", iocshArgString};			    ///< options as per #lvDCOMOptions enum

static const iocshArg initArg2 = { "nsvParam", iocshArgString};			    ///< options as per #lvDCOMOptions enum

static const iocshArg initArg3 = { "options", iocshArgInt};			    ///< options as per #lvDCOMOptions enum

static const iocshArg * const initArgs[] = { &initArg0, &initArg1, &initArg2, &initArg3 };

static const iocshFuncDef initFuncDef = {"GP2CameraConfigure", sizeof(initArgs) / sizeof(iocshArg*), initArgs};

static void initCallFunc(const iocshArgBuf *args)
{
    GP2CameraConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].ival);
}

static void GP2CameraRegister(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
}

epicsExportRegistrar(GP2CameraRegister);

}

