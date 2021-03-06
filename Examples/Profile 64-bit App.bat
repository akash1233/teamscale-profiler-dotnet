REM Example batch-file to profile a 64-bit-application.
REM For full documentation check out
REM https://github.com/cqse/teamscale-profiler-dotnet/blob/master/documentation/userguide.md

REM Register the profiler (do not change this).
set COR_PROFILER={DD0A1BB6-11CE-11DD-8EE8-3F9E55D89593}

REM Enable the profiler, set to 0 to start the application without profiling.
set COR_ENABLE_PROFILING=1

REM Path to profiler DLL, use Profiler32.dll for 32-bit applications (do not use quotation marks).
set COR_PROFILER_PATH=C:\PATH\TO\PROFILER\Profiler64.dll

REM Path where to store the traces (do not use quotation marks).
set COR_PROFILER_TARGETDIR=C:\PATH\TO\TRACE\LOCATION

REM If you use ngen.exe to build the application, disable this line.
set COR_PROFILER_LIGHT_MODE=1

REM Start the application.
"E:\PATH\TO\APPLICATION\app.exe"
