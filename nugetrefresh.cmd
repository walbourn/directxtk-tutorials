@echo off
setlocal
set NUGET=c:\util\nuget.exe
if NOT exist %NUGET% goto nuerror
if %VisualStudioVersion%.==. goto deverr
echo Updates all projects to latest NuGet version
echo .
pause
pushd DX11
for /D %%1 in (*) do (pushd %%1
if exist "%%1.sln" (
%NUGET% restore "%%1.sln"
%NUGET% update "%%1.sln") 
popd)
popd
pushd DX12
for /D %%1 in (*) do (pushd %%1
if exist "%%1.sln" (
%NUGET% restore "%%1.sln"
%NUGET% update "%%1.sln")
popd)
popd
goto :eof
:nuerror
echo ERROR cannot find %NUGET%
goto :eof
:deverr
echo ERROR Run this script from a Visual Studio Developer Command Prompt

