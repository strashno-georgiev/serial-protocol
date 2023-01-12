@ECHO OFF

FOR %%i IN (Exe, Output, Release, Debug, Temp, bin, obj) DO IF EXIST %%i RD %%i /S/Q
FOR %%i IN (EX~, DEP, OPT, PLG, APS, NCB, TMP, LOG, ILK, SIO, ERR, TPU, RES) DO IF EXIST *.%%i DEL *.%%i
