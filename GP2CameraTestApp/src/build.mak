TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

### NOTE: there should only be one build.mak for a given IOC family and this should be located in the ###-IOC-01 directory

#=============================
# Build the IOC application isisdaeTest
# We actually use $(APPNAME) below so this file can be included by multiple IOCs

PROD_IOC_WIN32 = $(APPNAME)
# isisdaeTest.dbd will be created and installed
DBD += $(APPNAME).dbd

PROD_NAME = $(APPNAME)
include $(ADCORE)/ADApp/commonDriverMakefile

# we get base, asyn + areadetetor standard plugins as part of commonDriverMakefile include
## ISIS standard dbd ##
$(APPNAME)_DBD += icpconfig.dbd
$(APPNAME)_DBD += pvdump.dbd
$(APPNAME)_DBD += caPutLog.dbd
$(APPNAME)_DBD += utilities.dbd
## add other dbd here ##
$(APPNAME)_DBD += GP2Camera.dbd
$(APPNAME)_DBD += ffmpegServer.dbd
$(APPNAME)_DBD += NetShrVar.dbd
$(APPNAME)_DBD += NetStreams.dbd

# Add all the support libraries needed by this IOC
## ISIS standard libraries ##
$(APPNAME)_LIBS += seq pv
$(APPNAME)_LIBS += devIocStats 
$(APPNAME)_LIBS += pvdump $(MYSQLLIB) easySQLite sqlite 
$(APPNAME)_LIBS += caPutLog
$(APPNAME)_LIBS += icpconfig
$(APPNAME)_LIBS += autosave
$(APPNAME)_LIBS += utilities
## Add other libraries here ##
#$(APPNAME)_LIBS += xxx
$(APPNAME)_LIBS += GP2Camera NetShrVar NetStreams
$(APPNAME)_LIBS += busy asyn oncrpc zlib libjson pcrecpp pcre pugixml
$(APPNAME)_LIBS += ffmpegServer
$(APPNAME)_LIBS += avdevice
$(APPNAME)_LIBS += avformat
$(APPNAME)_LIBS += avcodec
$(APPNAME)_LIBS += avutil
$(APPNAME)_LIBS += swscale

# isisdaeTest_registerRecordDeviceDriver.cpp derives from isisdaeTest.dbd
$(APPNAME)_SRCS += $(APPNAME)_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
$(APPNAME)_SRCS_DEFAULT += $(APPNAME)Main.cpp
$(APPNAME)_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#$(APPNAME)_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

ifeq (WIN32,$(OS_CLASS))
ifneq ($(findstring windows,$(EPICS_HOST_ARCH)),)
CVILIB = $(NETSHRVAR)/NetShrVarApp/src/O.$(EPICS_HOST_ARCH)/CVI/extlib/msvc64
else
CVILIB = $(NETSHRVAR)/NetShrVarApp/src/O.$(EPICS_HOST_ARCH)/CVI/extlib/msvc
endif
else
CVILIB=/usr/local/lib
endif

$(APPNAME)_SYS_LIBS_WIN32 += $(CVILIB)/cvinetv $(CVILIB)/cvinetstreams $(CVILIB)/cvisupp $(CVILIB)/cvirt
$(APPNAME)_SYS_LIBS_WIN32 += wldap32 ws2_32 # advapi32 user32 msxml2
$(APPNAME)_SYS_LIBS_Linux += ninetv ninetstreams

ifeq ($(STATIC_BUILD),NO)
USR_LDFLAGS_WIN32 += /NODEFAULTLIB:LIBCMT.LIB /NODEFAULTLIB:LIBCMTD.LIB
endif

# Finally link to the EPICS Base libraries
$(APPNAME)_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
#=============================



