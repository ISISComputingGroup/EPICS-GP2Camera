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
#include "ADCore/ADApp/pluginSrc/colorMaps.h"

#include "asynPortClient.h"

#include <epicsExport.h>

static epicsThreadOnceId onceId = EPICS_THREAD_ONCE_INIT;

static const char *driverName="GP2CameraDriver";

#if 0

void isisdaeDriver::setADAcquire(int acquire)
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

template<typename T>
asynStatus isisdaeDriver::writeValue(asynUser *pasynUser, const char* functionName, T value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *paramName = NULL;
	getParamName(function, &paramName);
	m_iface->resetMessages();
	try
	{
		if (function == P_BeginRun)
		{
		    beginStateTransition(RS_BEGINNING);
            zeroRunCounters();
			m_iface->beginRun();
			setADAcquire(1);
		}
        else if (function == P_BeginRunEx)
		{
		    beginStateTransition(RS_BEGINNING);
            zeroRunCounters();
			m_iface->beginRunEx(static_cast<long>(value), -1);
			setADAcquire(1);
		}
		else if (function == P_AbortRun)
		{
		    beginStateTransition(RS_ABORTING);
			m_iface->abortRun();
			setADAcquire(0);
		}
        else if (function == P_PauseRun)
		{
		    beginStateTransition(RS_PAUSING);
			m_iface->pauseRun();
			setADAcquire(0);
		}
        else if (function == P_ResumeRun)
		{
		    beginStateTransition(RS_RESUMING);
			m_iface->resumeRun();
			setADAcquire(1);
		}
        else if (function == P_EndRun)
		{
		    beginStateTransition(RS_ENDING);
			m_iface->endRun();
			setADAcquire(0);
		}
        else if (function == P_RecoverRun)
		{
			m_iface->recoverRun();
		}
        else if (function == P_SaveRun)
		{
		    beginStateTransition(RS_SAVING);
			m_iface->saveRun();
		}
        else if (function == P_UpdateRun)
		{
		    beginStateTransition(RS_UPDATING);
			m_iface->updateRun();
		}
        else if (function == P_StoreRun)
		{
		    beginStateTransition(RS_STORING);
			m_iface->storeRun();
		}
        else if (function == P_StartSEWait)
		{
			m_iface->startSEWait();
			setADAcquire(0);
		}
        else if (function == P_EndSEWait)
		{
			m_iface->endSEWait();
			setADAcquire(1);
		}
        else if (function == P_Period)
		{
			m_iface->setPeriod(static_cast<long>(value));
		}
        else if (function == P_NumPeriods)
		{
			m_iface->setNumPeriods(static_cast<long>(value));
		}
		endStateTransition();
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=%s\n", 
              driverName, functionName, function, paramName, convertToString(value).c_str());
		reportMessages();
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=%s, error=%s", 
                  driverName, functionName, status, function, paramName, convertToString(value).c_str(), ex.what());
		reportErrors(ex.what());
		endStateTransition();
		return asynError;
	}
}

template<typename T>
asynStatus isisdaeDriver::readValue(asynUser *pasynUser, const char* functionName, T* value)
{
	int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *paramName = NULL;
	getParamName(function, &paramName);
	m_iface->resetMessages();
	try
	{
		status = ADDriver::readValue(pasynUser, functionName, &value);
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=%s\n", 
              driverName, functionName, function, paramName, convertToString(*value).c_str());
		reportMessages();
		return status;
	}
	catch(const std::exception& ex)
	{
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=%s, error=%s", 
                  driverName, functionName, status, function, paramName, convertToString(*value).c_str(), ex.what());
		reportErrors(ex.what());
		return asynError;
	}
}

template<typename T>
asynStatus isisdaeDriver::readArray(asynUser *pasynUser, const char* functionName, T *value, size_t nElements, size_t *nIn)
{
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  const char *paramName = NULL;
	getParamName(function, &paramName);
	m_iface->resetMessages();
	try
	{
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s\n", 
              driverName, functionName, function, paramName);
		reportMessages();
		return status;
	}
	catch(const std::exception& ex)
	{
		*nIn = 0;
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, error=%s", 
                  driverName, functionName, status, function, paramName, ex.what());
		reportErrors(ex.what());
		return asynError;
	}
}

asynStatus isisdaeDriver::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
	asynStatus stat = writeValue(pasynUser, "writeFloat64", value);
    if (stat == asynSuccess)
    {
        stat = ADDriver::writeFloat64(pasynUser, value);
    }
	else
	{
	    callParamCallbacks(); // this flushes P_ErrMsgs
	}
    return stat;
}

asynStatus isisdaeDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    asynStatus stat = writeValue(pasynUser, "writeInt32", value);
    if (stat == asynSuccess)
    {
        stat = ADDriver::writeInt32(pasynUser, value);
    }
	else
	{
	    callParamCallbacks(); // this flushes P_ErrMsgs
	}
    return stat;
}

asynStatus isisdaeDriver::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
{
	int function = pasynUser->reason;
	if (function < FIRST_GP2CAM_PARAM)
	{
		return ADDriver::readFloat64Array(pasynUser, value, nElements, nIn);
	}
//    asynStatus stat = readArray(pasynUser, "readFloat64Array", value, nElements, nIn);
//	callParamCallbacks(); // this flushes P_ErrMsgs
//	doCallbacksFloat64Array(value, *nIn, function, 0);
    return stat;
}

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

//asynStatus isisdaeDriver::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
//{
//	return readValue(pasynUser, "readFloat64", value);
//}

//asynStatus isisdaeDriver::readInt32(asynUser *pasynUser, epicsInt32 *value)
//{
//	return readValue(pasynUser, "readInt32", value);
//}

asynStatus isisdaeDriver::readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason)
{
	int function = pasynUser->reason;
	int status=0;
	const char *functionName = "readOctet";
    const char *paramName = NULL;
	getParamName(function, &paramName);
	m_iface->resetMessages();
	// we don't do much yet
	return ADDriver::readOctet(pasynUser, value, maxChars, nActual, eomReason);

	std::string value_s;
	try
	{
		if (m_iface == NULL)
		{
			throw std::runtime_error("m_iface is NULL");
		}
//		m_iface->getLabviewValue(paramName, &value_s);
		if ( value_s.size() > maxChars ) // did we read more than we have space for?
		{
			*nActual = maxChars;
			if (eomReason) { *eomReason = ASYN_EOM_CNT | ASYN_EOM_END; }
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=\"%s\" (TRUNCATED from %d chars)\n", 
			  driverName, functionName, function, paramName, value_s.substr(0,*nActual).c_str(), value_s.size());
		}
		else
		{
			*nActual = value_s.size();
			if (eomReason) { *eomReason = ASYN_EOM_END; }
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=\"%s\"\n", 
			  driverName, functionName, function, paramName, value_s.c_str());
		}
		strncpy(value, value_s.c_str(), maxChars); // maxChars  will NULL pad if possible, change to  *nActual  if we do not want this
        setStringParam(P_AllMsgs, m_iface->getAllMessages().c_str());
        setStringParam(P_ErrMsgs, "");
		m_iface->resetMessages();
		callParamCallbacks(); // this flushes P_ErrMsgs
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=\"%s\", error=%s", 
                  driverName, functionName, status, function, paramName, value_s.c_str(), ex.what());
		reportErrors(ex.what());
		callParamCallbacks(); // this flushes P_ErrMsgs
		*nActual = 0;
		if (eomReason) { *eomReason = ASYN_EOM_END; }
		value[0] = '\0';
		return asynError;
	}
}

// convert an EPICS waveform data type into what beamline/sample parameters would expect
void isisdaeDriver::translateBeamlineType(std::string& str)
{
	if (str == "CHAR" || str == "UCHAR" || str == "ENUM" || str == "STRING")
	{
		str = "String";
	}
	else if (str == "FLOAT" || str == "DOUBLE")
	{
		str = "Real";
	}
	else if (str == "SHORT" || str == "USHORT" || str == "LONG" || str == "ULONG")
	{
		str = "Integer";
	}
}

asynStatus isisdaeDriver::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *paramName = NULL;
	getParamName(function, &paramName);
    const char* functionName = "writeOctet";
	std::string value_s;
    // we might get an embedded NULL from channel access char waveform records
    if ( (maxChars > 0) && (value[maxChars-1] == '\0') )
    {
        value_s.assign(value, maxChars-1);
    }
    else
    {
        value_s.assign(value, maxChars);
    }
	m_iface->resetMessages();
	try
	{
        if (function == P_RunTitle)
        {
			m_iface->setRunTitle(value_s);
        }
        else if (function == P_SamplePar)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, value_s, boost::is_any_of("\2")); //  name, type, units, value
            if (tokens.size() == 4)
            {
				translateBeamlineType(tokens[1]);
                m_iface->setSampleParameter(tokens[0], tokens[1], tokens[2], tokens[3]);
            }
            else
            {
                throw std::runtime_error("SampleParameter: not enough tokens");
            }
        }
        else if (function == P_BeamlinePar)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, value_s, boost::is_any_of("\2")); //  name, type, units, value
            if (tokens.size() == 4)
            {
				translateBeamlineType(tokens[1]);
                m_iface->setBeamlineParameter(tokens[0], tokens[1], tokens[2], tokens[3]);
            }
            else
            {
                throw std::runtime_error("BeamlineParameter: not enough tokens");
            }
        }
        else if (function == P_RBNumber)
        {
            char user[256];
            getStringParam(P_UserName, sizeof(user), user);
            m_iface->setUserParameters(atol(value_s.c_str()), user, "", "");
        }
        else if (function == P_UserName)
        {
            char rbno[16];
            getStringParam(P_RBNumber, sizeof(rbno), rbno);
            m_iface->setUserParameters(atol(rbno), value_s, "", "");
        }
        else if (function == P_DAESettings)
		{
			m_iface->setDAESettingsXML(value_s);
		}
        else if (function == P_HardwarePeriodsSettings)
		{
			m_iface->setHardwarePeriodsSettingsXML(value_s);
		}
        else if (function == P_UpdateSettings)
		{
			m_iface->setUpdateSettingsXML(value_s);
		}
        else if (function == P_TCBSettings)
		{
		    std::string tcb_xml;
		    if (uncompressString(value_s, tcb_xml) == 0)
			{
                size_t found = tcb_xml.find_last_of(">");  // in cased junk on end
                m_iface->setTCBSettingsXML(tcb_xml.substr(0,found+1));
			}
		}        
        else if (function == P_SnapshotCRPT)
		{
		    beginStateTransition(RS_STORING);
			m_iface->snapshotCRPT(value_s, 1, 1);
            endStateTransition();
		}        
        
		reportMessages();
		status = ADDriver::writeOctet(pasynUser, value_s.c_str(), value_s.size(), nActual);
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=%s\n", 
              driverName, functionName, function, paramName, value_s.c_str());
        if (status == asynSuccess)
        {
		    *nActual = maxChars;   // to keep result happy in case we skipped an embedded trailing NULL
        }
		return status;
	}
	catch(const std::exception& ex)
	{
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=%s, error=%s", 
                  driverName, functionName, status, function, paramName, value_s.c_str(), ex.what());
		reportErrors(ex.what());
		callParamCallbacks(); // this flushes P_ErrMsgs
        endStateTransition();
		*nActual = 0;
		return asynError;
	}
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
	doCallbacksInt16Array(value, nelements, P_nsvData, 0);
}

/// Constructor for the isisdaeDriver class.
/// Calls constructor for the asynPortDriver base class.
/// \param[in] dcomint DCOM interface pointer created by lvDCOMConfigure()
/// \param[in] portName @copydoc initArg0
GP2CameraDriver::GP2CameraDriver(const char *portName, const char* nsvPortName) 
   : ADDriver(portName, 
                    1, /* maxAddr */ 
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
					m_nsvPort(nsvPortName), m_pRaw(NULL)
{					
	int i;
	int status;
    const char *functionName = "GP2CameraDriver";

	m_nsvDataClient = new NSVDataClient(this, nsvPortName, 0, "array1");
	createParam(P_nsvDataString, asynParamInt16Array, &P_nsvData);

    // area detector defaults
//	int maxSizeX = 128, maxSizeY = 128;
//	int maxSizeX = 8, maxSizeY = 8;
	int maxSizeX = 512, maxSizeY = 512;
	NDDataType_t dataType = NDUInt16;
//	NDDataType_t dataType = NDUInt8;
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
}

#if 0
			
void isisdaeDriver::pollerThread3()
{
    static const char* functionName = "isisdaePoller3";
    double delay = 2.0;
	std::vector<long> sums[2], max_vals, spec_nums;
	std::vector<double> rate;
	int frames[2] = {0, 0}, period = 1, first_spec = 1, num_spec = 10, spec_type = 0, nmatch;
	double time_low = 0.0, time_high = -1.0;
	bool b = true;
	int i1, i2, n1, sum, fdiff, fdiff_min = 0, diag_enable = 0;
    lock();
	setIntegerParam(P_diagFrames, 0);
	setIntegerParam(P_diagSum, 0);
	setIntegerParam(P_diagSpecMatch, 0);
    callParamCallbacks();
	// read sums alternately into sums[0] and sums[1] by toggling b so a count rate can be calculated
	while(true)
	{
        unlock();
		epicsThreadSleep(delay);
		i1 = (b == true ? 0 : 1);
		i2 = (b == true ? 1 : 0);
		frames[i1] = m_iface->getGoodFrames(); // read prior to lock in case ICP busy
        lock();
		getIntegerParam(P_diagEnable, &diag_enable);
		if (diag_enable != 1)
			continue;
		fdiff = frames[i1] - frames[i2];
		getIntegerParam(P_diagMinFrames, &fdiff_min);
		if (fdiff < fdiff_min)
			continue;
		getIntegerParam(P_diagSpecShow, &spec_type);
		getIntegerParam(P_diagSpecStart, &first_spec);
		getIntegerParam(P_diagSpecNum, &num_spec);
		getIntegerParam(P_diagPeriod, &period);
		getDoubleParam(P_diagSpecIntLow, &time_low);
		getDoubleParam(P_diagSpecIntHigh, &time_high);
		
        unlock(); // getSepctraSum may take a while so release asyn lock
		m_iface->getSpectraSum(period, first_spec, num_spec, spec_type, 
		     time_low, time_high, sums[i1], max_vals, spec_nums);
        lock();
		n1 = sums[i1].size();
		sum = std::accumulate(sums[i1].begin(), sums[i1].end(), 0);
		rate.resize(n1);
		if ( n1 == sums[i2].size() && fdiff > 0 )
		{
			for(int i=0; i<n1; ++i)
			{
				rate[i] = static_cast<double>(sums[i1][i] - sums[i2][i]) / static_cast<double>(fdiff);
			}
		}
		else
		{
			std::fill(rate.begin(), rate.end(), 0.0);
		}
	    doCallbacksInt32Array(reinterpret_cast<epicsInt32*>(&(sums[i1][0])), n1, P_diagTableSum, 0);
	    doCallbacksInt32Array(reinterpret_cast<epicsInt32*>(&(max_vals[0])), n1, P_diagTableMax, 0);
	    doCallbacksInt32Array(reinterpret_cast<epicsInt32*>(&(spec_nums[0])), n1, P_diagTableSpec, 0);
		doCallbacksFloat64Array(reinterpret_cast<epicsFloat64*>(&(rate[0])), n1, P_diagTableCntRate, 0);
		setIntegerParam(P_diagFrames, fdiff);
		setIntegerParam(P_diagSum, sum);
		// spec_array is padded with -1 at end if less than requested matched
		nmatch = std::count_if(spec_nums.begin(), spec_nums.end(), std::bind2nd(std::greater_equal<int>(),0));
		setIntegerParam(P_diagSpecMatch, nmatch);
        callParamCallbacks();
		b = !b;
    }
}

void isisdaeDriver::pollerThread4()
{
    static const char* functionName = "isisdaePoller4";
	int acquiring = 0;
	int enable = 0;
    int status = asynSuccess;
    int imageCounter;
    int numImages, numImagesCounter;
    int imageMode;
    int arrayCallbacks;
    int old_acquiring = 0;
    NDArray *pImage;
    double acquireTime, acquirePeriod, delay;
    epicsTimeStamp startTime, endTime;
    double elapsedTime;

	while(true)
	{
		lock();
		getIntegerParam(ADAcquire, &acquiring);
		getIntegerParam(P_integralsEnable, &enable);
        getDoubleParam(ADAcquirePeriod, &acquirePeriod);
		if (acquiring == 0 || enable == 0)
		{
			old_acquiring = acquiring;
			unlock();
			epicsThreadSleep( acquirePeriod + (enable == 0 ? 1.0 : 0.0) );
			continue;
		}
		if (old_acquiring == 0)
		{
            setIntegerParam(ADNumImagesCounter, 0);
			old_acquiring = acquiring;
        }
        setIntegerParam(ADStatus, ADStatusAcquire); 
		epicsTimeGetCurrent(&startTime);
        getIntegerParam(ADImageMode, &imageMode);

        /* Get the exposure parameters */
        getDoubleParam(ADAcquireTime, &acquireTime);  // not really used

        setShutter(ADShutterOpen);
        callParamCallbacks();
            
        /* Update the image */
        status = computeImage();
//        if (status) continue;

		// could sleep to make up to acquireTime
		
        /* Close the shutter */
        setShutter(ADShutterClosed);
        
        setIntegerParam(ADStatus, ADStatusReadout);
        /* Call the callbacks to update any changes */
        callParamCallbacks();

        pImage = this->pArrays[0];

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
        pImage->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;
        updateTimeStamp(&pImage->epicsTS);

        /* Get any attributes that have been defined for this driver */
        this->getAttributes(pImage->pAttributeList);

        if (arrayCallbacks) {
          /* Call the NDArray callback */
          /* Must release the lock here, or we can get into a deadlock, because we can
           * block on the plugin lock, and the plugin can be calling us */
          this->unlock();
          asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: calling imageData callback\n", driverName, functionName);
          doCallbacksGenericPointer(pImage, NDArrayData, 0);
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
        if (delay >= 0.0) {
            /* We set the status to waiting to indicate we are in the period delay */
            setIntegerParam(ADStatus, ADStatusWaiting);
            callParamCallbacks();
            this->unlock();
            epicsThreadSleep(delay);
            this->lock();
            setIntegerParam(ADStatus, ADStatusIdle);
            callParamCallbacks();  
        }
        this->unlock();
    }
}

/** Computes the new image data */
int isisdaeDriver::computeImage()
{
    int status = asynSuccess;
    NDDataType_t dataType;
    int itemp;
    int binX, binY, minX, minY, sizeX, sizeY, reverseX, reverseY;
    int xDim=0, yDim=1, colorDim=-1;
    int resetImage;
    int spec_start, trans_mode, maxSizeX, maxSizeY;
    int colorMode;
    int ndims=0;
    NDDimension_t dimsOut[3];
    size_t dims[3];
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
    status |= getIntegerParam(P_integralsSpecStart, &spec_start); 
    status |= getIntegerParam(P_integralsTransformMode, &trans_mode); 
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
            status |= computeArray<epicsInt8>(spec_start, trans_mode, maxSizeX, maxSizeY);
            break;
        case NDUInt8:
            status |= computeArray<epicsUInt8>(spec_start, trans_mode, maxSizeX, maxSizeY);
            break;
        case NDInt16:
            status |= computeArray<epicsInt16>(spec_start, trans_mode, maxSizeX, maxSizeY);
            break;
        case NDUInt16:
            status |= computeArray<epicsUInt16>(spec_start, trans_mode, maxSizeX, maxSizeY);
            break;
        case NDInt32:
            status |= computeArray<epicsInt32>(spec_start, trans_mode, maxSizeX, maxSizeY);
            break;
        case NDUInt32:
            status |= computeArray<epicsUInt32>(spec_start, trans_mode, maxSizeX, maxSizeY);
            break;
        case NDFloat32:
            status |= computeArray<epicsFloat32>(spec_start, trans_mode, maxSizeX, maxSizeY);
            break;
        case NDFloat64:
            status |= computeArray<epicsFloat64>(spec_start, trans_mode, maxSizeX, maxSizeY);
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
    return(status);
}

template <typename epicsType> 
void isisdaeDriver::computeColour(double value, double maxval, epicsType& mono)
{
	epicsType limit = std::numeric_limits<epicsType>::max();
	if (maxval > 0.0)
	{
		mono = static_cast<epicsType>(value / maxval * (double)limit);
	}
	else
	{
		mono = static_cast<epicsType>(0);
	}
}

static double myround(double d)
{
    return (d < 0.0) ? ceil(d - 0.5) : floor(d + 0.5);
}

template <typename epicsType> 
void isisdaeDriver::computeColour(double value, double maxval, epicsType& red, epicsType& green, epicsType& blue)
{
	int i;
	epicsType limit = std::numeric_limits<epicsType>::max();
	if (maxval > 0.0)
	{
		i = (int)myround(255.0 * value / maxval);
	}
	else
	{
		i = 0;
	}
	red = static_cast<epicsType>((double)RainbowColorR[i] / 255.0 * (double)limit);
	green = static_cast<epicsType>((double)RainbowColorG[i] / 255.0 * (double)limit);
	blue = static_cast<epicsType>((double)RainbowColorB[i] / 255.0 * (double)limit);
}


template <typename epicsType> 
int isisdaeDriver::computeArray(int spec_start, int trans_mode, int sizeX, int sizeY)
{
    epicsType *pMono=NULL, *pRed=NULL, *pGreen=NULL, *pBlue=NULL;
    int columnStep=0, rowStep=0, colorMode, numSpec;
    int status = asynSuccess;
    double exposureTime, gain;
    int i, j, k;
	
    status = getDoubleParam (ADGain,        &gain);
    status = getIntegerParam(NDColorMode,   &colorMode);
    status = getDoubleParam (ADAcquireTime, &exposureTime);
	status = getIntegerParam(P_NumSpectra,  &numSpec);

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

	const uint32_t* integrals = m_iface->getEventSpecIntegrals();
	int max_spec_int_size = m_iface->getEventSpecIntegralsSize();
	int nspec = sizeX * sizeY;
	if ( (spec_start + nspec) > (numSpec + 1) )
	{
		nspec = numSpec + 1 - spec_start;
	}
	if ( (spec_start + nspec) > max_spec_int_size )
	{
		nspec = max_spec_int_size - spec_start;
	}
	if (integrals == NULL)
	{
		nspec = 0;
	}
	double* dintegrals = new double[sizeX * sizeY];
	if (trans_mode == 0)
	{
	    for(i=0; i<nspec; ++i)
	    {
		    dintegrals[i] = static_cast<double>(integrals[i+spec_start]);
	    }
	}
	else if (trans_mode == 1)
	{
	    for(i=0; i<nspec; ++i)
	    {
		    dintegrals[i] = sqrt(static_cast<double>(integrals[i+spec_start]));
	    }
	}
	else if (trans_mode == 2)
	{
	    for(i=0; i<nspec; ++i)
	    {
		    dintegrals[i] = log(static_cast<double>(1 + integrals[i+spec_start]));
	    }
	}
	for(i=nspec; i < (sizeX * sizeY); ++i)
	{
		dintegrals[i] = 0.0;		
	}
    k = 0;
	double maxval = 0.0;
	for (i=0; i<sizeY; i++) {
			for (j=0; j<sizeX; j++) {
				if (dintegrals[k] > maxval)
				{
					maxval = dintegrals[k];
				}
				++k;
			}
	}
	
    k = 0;
	for (i=0; i<sizeY; i++) {
		switch (colorMode) {
			case NDColorModeMono:
				for (j=0; j<sizeX; j++) {
					computeColour(dintegrals[k], maxval, *pMono);
					++pMono;
					++k;
				}
				break;
			case NDColorModeRGB1:
			case NDColorModeRGB2:
			case NDColorModeRGB3:
				for (j=0; j<sizeX; j++) {
					computeColour(dintegrals[k], maxval, *pRed, *pGreen, *pBlue);
					pRed   += columnStep;
					pGreen += columnStep;
					pBlue  += columnStep;
					++k;
				}
				pRed   += rowStep;
				pGreen += rowStep;
				pBlue  += rowStep;
				break;
		}
	}
	delete []dintegrals;
    return(status);
}

void isisdaeDriver::getDAEXML(const std::string& xmlstr, const std::string& path, std::string& value)
{
	value = "";
	pugi::xml_document dae_doc;
	pugi::xml_parse_result result = dae_doc.load_buffer(xmlstr.c_str(), xmlstr.size());
 	if (!result)
	{
	    std::cerr << "Error in XML: " << result.description() << " at offset " << result.offset << std::endl;
		return;
	}
	pugi::xpath_node_set nodes = dae_doc.select_nodes(path.c_str());
	if (nodes.size() > 0)
	{
	    value = nodes[0].node().child_value();
	}
}

/** Controls the shutter */
void isisdaeDriver::setShutter(int open)
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

#endif

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
int GP2CameraConfigure(const char *portName, const char *nsvPortName)
{
	try
	{
		GP2CameraDriver* driver = new GP2CameraDriver(portName, nsvPortName);
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

static const iocshArg * const initArgs[] = { &initArg0, &initArg1 };

static const iocshFuncDef initFuncDef = {"GP2CameraConfigure", sizeof(initArgs) / sizeof(iocshArg*), initArgs};

static void initCallFunc(const iocshArgBuf *args)
{
    GP2CameraConfigure(args[0].sval, args[1].sval);
}

static void GP2CameraRegister(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
}

epicsExportRegistrar(GP2CameraRegister);

}

