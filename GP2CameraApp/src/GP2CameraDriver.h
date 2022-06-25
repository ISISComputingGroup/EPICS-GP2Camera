#ifndef GP2CAMERADRIVER_H
#define GP2CAMERADRIVER_H
 
#include "ADDriver.h"

class NSVDataClient;

class GP2CameraDriver : public ADDriver 
{
public:
    GP2CameraDriver(const char *portName, const char* nsvPortName, const char* nsvParam, int options, int binSize);
 	static void pollerThreadC1(void* arg);
    static void fakeDataC(void* arg);
    // These are the methods that we override from asynPortDriver
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
//    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
//  virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
	virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);
	virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
 //   virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);
//    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn);
//    virtual asynStatus readInt16Array(asynUser *pasynUser, epicsInt16 *value, size_t nElements, size_t *nIn);
	
    virtual void report(FILE *fp, int details);
    virtual void setShutter(int open);
    void nsvDataInterruptCallback(asynUser *pasynUser, epicsInt16 *value, size_t nelements);

private:

	int P_nsvData; // int16 array
    int P_tofHistogram; // int32array
    int P_tofBinValue; // int
    int P_tofBinSize; // int
	int P_testFileName; // string
    
	#define FIRST_GP2CAM_PARAM P_nsvData
	#define LAST_GP2CAM_PARAM P_testFileName

	int m_options;
	int m_old_acquiring;
    int m_binSize;
    int m_tofBinsMax;
    int m_tofBins;
	FILE* m_outfile;
    std::vector<int> m_tof;
	std::vector<epicsInt32> m_values; // values with TOF
	std::vector<epicsInt32> m_valuesSum; // TOF summed values 
	std::string m_filename;
	struct DataQueueMessage
	{
		epicsInt16 *value;
		size_t nelements;
		epicsTimeStamp ts;
		DataQueueMessage(epicsInt16 *value_, size_t nelements_, epicsTimeStamp ts_) : value(value_), nelements(nelements_), ts(ts_) { }
		DataQueueMessage() : value(0), nelements(0) { }
	};
	epicsMessageQueue m_data_queue;
	enum GP2Options { GP2NSVEpicsCallback = 0x1 };
    NDArray* m_pRaw;
    NDArray* m_pRawTOF;
	NSVDataClient* m_nsvDataClient;
	
	void pollerThread1();
    void fakeData();
	void processCameraData(epicsInt16 *value, size_t nelements, epicsTimeStamp* epicsTS);
	void setADAcquire(int acquire);
	int computeImage(epicsInt16 *value, size_t nelements, int tofBinValue);
//    template <typename epicsType> 
//	  void computeColour(double value, double maxval, epicsType& mono);
 //   template <typename epicsType> 
 //     void computeColour(double value, double maxval, epicsType& red, epicsType& green, epicsType& blue);
	template <typename epicsType> int computeArray(epicsInt16* value, size_t nelements, int sizeX, int sizeY, int tofBinValue);
    
};

#define NUM_GP2CAM_PARAMS (&LAST_GP2CAM_PARAM - &FIRST_GP2CAM_PARAM + 1)

#define P_nsvDataString	        "NSVDATA"
#define P_tofHistogramString	"TOF_HISTOGRAM"
#define P_tofBinValueString	    "TOF_BIN_VALUE"
#define P_tofBinSizeString	    "TOF_BIN_SIZE"
#define P_testFileNameString	"TESTFILENAME"

#endif /* GP2CAMERADRIVER_H */
