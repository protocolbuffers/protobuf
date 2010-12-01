write-host `nRunning build. Please wait...

$iterations = 10

for ($i=1; $i -le $iterations; $i++)
{
	$sw = [System.Diagnostics.StopWatch]::StartNew()
	& E:\dotnet-protobufs\build\BuildAll.bat
	$sw.Stop()
	$msbuildTotalRunTime += $sw.ElapsedMilliseconds
}

write-host `nMSBuild average speed over $iterations iterations: ($msbuildTotalRunTime/$iterations) milliseconds