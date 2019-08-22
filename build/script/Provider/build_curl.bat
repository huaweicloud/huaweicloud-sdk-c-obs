@echo off

rem 设置变量
set curl_version=curl-7.64.1
SET LIBCURL_PATH=%cd%\..\..\..\third_party_groupware\eSDK_Storage_Plugins\%curl_version%
::SET SOLUTION="%LIBCURL_PATH%\projects\Windows\VC10\curl-all.sln"
::SET SOLUTION_CURL="%LIBCURL_PATH%\projects\Windows\VC10\src\curl.sln"
SET SOLUTION_LIBCURL="%LIBCURL_PATH%\projects\Windows\VC10\lib\libcurl.sln"
if {x64} == {%~1} (
SET SOLUTION_CONFIG="DLL Release - DLL Windows SSPI - DLL WinIDN|x64"
set OUTPUT_TAG=win64_x64_msvc
set BUILD_PLATFORM=Win64
) else if {win32} == {%~1} (
SET SOLUTION_CONFIG="DLL Release - DLL Windows SSPI - DLL WinIDN|Win32"
set OUTPUT_TAG=win32_x86_msvc
set BUILD_PLATFORM=Win32
) else (
echo "the first parameter should be x64 or win32"
echo "Usage: build_curl.bat x64 or build_curl.bat win32"
goto Exit
)

SET ACTION=Rebuild
rem 编译系统工程

echo -----------start to compile curl-----------

::"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv" %SOLUTION% /%ACTION% %SOLUTION_CONFIG%

"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv" %SOLUTION_LIBCURL% /%ACTION% %SOLUTION_CONFIG%
::"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv" %SOLUTION_CURL% /%ACTION% %SOLUTION_CONFIG%

xCOPY "%LIBCURL_PATH%\build\%BUILD_PLATFORM%\VC10\DLL Release - DLL Windows SSPI - DLL WinIDN\*.lib" "%LIBCURL_PATH%\..\build\%curl_version%\%OUTPUT_TAG%\lib\" /y
xCOPY "%LIBCURL_PATH%\build\%BUILD_PLATFORM%\VC10\DLL Release - DLL Windows SSPI - DLL WinIDN\*.dll" "%LIBCURL_PATH%\..\build\%curl_version%\%OUTPUT_TAG%\bin\" /y
xCOPY "%LIBCURL_PATH%\include\curl\*.h" "%LIBCURL_PATH%\..\build\%curl_version%\include\curl\" /y

xCOPY "%LIBCURL_PATH%\build\%BUILD_PLATFORM%\VC10\DLL Release - DLL Windows SSPI - DLL WinIDN\*.lib" "%LIBCURL_PATH%\lib\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBCURL_PATH%\build\%BUILD_PLATFORM%\VC10\DLL Release - DLL Windows SSPI - DLL WinIDN\*.dll" "%LIBCURL_PATH%\bin\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBCURL_PATH%\include\curl\*.h" "%LIBCURL_PATH%\include\curl\" /y

xCOPY "%LIBCURL_PATH%\build\%BUILD_PLATFORM%\VC10\DLL Release - DLL Windows SSPI - DLL WinIDN\*.lib" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\lib\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBCURL_PATH%\build\%BUILD_PLATFORM%\VC10\DLL Release - DLL Windows SSPI - DLL WinIDN\*.dll" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\bin\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBCURL_PATH%\include\curl\*.h" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\include\curl\" /y


:Exit