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

NetShrVarConfigure("nsv", "sec1", "$(TOP)/GP2CameraTestApp/src/netvarconfig.xml", 50, 0)
## pass 1 as 4th argument to do callbacks on PV with array data
GP2CameraConfigure("gp2", "nsv", "DATA", 1)
#GP2CameraConfigure("gp2", "nsv", "DATA", 0)

dbLoadRecords("$(TOP)/db/ADGP2Camera.template","P=$(MYPVPREFIX),R=GP2:,PORT=gp2,ADDR=0,TIMEOUT=1,ENABLED=1,DATATYPE=2,TYPE=Int16")

## Load record instances

##ISIS## Load common DB records 
< $(IOCSTARTUP)/dbload.cmd

## Load our record instances
dbLoadRecords("db/TestGP2Camera.db","P=$(MYPVPREFIX)GP2:")
dbLoadRecords("db/GP2extra.db","P=$(MYPVPREFIX)GP2:")

NDStdArraysConfigure("Image1", 3, 0, "gp2", 0, 0)
NDStdArraysConfigure("Image2", 3, 0, "gp2", 1, 0)

# This waveform allows transporting 8-bit images
# needs to fit in EPICS_CA_MAX_ARRAY_BYTES
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES",  "20000000")
#
dbLoadRecords("NDStdArrays.template", "P=$(MYPVPREFIX),R=GP2:image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,NDARRAY_ADDR=0,TYPE=Int16,FTVL=SHORT,NELEMENTS=150000,ENABLED=1,DATATYPE=2")
dbLoadRecords("NDStdArrays.template", "P=$(MYPVPREFIX),R=GP2:image2:,PORT=Image2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,NDARRAY_ADDR=1,TYPE=Int16,FTVL=SHORT,NELEMENTS=150000,ENABLED=1,DATATYPE=2")

NDProcessConfigure("PROC1", 3, 0, "gp2", 0)
dbLoadRecords("NDProcess.template", "P=$(MYPVPREFIX),R=GP2:PROC1:,PORT=PROC1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1,DATATYPE=2,TYPE=Int16,FTVL=SHORT,NELEMENTS=150000")

NDTransformConfigure("TPROC1", 3, 0, "PROC1", 0, 0)
dbLoadRecords("NDTransform.template", "P=$(MYPVPREFIX),R=GP2:TPROC1:,PORT=TPROC1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=PROC1,NDARRAY_ADDR=0,DATATYPE=4,ENABLED=1,TYPE=Int32,FTVL=LONG,NELEMENTS=150000")

NDROIConfigure("ROI1", 3, 0, "TPROC1", 0, 0)
dbLoadRecords("NDROI.template", "P=$(MYPVPREFIX),R=GP2:roi1:,PORT=ROI1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=TPROC1,NDARRAY_ADDR=0,DATATYPE=4,ENABLED=1,TYPE=Int32,FTVL=LONG,NELEMENTS=150000")

NDStatsConfigure("STATS1", 3, 0, "ROI1", 0)
NDTimeSeriesConfigure("STATS1_TS", 100, 0, "ROI1", 0, 22, 0, 0, 0, 0)
dbLoadRecords("NDStats.template", "P=$(MYPVPREFIX),R=GP2:STATS1:,PORT=STATS1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI1,ENABLED=1,DATATYPE=4,TYPE=Int32,FTVL=LONG,NCHANS=1,XSIZE=1,YSIZE=1,HIST_SIZE=1")


#NDColorConvertConfigure("COL", 3, 0, "PROC", 0)
#dbLoadRecords("NDColorConvert.template","P=$(MYPVPREFIX),R=GP2:COL:,PORT=COL,ADDR=0,TIMEOUT=1,NDARRAY_PORT=PROC,ENABLED=1")  

NDPvaConfigure("PVA1", 3, 0, "ROI1", 0, "$(MYPVPREFIX)GP2:V4:image")
dbLoadRecords("NDPva.template", "P=$(MYPVPREFIX),R=GP2:V4:,PORT=PVA1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI1,ENABLED=1,DATATYPE=4")

NDStdArraysConfigure("ImageSum", 3, 0, "ROI1", 0, 0)
dbLoadRecords("NDStdArrays.template", "P=$(MYPVPREFIX),R=GP2:imageSum:,PORT=ImageSum,ADDR=0,TIMEOUT=1,NDARRAY_PORT=ROI1,TYPE=Int32,FTVL=LONG,NELEMENTS=150000,ENABLED=1,DATATYPE=4")

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

dbpf "$(MYPVPREFIX)GP2:PROC1:FilterType", "Sum"
dbpf "$(MYPVPREFIX)GP2:PROC1:NumFilter", "1000000000"
dbpf "$(MYPVPREFIX)GP2:PROC1:EnableFilter", "1"
dbpf "$(MYPVPREFIX)GP2:PROC1:DataTypeOut", "Int32"
dbpf "$(MYPVPREFIX)GP2:DataType", "2"
