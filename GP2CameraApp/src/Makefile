TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += GP2Camera

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install GP2Camera.dbd into <top>/dbd
DBD += GP2Camera.dbd

# specify all source files to be compiled and added to the library
#GP2Camera_SRCS += xxx

GP2Camera_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

