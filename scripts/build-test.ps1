param(
    [Parameter(Mandatory = $true)]
    [string]$QtPrefix,
    [string]$BuildDir,
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Configuration = 'Release',
    [string]$Generator
)

$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) {
    $BuildDir = Join-Path $projectDir 'build/windows'
}
$originalPath = $env:PATH

function Invoke-Checked {
    param([string]$Command, [string[]]$CommandArguments)
    & $Command @CommandArguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed with exit code $LASTEXITCODE."
    }
}

try {
    $QtPrefix = (Resolve-Path -LiteralPath $QtPrefix).Path
    $qtConfig = Join-Path $QtPrefix 'lib/cmake/Qt6/Qt6Config.cmake'
    if (-not (Test-Path -LiteralPath $qtConfig -PathType Leaf)) {
        throw 'QtPrefix must point to the Qt kit directory, such as C:/Qt/6.8.3/msvc2022_64.'
    }
    # CTest needs this kit's Qt DLLs; restore the caller's PATH when finished.
    $qtBin = Join-Path $QtPrefix 'bin'
    $env:PATH = "$qtBin;$originalPath"
    foreach ($tool in @('cmake', 'ctest')) {
        if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
            throw "$tool is required. Install the build prerequisites in README.md."
        }
    }

    $configureArgs = @('-S', $projectDir, '-B', $BuildDir,
        "-DCMAKE_BUILD_TYPE=$Configuration", '-DBUILD_TESTING=ON',
        "-DCMAKE_PREFIX_PATH=$QtPrefix")
    if ($Generator) {
        $configureArgs += @('-G', $Generator)
    }

    Invoke-Checked -Command 'cmake' -CommandArguments $configureArgs
    Invoke-Checked -Command 'cmake' -CommandArguments @(
        '--build', $BuildDir, '--config', $Configuration, '--parallel')
    Invoke-Checked -Command 'ctest' -CommandArguments @(
        '--test-dir', $BuildDir, '-C', $Configuration,
        '--output-on-failure', '--no-tests=error')

    Write-Host "Build and tests passed. Output directory: $BuildDir"
}
catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
finally {
    $env:PATH = $originalPath
}
