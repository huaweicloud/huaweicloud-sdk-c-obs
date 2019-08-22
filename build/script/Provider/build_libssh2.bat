::请先用vs2010打开libssh2.dsw将其转化为vs2010工程，并修改其工程属性，包括opennssl、zlib的头文件和库路径
@echo off

rem 设置变量
set libssh2_version=libssh2-1.9.0
SET LIBSSH2_PATH=%cd%\..\..\..\third_party_groupware\eSDK_Storage_Plugins\%libssh2_version%
SET SOLUTION="%LIBSSH2_PATH%\win32\libssh2.sln"
SET PROJECT="libssh2"
SET ACTION=Rebuild

if {x64} == {%~1} (
SET SOLUTION_CONFIG="OpenSSL DLL Release|x64"
set PLATFORM_TAG=x64
set OUTPUT_TAG=win64_x64_msvc
) else if {win32} == {%~1} (
SET SOLUTION_CONFIG="OpenSSL DLL Release|Win32"
set PLATFORM_TAG=Win32
set OUTPUT_TAG=win32_x86_msvc
) else (
echo "the first parameter should be x64 or win32"
echo "Usage: build_libssh2.bat x64 or build_libssh2.bat win32"
goto Exit
)

echo -----------start to compile libssh2-----------

rem 编译系统工程
"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv" %SOLUTION% /%ACTION% %SOLUTION_CONFIG% /project %PROJECT%


xCOPY "%LIBSSH2_PATH%\win32\%PLATFORM_TAG%\Release_dll\*.dll" "%LIBSSH2_PATH%\..\build\%libssh2_version%\bin\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBSSH2_PATH%\win32\%PLATFORM_TAG%\Release_dll\*.lib" "%LIBSSH2_PATH%\..\build\%libssh2_version%\lib\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBSSH2_PATH%\include\*.h" "%LIBSSH2_PATH%\..\build\%libssh2_version%\include\" /y

xCOPY "%LIBSSH2_PATH%\win32\%PLATFORM_TAG%\Release_dll\*.dll" "%LIBSSH2_PATH%\bin\" /y
xCOPY "%LIBSSH2_PATH%\win32\%PLATFORM_TAG%\Release_dll\*.lib" "%LIBSSH2_PATH%\lib\" /y
::xCOPY "%LIBSSH2_PATH%\include\*.h" "%LIBSSH2_PATH%\include\" /y

xCOPY "%LIBSSH2_PATH%\win32\%PLATFORM_TAG%\Release_dll\*.dll" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\bin\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBSSH2_PATH%\win32\%PLATFORM_TAG%\Release_dll\*.lib" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\lib\%OUTPUT_TAG%\release\" /y
xCOPY "%LIBSSH2_PATH%\include\*.h" "%cd%\..\..\..\source\eSDK_OBS_API\eSDK_OBS_API_C++\include\" /y
  
:Exit