@echo off
make ./out/app.exe EXE_ARGS="-mwindows"
robocopy Resources out/Resources /E /NFL /NDL /NJH /NJS /nc /ns /np
cd out
start app.exe