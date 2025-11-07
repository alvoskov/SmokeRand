param(
    [Parameter(Mandatory=$true, Position=0)]
    [ValidateNotNullOrEmpty()]
    [string]$battery,

    [Parameter(Mandatory=$true, Position=1)]
    [ValidateNotNullOrEmpty()]
    [string]$generator,

    [Parameter(Position=2)]
    [int]$nthreads = 4
)

$bytes = New-Object byte[] 40
[System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)

$seed = [Convert]::ToBase64String($bytes)

# Check if executable is present in the folder
$exe = Join-Path -Path (Get-Location) -ChildPath "smokerand.exe"
if (-not (Test-Path $exe -PathType Leaf)) {
    # Find it is in the path
    $exe = "smokerand.exe"
}

$argList = @($battery, $generator, "--seed=$seed", "--nthreads=$nthreads")

try {
    & $exe @argList
    $exitCode = $LASTEXITCODE
} catch {
    Write-Error "Error during '$exe' launch: $_"
    exit 1
}

Write-Host "Exit code: $exitCode"
Write-Host "The used seed: $seed"
exit $exitCode
