@echo off
xCOPY %cd%\..\bin\*  %~dp0 /y

xCOPY %cd%\..\include\* %~dp0 /y

xCOPY %cd%\huaweisecurec\include\* %~dp0 /y

xCOPY %cd%\huaweisecurec\lib\* %~dp0 /y

cl demo_windows.cpp /link libeSDKOBS.lib huaweisecurec.lib

@echo on