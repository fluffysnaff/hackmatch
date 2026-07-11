$ErrorActionPreference = 'Stop'

$repository = Split-Path -Parent $PSScriptRoot
$finder = Join-Path $PSScriptRoot 'find_redmatch.ps1'
$installer = Join-Path $PSScriptRoot 'install_enforcer.bat'
$builtDll = Join-Path $repository 'build-release\hackmatch.dll'
$sandbox = Join-Path ([System.IO.Path]::GetTempPath()) ("hackmatch-installer-" + [guid]::NewGuid())
$steam = Join-Path $sandbox 'Steam'
$library = Join-Path $sandbox 'Library With Spaces'
$game = Join-Path $library 'steamapps\common\Redmatch 2'
$data = Join-Path $game 'Redmatch 2_Data'
$destination = Join-Path $data 'enforcer.dll'

function Assert-True {
    param([bool] $Condition, [string] $Message)
    if (-not $Condition) {
        throw $Message
    }
}

function Invoke-Installer {
    param([string[]] $Arguments)

    $argumentLine = @('"' + $installer + '"', '--same-window') + $Arguments
    & cmd.exe /d /c (($argumentLine -join ' ') + ' 2>nul') | Out-Null
    return $LASTEXITCODE
}

try {
    Assert-True (Test-Path -LiteralPath $builtDll -PathType Leaf) 'Build hackmatch.dll before running installer fixtures.'

    New-Item -ItemType Directory -Path $data -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $steam 'steamapps') -Force | Out-Null
    New-Item -ItemType File -Path (Join-Path $game 'Redmatch 2.exe') | Out-Null

    $unrelatedGameFile = Join-Path $game 'settings.dat'
    $unrelatedDataFile = Join-Path $data 'sharedassets0.assets'
    Set-Content -LiteralPath $unrelatedGameFile -Value 'keep-game-file' -NoNewline
    Set-Content -LiteralPath $unrelatedDataFile -Value 'keep-data-file' -NoNewline

    $escapedLibrary = $library -replace '\\', '\\'
    @"
"libraryfolders"
{
    "0"
    {
        "path" "$escapedLibrary"
        "apps" { "1280770" "1" }
    }
}
"@ | Set-Content -LiteralPath (Join-Path $steam 'steamapps\libraryfolders.vdf')
    @"
"AppState"
{
    "appid" "1280770"
    "name" "Redmatch 2"
    "installdir" "Redmatch 2"
}
"@ | Set-Content -LiteralPath (Join-Path $library 'steamapps\appmanifest_1280770.acf')

    $resolved = & $finder -ExplicitPath $data
    Assert-True ($LASTEXITCODE -eq 0) 'The finder rejected a valid data-directory path.'
    Assert-True ($resolved -eq $game) 'The finder did not normalize the data-directory path.'

    $steamResolved = & $finder -SteamRoot $steam
    Assert-True ($LASTEXITCODE -eq 0) 'The finder rejected a valid Steam library manifest.'
    Assert-True ($steamResolved -eq $game) 'Steam library discovery returned the wrong game path.'

    & $finder -ExplicitPath (Join-Path $sandbox 'missing') | Out-Null
    Assert-True ($LASTEXITCODE -eq 1) 'The finder accepted an invalid explicit path.'

    $dryRunExit = Invoke-Installer @('--dry-run', ('"' + $game + '"'))
    Assert-True ($dryRunExit -eq 0) 'Installer dry-run failed.'
    Assert-True (-not (Test-Path -LiteralPath $destination)) 'Dry-run created the destination DLL.'

    $installExit = Invoke-Installer @(('"' + $game + '"'))
    Assert-True ($installExit -eq 0) 'Installer fixture run failed.'
    Assert-True (Test-Path -LiteralPath $destination -PathType Leaf) 'The DLL was not installed.'
    Assert-True ((Get-FileHash -LiteralPath $destination).Hash -eq (Get-FileHash -LiteralPath $builtDll).Hash) 'Installed DLL content differs from the build artifact.'
    Assert-True ((Get-Content -LiteralPath $unrelatedGameFile -Raw) -eq 'keep-game-file') 'An unrelated game-root file changed.'
    Assert-True ((Get-Content -LiteralPath $unrelatedDataFile -Raw) -eq 'keep-data-file') 'An unrelated data file changed.'

    $lock = [System.IO.File]::Open($destination, 'Open', 'Read', 'None')
    try {
        $lockedExit = Invoke-Installer @(('"' + $game + '"'))
        Assert-True ($lockedExit -ne 0) 'Installer unexpectedly replaced a locked destination.'
    }
    finally {
        $lock.Dispose()
    }
    Assert-True (-not (Test-Path -LiteralPath ($destination + '.hackmatch.tmp'))) 'Installer left its temporary DLL after a locked-file failure.'
    Assert-True ((Get-FileHash -LiteralPath $destination).Hash -eq (Get-FileHash -LiteralPath $builtDll).Hash) 'Locked-file failure changed the installed DLL.'
    Assert-True ((Get-Content -LiteralPath $unrelatedGameFile -Raw) -eq 'keep-game-file') 'Locked-file failure changed an unrelated game-root file.'
    Assert-True ((Get-Content -LiteralPath $unrelatedDataFile -Raw) -eq 'keep-data-file') 'Locked-file failure changed an unrelated data file.'

    Write-Host 'Installer fixture tests passed.' -ForegroundColor Green
}
finally {
    if (Test-Path -LiteralPath $sandbox) {
        Remove-Item -LiteralPath $sandbox -Recurse -Force
    }
}
