#ifndef GP2CAMERADRIVER_H
#define GP2CAMERADRIVER_H
 
#include "ADDriver.h"

class NSVDataClient;

class GP2CameraDriver : public ADDriver 
{
public:
    GP2CameraDriver(const char *portName, const char* nsvPortName);
 	static void pollerThreadC1(void* arg);
                
    // These are the methods that we override from asynPortDriver
//    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
//	virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
//    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
//  virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
//	virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);
//	virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
 //   virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);
//    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn);
	
    virtual void report(FILE *fp, int details);
//    virtual void setShutter(int open);
    void nsvDataInterruptCallback(asynUser *pasynUser, epicsInt16 *value, size_t nelements);

private:

	int P_nsvData; // double
	#define FIRST_GP2CAM_PARAM P_nsvData
	#define LAST_GP2CAM_PARAM P_nsvData

    NDArray* m_pRaw;
	std::string m_nsvPort;
	NSVDataClient* m_nsvDataClient;
	
	void pollerThread1();
//	void setADAcquire(int acquire);
//	int computeImage();
//    template <typename epicsType> 
//	  void computeColour(double value, double maxval, epicsType& mono);
 //   template <typename epicsType> 
 //     void computeColour(double value, double maxval, epicsType& red, epicsType& green, epicsType& blue);
//	template <typename epicsType> int computeArray(int spec_start, int trans_mode, int sizeX, int sizeY);
 
	
    
};

#define NUM_GP2CAM_PARAMS (&LAST_GP2CAM_PARAM - &FIRST_GP2CAM_PARAM + 1)

#define P_nsvDataString	"NSVDATA"

#endif /* GP2CAMERADRIVER_H */
