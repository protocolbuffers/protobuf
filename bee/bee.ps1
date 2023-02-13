# Bee bootstrap script. If you use powershell as your shell, runnning ./bee
# will directly run this script. If you're running cmd.exe running ./bee will
# run the bee.cmd and that will run powershell to run this script. This script
# is responsible for downloading a dotnet runtime if required, and for
# downloading a bee distribution if required, and for starting the actual bee
# driver executable.

trap
{
  # Ensure that we exit with an error code if there are uncaught exceptions.
  $ErrorActionPreference = "Continue"
  Write-Error $_
  exit 1
}

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.IO.Compression.FileSystem

# These two values will be replaced by the buildprocess when this script is
# used in a bootstrap distribution. When not replaced this script assumes it
# lives next to the standalone driver.
$use_bee_from_steve="bee/4614ca8dfa47_e30d39dc9e8f44f17d5d0e36aed8224ec76a26ca0702ca4a2bc2679504ef7520.zip"
$use_bee_from_steve_repo="https://public-stevedore.unity3d.com/r/public"

if("$use_bee_from_steve_repo" -Match "testing")
{
  Write-Host "Warning this bee bootstrap script is using a bee from the testing repository. This fine for testing, but not to use in production."
}

# Stevedore artifact for the dotnet runtimes. these are produced by the
# yamato CI for our "netcorerun" repo. The zip file contains an info file
# that describes from which git commit / yamato-id it was built.  They are
# plain upstream packages, just unzipped & rezipped with this extra
# information added.
$dotnet_runtime_win_x64 = "dotnet-runtime-win-x64/a057d2d_16de45f31f424625fa1de7ec0cf4d5969c60412a701e17ab6083f83ea1bcb420.zip" #net-runtime 6.0.8

$global:steve_artifact_return_value = ""
function Get-Stevedore-Artifact($steve_name, $steve_repo_url, $output_name)
{
    # We could extend this to parse the Stevedore.conf, and to attempt
    # downloads from multiple mirrors...
    $unzip_dir_path = "$HOME\.beebootstrap\$steve_name"

    $unpacked_marker = "$unzip_dir_path\.UNPACKED"
    if(![System.IO.File]::Exists($unpacked_marker)) {

        if (Test-Path $unzip_dir_path) {
            Remove-Item $unzip_dir_path -Recurse -Force
        }

        $download_link = "$steve_repo_url/$steve_name"
        $random = Get-Random
        $temporary_dir = "$HOME/.beebootstrap/download_$random"
        New-Item -ItemType Directory -Force -Path $temporary_dir | Out-Null
        $downloaded_file = "$temporary_dir/download.zip"

        # Turn off this weird powershell progress thing. Not only is it super
        # ugly, it's also reported to be the cause of making the download
        # superslow.
        # https://stackoverflow.com/questions/28682642/powershell-why-is-using-invoke-webrequest-much-slower-than-a-browser-download
        $ProgressPreference = 'SilentlyContinue'

        Write-Host "Downloading $output_name"

        # Download into the temporary directory
        Invoke-WebRequest $download_link -outfile "$downloaded_file"

        # Make sure the parent directory of the target directory already exists
        $parent_dir = Split-Path -Parent $unzip_dir_path
        if( -Not (Test-Path -Path $parent_dir) )
        {
            New-Item -ItemType Directory -Path $parent_dir | Out-Null
        }

        Write-Host "Unzipping $output_name"
        [System.IO.Compression.ZipFile]::ExtractToDirectory("$downloaded_file", "$unzip_dir_path")

        # Create the .UNPACKED marker file
        Out-File -FilePath "$unpacked_marker"

        Remove-Item "$temporary_dir" -Recurse -Force
    }

    # We assign to a global instead of returning a value, since returning 
    # a value is somehow very brittle, as any command in this function
    # that isn't piped to Out-Null might actually pollute the return value.
    $global:steve_artifact_return_value = $unzip_dir_path
}

if( $use_bee_from_steve -eq "no") {
    # This script supports running as part of a full bee distribution. In
    # this case, use_bee_from_steve is not set, and we find the path to
    # the distribution by looking at where the script itself is. It should
    # be placed in Standalone/Release of the distribution.
    $standalone_release=$PSScriptRoot
}
else {
    # We also support downloading the bee distribution from a stevedore
    # server. In this case use_bee_from_steve should be set to a stevedore
    # artifact name. We'll download it and run it. In this mode, the only
    # thing a user needs to version in their repo is this script.
    Get-Stevedore-Artifact "$use_bee_from_steve" "$use_bee_from_steve_repo" "Bee distribution"
    $standalone_release = "$global:steve_artifact_return_value/Standalone/Release"
}

$distribution_path = [System.IO.Path]::GetFullPath("$standalone_release/../..")
$standalone_path = "$standalone_release/Bee.StandaloneDriver.exe"
if(![System.IO.File]::Exists($standalone_path)) {
    $standalone_path = "$standalone_release/Bee.StandaloneDriver.dll"
}

$dotnet_steve_artifact = "$dotnet_runtime_win_x64"
Get-Stevedore-Artifact $dotnet_steve_artifact "https://public-stevedore.unity3d.com/r/public" "Dotnet runtime"
$dotnet_exe = "$global:steve_artifact_return_value\dotnet.exe"

# We assign BEE_DOTNET_MUXER env var here, so that the bee that is running
# knows how it can use this dotnet runtime to start other net5 framework
# dependent apps. It uses this to run the stevedore downloader program on net5.
try {
    $env:BEE_DISTRIBUTION_PATH = $distribution_path
    $env:BEE_DOTNET_MUXER = $dotnet_exe
    $env:DOTNET_MULTILEVEL_LOOKUP=0

    # Run the wrapper and pass on any user args
    & $dotnet_exe $standalone_path $args
} finally {
    $env:BEE_DISTRIBUTION_PATH = $Null
    $env:BEE_DOTNET_MUXER = $Null
}

# Ensure exit code is preserved when running from another script.
exit $LastExitCode
# ReleaseNotes: 


# Automatically generated by Yamato Job: https://unity-ci.cds.internal.unity3d.com/job/20992285
#MANIFEST public: bee/4614ca8dfa47_e30d39dc9e8f44f17d5d0e36aed8224ec76a26ca0702ca4a2bc2679504ef7520.zip