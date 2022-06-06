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
#GP2CameraConfigure("gp2", "nsv", "DATA", 1)
GP2CameraConfigure("gp2", "nsv", "DATA", 0)

dbLoadRecords("$(TOP)/db/ADGP2Camera.template","P=$(MYPVPREFIX),R=GP2:,PORT=gp2,ADDR=0,TIMEOUT=1")

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
dbLoadRecords("NDStdArrays.template", "P=$(MYPVPREFIX),R=GP2:image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,TYPE=Int16,FTVL=SHORT,NELEMENTS=150000,ENABLED=1")
dbLoadRecords("NDStdArrays.template", "P=$(MYPVPREFIX),R=GP2:image2:,PORT=Image2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,NDARRAY_ADDR=1,TYPE=Int16,FTVL=SHORT,NELEMENTS=150000,ENABLED=1")

# This waveform allows transporting 32-bit images
#dbLoadRecords("NDStdArrays.template", "P=$(MYPVPREFIX),R=DAE:image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,TYPE=Int32,FTVL=LONG,NELEMENTS=12000000,ENABLED=1")

## Create an FFT plugin
#NDFFTConfigure("FFT1", 3, 0, "gp2", 0)
#dbLoadRecords("NDFFT.template", "P=$(PREFIX),R=DAE:FFT1:,PORT=FFT1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NAME=FFT1,NCHANS=2048")

NDProcessConfigure("PROC", 3, 0, "gp2", 0)
dbLoadRecords("NDProcess.template", "P=$(MYPVPREFIX),R=GP2:PROC:,PORT=PROC,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1")

NDColorConvertConfigure("COL", 3, 0, "PROC", 0)
dbLoadRecords("NDColorConvert.template","P=$(MYPVPREFIX),R=GP2:COL:,PORT=COL,ADDR=0,TIMEOUT=1,NDARRAY_PORT=PROC,ENABLED=1")  

#ffmpegServerConfigure(8081)
## ffmpegStreamConfigure(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxMemory)
#ffmpegStreamConfigure("C1.MJPG", 2, 0, "gp2", "0")
#dbLoadRecords("$(FFMPEGSERVER)/db/ffmpegStream.template", "P=$(MYPVPREFIX),R=GP2:Stream:,PORT=C1.MJPG,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1")
#ffmpegStreamConfigure("C1.MJPG", 2, 0, "COL", "0")
#dbLoadRecords("$(FFMPEGSERVER)/db/ffmpegStream.template", "P=$(MYPVPREFIX),R=GP2:Stream:,PORT=C1.MJPG,ADDR=0,TIMEOUT=1,NDARRAY_PORT=COL,ENABLED=1")

## ffmpegFileConfigure(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr)
#ffmpegFileConfigure("C1.FILE", 16, 0, "gp2", 0)
#dbLoadRecords("$(FFMPEGSERVER)/db/ffmpegFile.template", "P=$(MYPVPREFIX),R=GP2:File:,PORT=C1.FILE,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1")

NDPvaConfigure("PVA", 3, 0, "gp2", 0, "$(MYPVPREFIX)GP2:V4:image1")
dbLoadRecords("NDPva.template", "P=$(MYPVPREFIX),R=GP2:V4:,PORT=PVA,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1")

NDStdArraysConfigure("ImageSum", 3, 0, "PROC", 0, 0)
dbLoadRecords("NDStdArrays.template", "P=$(MYPVPREFIX),R=GP2:imageSum:,PORT=ImageSum,ADDR=0,TIMEOUT=1,NDARRAY_PORT=PROC,TYPE=Int16,FTVL=SHORT,NELEMENTS=150000,ENABLED=1")

#NDFileHDF5Configure ("HDF5", 3, 0, "gp2", 0)
#dbLoadRecords("NDFileHDF5.template", "P=$(MYPVPREFIX),R=GP2:HDF5:,PORT=HDF5,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1")

#NDFileTIFFConfigure("TIFF", 3, 0, "gp2", 0)
#dbLoadRecords("NDFileTIFF.template", "P=$(MYPVPREFIX),R=GP2:TIFF:,PORT=TIFF,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1")

#NDStatsConfigure("STATS", 3, 0, "gp2", 0)
#dbLoadRecords("NDStats.template", "P=$(MYPVPREFIX),R=GP2:STATS:,PORT=STATS,ADDR=0,TIMEOUT=1,NDARRAY_PORT=gp2,ENABLED=1")

##ISIS## Stuff that needs to be done after all records are loaded but before iocInit is called 
< $(IOCSTARTUP)/preiocinit.cmd

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59Host"

##ISIS## Stuff that needs to be done after iocInit is called e.g. sequence programs 
< $(IOCSTARTUP)/postiocinit.cmd
