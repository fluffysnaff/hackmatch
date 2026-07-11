[CmdletBinding()]
param(
    [string] $ExplicitPath,
    [string] $SteamRoot
)

$ErrorActionPreference = 'Stop'
$GameAppId = '1280770'

function Resolve-GameRoot {
    param([string] $Candidate)

    if ([string]::IsNullOrWhiteSpace($Candidate)) {
        return $null
    }

    $cleanPath = $Candidate.Trim().Trim('"')
    try {
        $resolved = [System.IO.Path]::GetFullPath($cleanPath)
    }
    catch {
        return $null
    }

    if ((Split-Path -Leaf $resolved) -ieq 'Redmatch 2_Data') {
        $resolved = Split-Path -Parent $resolved
    }

    $executable = Join-Path $resolved 'Redmatch 2.exe'
    $dataDirectory = Join-Path $resolved 'Redmatch 2_Data'
    if ((Test-Path -LiteralPath $executable -PathType Leaf) -and
        (Test-Path -LiteralPath $dataDirectory -PathType Container)) {
        return $resolved
    }

    return $null
}

function Read-SteamLibraryPaths {
    param([string] $SteamRoot)

    $paths = [System.Collections.Generic.List[string]]::new()
    if ([string]::IsNullOrWhiteSpace($SteamRoot)) {
        return $paths
    }

    $paths.Add($SteamRoot)
    $libraryFile = Join-Path $SteamRoot 'steamapps\libraryfolders.vdf'
    if (-not (Test-Path -LiteralPath $libraryFile -PathType Leaf)) {
        return $paths
    }

    foreach ($line in Get-Content -LiteralPath $libraryFile) {
        if ($line -match '^\s*"path"\s+"(?<path>.+)"\s*$') {
            $libraryPath = $Matches.path -replace '\\\\', '\'
            if (-not $paths.Contains($libraryPath)) {
                $paths.Add($libraryPath)
            }
        }
    }

    return $paths
}

function Find-InSteamLibrary {
    param([string] $LibraryRoot)

    $steamApps = Join-Path $LibraryRoot 'steamapps'
    $manifest = Join-Path $steamApps "appmanifest_$GameAppId.acf"
    if (Test-Path -LiteralPath $manifest -PathType Leaf) {
        $installDirectory = 'Redmatch 2'
        foreach ($line in Get-Content -LiteralPath $manifest) {
            if ($line -match '^\s*"installdir"\s+"(?<name>.+)"\s*$') {
                $installDirectory = $Matches.name
                break
            }
        }

        $fromManifest = Resolve-GameRoot (Join-Path $steamApps "common\$installDirectory")
        if ($fromManifest) {
            return $fromManifest
        }
    }

    return Resolve-GameRoot (Join-Path $steamApps 'common\Redmatch 2')
}

if ($ExplicitPath) {
    $explicitResult = Resolve-GameRoot $ExplicitPath
    if ($explicitResult) {
        $explicitResult
        exit 0
    }
    exit 1
}

$steamRoots = [System.Collections.Generic.List[string]]::new()
if ($SteamRoot) {
    $steamRoots.Add($SteamRoot)
}
foreach ($registryPath in @(
    'HKCU:\Software\Valve\Steam',
    'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam',
    'HKLM:\SOFTWARE\Valve\Steam'
)) {
    try {
        $properties = Get-ItemProperty -LiteralPath $registryPath
        $candidate = $properties.SteamPath
        if (-not $candidate) {
            $candidate = $properties.InstallPath
        }
        if ($candidate -and -not $steamRoots.Contains($candidate)) {
            $steamRoots.Add($candidate)
        }
    }
    catch {
        # Steam may not be registered for this user or registry view.
    }
}

if (${env:ProgramFiles(x86)}) {
    $defaultSteam = Join-Path ${env:ProgramFiles(x86)} 'Steam'
    if (-not $steamRoots.Contains($defaultSteam)) {
        $steamRoots.Add($defaultSteam)
    }
}

foreach ($steamRoot in $steamRoots) {
    foreach ($libraryRoot in Read-SteamLibraryPaths $steamRoot) {
        $result = Find-InSteamLibrary $libraryRoot
        if ($result) {
            $result
            exit 0
        }
    }
}

exit 1
