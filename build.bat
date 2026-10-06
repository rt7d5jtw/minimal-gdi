@echo off
setlocal

set "ENV_CACHE=%~dp0.msvc_env.bat"
set "BUILD_DIR=%~dp0build"

:: -----------------------------------------------------------------------------
:: Command-Line Argument Handling
:: -----------------------------------------------------------------------------
set "BUILD_MODE=debug"

for %%a in (%*) do (
    if /i "%%~a"=="clean" (
        echo Cleaning build artifacts and environment cache...
        if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
        if exist "%ENV_CACHE%" del /f /q "%ENV_CACHE%"
        if exist "%~dp0msvc_debug_log.txt" del /f /q "%~dp0msvc_debug_log.txt"
        echo Cleanup complete.
        exit /b 0
    )
    if /i "%%~a"=="release" set "BUILD_MODE=release"
    if /i "%%~a"=="debug" set "BUILD_MODE=debug"
)

:: -----------------------------------------------------------------------------
:: Locate Visual Studio and initialize environment (targeting x86 for XP)
:: -----------------------------------------------------------------------------
if defined VCINSTALLDIR goto :EnvironmentReady
if defined DevEnvDir goto :EnvironmentReady

if exist "%ENV_CACHE%" (
    call "%ENV_CACHE%"
    if defined VCINSTALLDIR goto :EnvironmentReady
)

echo MSVC environment cache not found. Locating Visual Studio (x86)...

:: Check for modern VS (2017 - 2022+) using vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath 2^>nul`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvars32.bat" (
            set "VCVARS_SCRIPT=%%i\VC\Auxiliary\Build\vcvars32.bat"
            set "VCVARS_ARG="
            goto :GenerateCache
        )
        if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VCVARS_SCRIPT=%%i\VC\Auxiliary\Build\vcvarsall.bat"
            set "VCVARS_ARG=x86"
            goto :GenerateCache
        )
    )
)

:: Check legacy environment variables (VS 2010 / 2008 / 2005 / etc.)
for %%V in (140 120 110 100 90 80 71 70) do (
    if not defined VCVARS_SCRIPT (
        if defined VS%%VCOMNTOOLS (
            call :CheckLegacyTools VS%%VCOMNTOOLS
        )
    )
)
if defined VCVARS_SCRIPT goto :GenerateCache

:: Modern VS directory fallback scan (Targeting 32-bit x86)
for %%D in ("%ProgramFiles(x86)%" "%ProgramFiles%" "C:\Program Files (x86)" "C:\Program Files") do (
    if exist "%%~D" (
        for %%Y in (2022 2019 2017) do (
            for %%E in (Community Professional Enterprise BuildTools) do (
                if exist "%%~D\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars32.bat" (
                    set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars32.bat"
                    set "VCVARS_ARG="
                    goto :GenerateCache
                )
            )
        )
    )
)

:: Legacy VS directory scan (VS 2015 down to VC6)
for %%D in ("%ProgramFiles%" "%ProgramFiles(x86)%" "C:\Program Files" "C:\Program Files (x86)") do (
    if exist "%%~D" (
        for %%V in (14.0 12.0 11.0 10.0 9.0 8.0 ".NET 2003" ".NET") do (
            if not defined VCVARS_SCRIPT (
                if exist "%%~D\Microsoft Visual Studio %%~V\VC\bin\vcvars32.bat" (
                    set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio %%~V\VC\bin\vcvars32.bat"
                    set "VCVARS_ARG="
                    goto :GenerateCache
                )
                if exist "%%~D\Microsoft Visual Studio %%~V\VC\vcvarsall.bat" (
                    set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio %%~V\VC\vcvarsall.bat"
                    set "VCVARS_ARG=x86"
                    goto :GenerateCache
                )
            )
        )
        if not defined VCVARS_SCRIPT (
            if exist "%%~D\Microsoft Visual Studio\VC98\bin\VCVARS32.BAT" (
                set "VCVARS_SCRIPT=%%~D\Microsoft Visual Studio\VC98\bin\VCVARS32.BAT"
                set "VCVARS_ARG="
                goto :GenerateCache
            )
        )
    )
)

echo Error: Could not locate a working Visual Studio installation. >&2
exit /b 1

:GenerateCache
echo Initializing MSVC environment and generating cache...
if defined VCVARS_ARG (
    call "%VCVARS_SCRIPT%" %VCVARS_ARG% > "%~dp0msvc_debug_log.txt"
) else (
    call "%VCVARS_SCRIPT%" > "%~dp0msvc_debug_log.txt"
)

(
    echo @echo off
    echo set "VCINSTALLDIR=%VCINSTALLDIR%"
    echo set "DevEnvDir=%DevEnvDir%"
    echo set "INCLUDE=%INCLUDE%"
    echo set "LIB=%LIB%"
    echo set "LIBPATH=%LIBPATH%"
    echo set "PATH=%PATH%"
) > "%ENV_CACHE%"

:EnvironmentReady

:: -----------------------------------------------------------------------------
:: Build Configuration & Flags
:: -----------------------------------------------------------------------------
set "EXECUTABLE=gdidemo.exe"
set "WARNING_FLAGS=-W4 -wd4201 -wd4100 -wd4189 -wd4505"
set "COMMON_FLAGS=-FC -I..\src %WARNING_FLAGS%"
set "SOURCES=..\src\main.c ..\src\drawing.c"
set "LIBS=user32.lib gdi32.lib"

:: Target Windows XP (subsystem version 5.01 for x86)
set "LINKER_FLAGS=/subsystem:windows,5.01"

if /i "%BUILD_MODE%"=="release" (
    set "MODE_FLAGS=-O2 -Oi -DNDEBUG"
    set "LINKER_FLAGS=/subsystem:windows,5.01"
) else (
    set "MODE_FLAGS=-Od -Zi -DDEBUG -Fdgdidemo.pdb"
    set "LINKER_FLAGS=/subsystem:windows,5.01 /DEBUG"
)

set "COMPILER_FLAGS=%MODE_FLAGS% %COMMON_FLAGS%"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
pushd "%BUILD_DIR%"

:: -----------------------------------------------------------------------------
:: Build Execution
:: -----------------------------------------------------------------------------
echo Compiling %EXECUTABLE% [%BUILD_MODE%]...
cl %COMPILER_FLAGS% -Fe%EXECUTABLE% %SOURCES% %LIBS% /link %LINKER_FLAGS%
set BUILD_STATUS=%ERRORLEVEL%

echo --------------------------------------------------
if %BUILD_STATUS% EQU 0 (
    echo Build DONE: %BUILD_DIR%\%EXECUTABLE%
) else (
    echo Build FAILED with error code %BUILD_STATUS%
)
echo --------------------------------------------------

popd
exit /b %BUILD_STATUS%

:: -----------------------------------------------------------------------------
:: Helper Subroutines
:: -----------------------------------------------------------------------------
:CheckLegacyTools
set "TOOL_VAR_NAME=%~1"
call set "LEGACY_DIR=%%%TOOL_VAR_NAME%%%"

if exist "%LEGACY_DIR%..\..\VC\bin\vcvars32.bat" (
    set "VCVARS_SCRIPT=%LEGACY_DIR%..\..\VC\bin\vcvars32.bat"
    set "VCVARS_ARG="
) else if exist "%LEGACY_DIR%..\..\VC\vcvarsall.bat" (
    set "VCVARS_SCRIPT=%LEGACY_DIR%..\..\VC\vcvarsall.bat"
    set "VCVARS_ARG=x86"
) else if exist "%LEGACY_DIR%vsvars32.bat" (
    set "VCVARS_SCRIPT=%LEGACY_DIR%vsvars32.bat"
    set "VCVARS_ARG="
)
exit /b 0
