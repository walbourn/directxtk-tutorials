rem @echo off
setlocal
set NUGET=c:\util\nuget.exe
if NOT exist %NUGET% goto error
echo Updates all projects to latest NuGet version
echo .
pause
pushd DX11
for /D %%1 in (*) do (pushd %%1
%NUGET% restore "%%1.sln"
%NUGET% update "%%1.sln"
popd)
popd
pushd DX12
for /D %%1 in (*) do (pushd %%1
%NUGET% restore "%%1.sln"
%NUGET% update "%%1.sln"
popd)
popd
goto :eof
:error
echo ERROR cannot find %NUGET%
