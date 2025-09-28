@echo off
make ./out/app.exe CONST_ARGS="-D_DEBUG=1"
if errorlevel 1 GOTO error
robocopy Resources out/Resources /E /NFL /NDL /NJH /NJS /nc /ns /np
cd out
start /WAIT /B app.exe
if not errorlevel 0 GOTO error
GOTO noerror
:error
pause
GOTO noerror
:noerror
cd ..
make clean