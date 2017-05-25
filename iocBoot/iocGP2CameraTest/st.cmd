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

NetShrVarConfigure("nsv", "sec1", "$(TOP)/GP2CameraTestApp/src/netvarconfig.xml", 100, 0)
GP2CameraConfigure("gp2", "nsv")
dbLoadRecords("$(TOP)/db/ADGP2Camera.template","P=$(MYPVPREFIX),R=GP2:,PORT=gp2,ADDR=0,TIMEOUT=1")

## Load record instances

##ISIS## Load common DB records 
< $(IOCSTARTUP)/dbload.cmd

## Load our record instances
dbLoadRecords("db/TestGP2Camera.db","P=$(MYPVPREFIX):GP2:")

##ISIS## Stuff that needs to be done after all records are loaded but before iocInit is called 
< $(IOCSTARTUP)/preiocinit.cmd

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59Host"

##ISIS## Stuff that needs to be done after iocInit is called e.g. sequence programs 
< $(IOCSTARTUP)/postiocinit.cmd
