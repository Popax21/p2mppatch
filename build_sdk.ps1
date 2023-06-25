cd sdk/mp/src/
./createallprojects.bat
Push-Location tier1
msbuild tier1.vcxproj
Pop-Location