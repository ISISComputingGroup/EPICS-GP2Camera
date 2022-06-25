#!../../bin/windows-x64-debug/GP2CameraTest

## You may have to change GP2CameraTest to something else
## everywhere it appears in this file

# Increase this if you get <<TRUNCATED>> or discarded messages warnings in your errlog output
errlogInit2(65536, 256)

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/GP2CameraTest.dbd"
GP2CameraTest_registerRecordDeviceDriver pdbbase

##ISIS## Run IOC initialisation 
< $(IOCSTARTUP)/init.cmd

epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")
asynSetMinTimerPeriod(0.001)

# This waveform allows transporting 8-bit images
# needs to fit in EPICS_CA_MAX_ARRAY_BYTES
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES",  "20000000")
#

NetShrVarConfigure("nsv", "sec1", "$(TOP)/GP2CameraTestApp/src/netvarconfig.xml", 50, 0)
## pass 1 as 4th argument to do callbacks on PV with NSV array data
#GP2CameraConfigure("gp2", "nsv", "DATA", 1, 10)
GP2CameraConfigure("gp2", "nsv", "DATA", 0, 10)

dbLoadRecords("$(TOP)/db/ADGP2Camera.template","P=$(MYPVPREFIX),R=GP2:,PORT=gp2,ADDR=0,TIMEOUT=1,ENABLED=1,DATATYPE=4,TYPE=Int32")
dbLoadRecords("$(TOP)/db/ADGP2Camera.template","P=$(MYPVPREFIX),R=GP2:TOF:,PORT=gp2,ADDR=1,TIMEOUT=1,ENABLED=1,DATATYPE=4,TYPE=Int32")

## Load record instances

##ISIS## Load common DB records 
< $(IOCSTARTUP)/dbload.cmd

## Load our record instances
dbLoadRecords("db/TestGP2Camera.db","P=$(MYPVPREFIX)GP2:")
dbLoadRecords("db/GP2extra.db","P=$(MYPVPREFIX)GP2:")

NDTransformConfigure("RIMAGE1", 3, 0, "gp2", 0, 0)
NDTransformConfigure("RIMAGE2", 3, 0, "gp2", 1, 0)
dbLoadRecords("NDTransform.template", "P=$(MYPVPREFIX),R=GP2:RIMAGE1:,PORT=RIMAGE1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,NDARRAY_ADDR=0,DATATYPE=4,ENABLED=1,TYPE=Int32,FTVL=LONG,NELEMENTS=150000")
dbLoadRecords("NDTransform.template", "P=$(MYPVPREFIX),R=GP2:RIMAGE2:,PORT=RIMAGE2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,NDARRAY_ADDR=1,DATATYPE=4,ENABLED=1,TYPE=Int32,FTVL=LONG,NELEMENTS=150000")

NDROIConfigure("ROI1", 3, 0, "RIMAGE1", 0, 0)
NDROIConfigure("ROI2", 3, 0, "RIMAGE2", 0, 0)
dbLoadRecords("NDROI.template", "P=$(MYPVPREFIX),R=GP2:ROI1:,PORT=ROI1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=RIMAGE1,NDARRAY_ADDR=0,DATATYPE=4,ENABLED=1,TYPE=Int32,FTVL=LONG,NELEMENTS=150000")
dbLoadRecords("NDROI.template", "P=$(MYPVPREFIX),R=GP2:ROI2:,PORT=ROI2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=RIMAGE2,NDARRAY_ADDR=0,DATATYPE=4,ENABLED=1,TYPE=Int32,FTVL=LONG,NELEMENTS=150000")

NDStatsConfigure("STATS1", 3, 0, "ROI1", 0)
NDStatsConfigure("STATS2", 3, 0, "ROI2", 0)
NDTimeSeriesConfigure("STATS1_TS", 100, 0, "ROI1", 0, 22, 0, 0, 0, 0)
NDTimeSeriesConfigure("STATS2_TS", 100, 0, "ROI2", 0, 22, 0, 0, 0, 0)
dbLoadRecords("NDStats.template", "P=$(MYPVPREFIX),R=GP2:STATS1:,PORT=STATS1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI1,ENABLED=1,DATATYPE=4,TYPE=Int32,FTVL=LONG,NCHANS=1,XSIZE=1,YSIZE=1,HIST_SIZE=1")
dbLoadRecords("NDStats.template", "P=$(MYPVPREFIX),R=GP2:STATS2:,PORT=STATS2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI2,ENABLED=1,DATATYPE=4,TYPE=Int32,FTVL=LONG,NCHANS=1,XSIZE=1,YSIZE=1,HIST_SIZE=1")

NDStdArraysConfigure("IMAGE1", 3, 0, "ROI1", 0, 0)
NDStdArraysConfigure("IMAGE2", 3, 0, "ROI2", 0, 0)
dbLoadRecords("NDStdArrays.template", "P=$(MYPVPREFIX),R=GP2:IMAGE1:,PORT=IMAGE1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI1,NDARRAY_ADDR=0,TYPE=Int32,FTVL=LONG,NELEMENTS=150000,ENABLED=1,DATATYPE=4")
dbLoadRecords("NDStdArrays.template", "P=$(MYPVPREFIX),R=GP2:IMAGE2:,PORT=IMAGE2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI2,NDARRAY_ADDR=0,TYPE=Int32,FTVL=LONG,NELEMENTS=150000,ENABLED=1,DATATYPE=4")

NDPvaConfigure("PVA1", 3, 0, "ROI1", 0, "$(MYPVPREFIX)GP2:V4:IMAGE1:DATA")
NDPvaConfigure("PVA2", 3, 0, "ROI2", 0, "$(MYPVPREFIX)GP2:V4:IMAGE2:DATA")
dbLoadRecords("NDPva.template", "P=$(MYPVPREFIX),R=GP2:V4:IMAGE1,PORT=PVA1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI1,ENABLED=1,DATATYPE=4")
dbLoadRecords("NDPva.template", "P=$(MYPVPREFIX),R=GP2:V4:IMAGE2,PORT=PVA2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI2,ENABLED=1,DATATYPE=4")

#NDFileHDF5Configure ("HDF5", 3, 0, "gp2", 0)
#dbLoadRecords("NDFileHDF5.template", "P=$(MYPVPREFIX),R=GP2:HDF5:,PORT=HDF5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1")

#NDFileTIFFConfigure("TIFF", 3, 0, "gp2", 0)
#dbLoadRecords("NDFileTIFF.template", "P=$(MYPVPREFIX),R=GP2:TIFF:,PORT=TIFF,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1")

##ISIS## Stuff that needs to be done after all records are loaded but before iocInit is called 
< $(IOCSTARTUP)/preiocinit.cmd

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59Host"

##ISIS## Stuff that needs to be done after iocInit is called e.g. sequence programs 
< $(IOCSTARTUP)/postiocinit.cmd
