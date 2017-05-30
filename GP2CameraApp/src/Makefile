TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# Build the IOC support library

USR_INCLUDES += -I"$(TOP)/../../libraries/boost/include" -I"$(AREA_DETECTOR)"
USR_CXXFLAGS += /DNOMINMAX

PROD_IOC += GP2FileReader
GP2FileReader_SRCS += GP2FileReader.cpp
GP2FileReader_LIBS += $(EPICS_BASE_IOC_LIBS)

LIBRARY_IOC += GP2Camera
GP2Camera_SRCS += GP2CameraDriver.cpp

GP2Camera_LIBS += asyn oncrpc utilities zlib pcrecpp pcre pugixml
GP2Camera_LIBS += NetShrVar
GP2Camera_LIBS += ffmpegServer
GP2Camera_LIBS += avdevice
GP2Camera_LIBS += avformat
GP2Camera_LIBS += avcodec
GP2Camera_LIBS += avutil
GP2Camera_LIBS += swscale
GP2Camera_LIBS += $(EPICS_BASE_IOC_LIBS)

DBD += GP2Camera.dbd

include $(ADCORE)/ADApp/commonLibraryMakefile

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
