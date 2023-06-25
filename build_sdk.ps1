cd sdk/mp/src/
./createallprojects.bat
Push-Location tier1
msbuild tier1_win32.sln
Pop-Location