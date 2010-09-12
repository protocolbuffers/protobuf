write-host `nRunning build. Please wait...

$iterations = 10

for ($i=1; $i -le $iterations; $i++)
{
	$sw = [System.Diagnostics.StopWatch]::StartNew()
	#& $env:windir\Microsoft.NET\Framework\v4.0.30319\msbuild.exe build.csproj /m /nologo /v:q
	& $env:windir\Microsoft.NET\Framework\v4.0.30319\msbuild.exe build.csproj /m /nologo /v:q /t:BuildAll /p:CompileGroup=BuildAll
	$sw.Stop()
	$msbuildTotalRunTime += $sw.ElapsedMilliseconds

	$sw = [System.Diagnostics.StopWatch]::StartNew()
	#& "..\nant-0.91-alpha2\bin\NAnt.exe" -buildfile:"..\ProtocolBuffers.build" -nologo -q clean-build
	& "..\nant-0.91-alpha2\bin\NAnt.exe" -buildfile:"..\ProtocolBuffers.build" -nologo -q clean-build-all
	$sw.Stop()
	$nantTotalRunTime += $sw.ElapsedMilliseconds
}

write-host `nMSBuild average speed over $iterations iterations: ($msbuildTotalRunTime/$iterations) milliseconds
write-host NAnt average speed over $iterations iterations: ($nantTotalRunTime/$iterations) milliseconds`n
write-host MSBuild execution speed: ([Math]::Round($(100/$nantTotalRunTime*$msbuildTotalRunTime), 2))% "(as a percentage of NAnt execution speed)"`n