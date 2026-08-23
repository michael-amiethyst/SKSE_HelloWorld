param(
    [Parameter(Mandatory = $true)]
    [string] $SourceDirectory,

    [Parameter(Mandatory = $true)]
    [string] $BinaryDirectory,

    [Parameter(Mandatory = $true)]
    [string] $ModContentsDirectory,

    [Parameter(Mandatory = $true)]
    [string] $DotnetExecutable
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$dotnetDirectory = Split-Path -Parent $DotnetExecutable
$env:DOTNET_ROOT = $dotnetDirectory
$env:PATH = "$dotnetDirectory;$env:PATH"

$spriggitVersion = '0.40.0'
$papyrusCommit = 'ae23267fe8f8d9c1ebad69c00ff3acf794b8d0eb'
$mcmHelperCommit = '9df6b69348dea57b3247b9f812711384f1113bab'

$toolDirectory = Join-Path $BinaryDirectory 'asset-tools'
$downloadDirectory = Join-Path $toolDirectory 'downloads'
$generatedDirectory = Join-Path $BinaryDirectory 'generated-assets'
New-Item -ItemType Directory -Force -Path $downloadDirectory, $generatedDirectory | Out-Null

function Install-PinnedArchive {
    param(
        [string] $Name,
        [string] $Uri,
        [string] $Sha256,
        [string] $ArchiveName,
        [string] $Destination,
        [string] $ExpectedFile
    )

    if (Test-Path -LiteralPath $ExpectedFile -PathType Leaf) {
        return
    }

    $archive = Join-Path $downloadDirectory $ArchiveName
    if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) {
        Write-Host "Downloading $Name"
        Invoke-WebRequest -Uri $Uri -OutFile $archive
    }

    $stream = [IO.File]::OpenRead($archive)
    try {
        $hasher = [Security.Cryptography.SHA256]::Create()
        try {
            $actualHash = ($hasher.ComputeHash($stream) | ForEach-Object { $_.ToString('X2') }) -join ''
        }
        finally {
            $hasher.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }
    if ($actualHash -ne $Sha256) {
        throw "$Name archive hash mismatch. Expected $Sha256, received $actualHash. Delete '$archive' and retry."
    }

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Expand-Archive -LiteralPath $archive -DestinationPath $Destination -Force
    if (-not (Test-Path -LiteralPath $ExpectedFile -PathType Leaf)) {
        throw "$Name did not contain the expected file '$ExpectedFile'."
    }
}

$spriggitDirectory = Join-Path $toolDirectory "Spriggit-$spriggitVersion"
$spriggitExe = Join-Path $spriggitDirectory 'Spriggit.CLI.exe'
Install-PinnedArchive `
    -Name "Spriggit $spriggitVersion" `
    -Uri "https://github.com/Mutagen-Modding/Spriggit/releases/download/$spriggitVersion/SpriggitCLI.zip" `
    -Sha256 '9D10320CF77FF909CFE888BE60FA9DABF39F078996439569165385E8A0D6C3BB' `
    -ArchiveName "SpriggitCLI-$spriggitVersion.zip" `
    -Destination $spriggitDirectory `
    -ExpectedFile $spriggitExe

$capricaDirectory = Join-Path $toolDirectory 'Caprica-0.3.0a'
$capricaExe = Join-Path $capricaDirectory 'Caprica.exe'
Install-PinnedArchive `
    -Name 'Caprica 0.3.0a' `
    -Uri 'https://github.com/KrisV-777/Caprica/releases/download/0.3.0a/Caprica.zip' `
    -Sha256 '8BB87175ECF685FB40B07B8805415062157714109BAB0925BC0BB0933E4F549C' `
    -ArchiveName 'Caprica-0.3.0a.zip' `
    -Destination $capricaDirectory `
    -ExpectedFile $capricaExe

$papyrusDirectory = Join-Path $toolDirectory "PAPYRUS-$papyrusCommit"
$papyrusRoot = Join-Path $papyrusDirectory "PAPYRUS-$papyrusCommit"
$papyrusSource = Join-Path $papyrusRoot 'SRC'
$papyrusForm = Join-Path $papyrusSource 'Form.psc'
Install-PinnedArchive `
    -Name 'Skyrim Papyrus source stubs' `
    -Uri "https://github.com/IHateMyKite/PAPYRUS/archive/$papyrusCommit.zip" `
    -Sha256 'C78E7EFD422BD270745AF3A353D7C873D557027654F77D08CDD2F45E2CD475FC' `
    -ArchiveName "PAPYRUS-$papyrusCommit.zip" `
    -Destination $papyrusDirectory `
    -ExpectedFile $papyrusForm

$mcmHelperDirectory = Join-Path $toolDirectory "MCM-Helper-$mcmHelperCommit"
$mcmHelperRoot = Join-Path $mcmHelperDirectory "MCM-Helper-$mcmHelperCommit"
$mcmHelperSource = Join-Path $mcmHelperRoot 'scripts/public'
$mcmConfigBase = Join-Path $mcmHelperSource 'MCM_ConfigBase.psc'
Install-PinnedArchive `
    -Name 'MCM Helper Papyrus SDK' `
    -Uri "https://github.com/Exit-9B/MCM-Helper/archive/$mcmHelperCommit.zip" `
    -Sha256 '65D1112A69A3B6240B42C0B2B1BA40F048981D56CE80903536944E0832F271E9' `
    -ArchiveName "MCM-Helper-$mcmHelperCommit.zip" `
    -Destination $mcmHelperDirectory `
    -ExpectedFile $mcmConfigBase

$pexDirectory = Join-Path $generatedDirectory 'Scripts'
New-Item -ItemType Directory -Force -Path $pexDirectory | Out-Null
$papyrusImports = "$papyrusSource;$mcmHelperSource"
& $capricaExe `
    --game skyrim `
    --import $papyrusImports `
    --flags (Join-Path $SourceDirectory 'scripts/SkyrimPapyrusFlags.flg') `
    --output $pexDirectory `
    --ignorecwd `
    (Join-Path $SourceDirectory 'scripts/Source/HelloWorldMCM.psc')
if ($LASTEXITCODE -ne 0) {
    throw "Caprica failed with exit code $LASTEXITCODE."
}

$pluginPath = Join-Path $generatedDirectory 'HelloWorld.esp'
& $spriggitExe deserialize `
    --InputPath (Join-Path $SourceDirectory 'plugin/HelloWorld') `
    --OutputPath $pluginPath `
    --PackageName 'Spriggit.YAML.Skyrim' `
    --PackageVersion $spriggitVersion
if ($LASTEXITCODE -ne 0) {
    throw "Spriggit failed with exit code $LASTEXITCODE."
}

$stagedScripts = Join-Path $ModContentsDirectory 'Scripts'
$stagedConfig = Join-Path $ModContentsDirectory 'MCM/Config/HelloWorld'
New-Item -ItemType Directory -Force -Path $ModContentsDirectory, $stagedScripts, $stagedConfig | Out-Null
Copy-Item -LiteralPath $pluginPath -Destination (Join-Path $ModContentsDirectory 'HelloWorld.esp') -Force
Copy-Item -LiteralPath (Join-Path $pexDirectory 'HelloWorldMCM.pex') -Destination $stagedScripts -Force
Copy-Item -LiteralPath (Join-Path $SourceDirectory 'mcm/Config/HelloWorld/config.json') -Destination $stagedConfig -Force
Copy-Item -LiteralPath (Join-Path $SourceDirectory 'mcm/Config/HelloWorld/settings.ini') -Destination $stagedConfig -Force
