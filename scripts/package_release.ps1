param(
    [string]$OutputPath = 'out\hackmatch-win64.zip'
)

$ErrorActionPreference = 'Stop'
$repository = Split-Path -Parent $PSScriptRoot
if (-not [IO.Path]::IsPathRooted($OutputPath)) {
    $OutputPath = Join-Path $repository $OutputPath
}

$files = @(
    @{ Source = 'build-release\hackmatch.dll'; Destination = 'hackmatch.dll' },
    @{ Source = 'install.bat'; Destination = 'install.bat' },
    @{ Source = 'scripts\find_redmatch.ps1'; Destination = 'scripts\find_redmatch.ps1' },
    @{ Source = 'scripts\install_enforcer.bat'; Destination = 'scripts\install_enforcer.bat' },
    @{ Source = 'LICENSE'; Destination = 'LICENSE' },
    @{ Source = 'NOTICE'; Destination = 'NOTICE' },
    @{ Source = 'README.md'; Destination = 'README.md' }
)

$staging = Join-Path ([IO.Path]::GetTempPath()) ("hackmatch-release-{0}" -f [guid]::NewGuid())
try {
    New-Item -ItemType Directory -Path $staging | Out-Null
    foreach ($file in $files) {
        $source = Join-Path $repository $file.Source
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "Missing release file: $($file.Source)"
        }
        $destination = Join-Path $staging $file.Destination
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $source -Destination $destination
    }

    New-Item -ItemType Directory -Path (Split-Path -Parent $OutputPath) -Force | Out-Null
    Remove-Item -LiteralPath $OutputPath -Force -ErrorAction SilentlyContinue
    Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $OutputPath -CompressionLevel Optimal

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead($OutputPath)
    try {
        $actual = @($archive.Entries | Where-Object { $_.Name } | ForEach-Object { $_.FullName.Replace('\', '/') } | Sort-Object)
        $expected = @($files.Destination | ForEach-Object { $_.Replace('\', '/') } | Sort-Object)
        if (Compare-Object $expected $actual) {
            throw 'Release archive contents do not match the package manifest.'
        }
    } finally {
        $archive.Dispose()
    }
    Write-Output $OutputPath
} finally {
    Remove-Item -LiteralPath $staging -Recurse -Force -ErrorAction SilentlyContinue
}
