param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '../build/coverage'),
    [string]$CollectorPath
)

$ErrorActionPreference = 'Stop'
$buildPath = (Resolve-Path -LiteralPath $BuildDirectory).Path
$testExecutable = Join-Path $buildPath 'HelloWorldTests.exe'
if (!(Test-Path -LiteralPath $testExecutable)) {
    throw 'Build HelloWorldTests with HELLOWORLD_TEST_COVERAGE=ON before collecting coverage.'
}

if (!$CollectorPath) {
    $collectorCommand = Get-Command Microsoft.CodeCoverage.Console.exe -ErrorAction SilentlyContinue
    if ($collectorCommand) {
        $CollectorPath = $collectorCommand.Source
    } else {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        if (Test-Path -LiteralPath $vswhere) {
            $CollectorPath = & $vswhere -latest -products '*' -find 'Common7/IDE/Extensions/Microsoft/CodeCoverage.Console/Microsoft.CodeCoverage.Console.exe' |
                Select-Object -First 1
        }
    }
}
if (!$CollectorPath -or !(Test-Path -LiteralPath $CollectorPath)) {
    throw 'Microsoft.CodeCoverage.Console.exe was not found. Supply its location with -CollectorPath.'
}

$settingsPath = Join-Path $PSScriptRoot '../tests/NotificationService.coverage.config'
$reportPath = Join-Path $buildPath 'NotificationService.cobertura.xml'
$testsPath = Join-Path $buildPath 'NotificationService.tests.xml'
$summaryPath = Join-Path $buildPath 'NotificationService.coverage-summary.md'
# Clear previous reports so a failed run cannot be mistaken for fresh coverage.
foreach ($report in @($reportPath, $testsPath, $summaryPath)) {
    if (Test-Path -LiteralPath $report) {
        Remove-Item -LiteralPath $report
    }
}

& $CollectorPath collect --settings $settingsPath --include-files $testExecutable `
    --output $reportPath --output-format cobertura -- `
    $testExecutable '[NotificationService]' --reporter junit --out $testsPath
if ($LASTEXITCODE -ne 0) {
    throw "Coverage collection failed with exit code $LASTEXITCODE."
}
if (!(Test-Path -LiteralPath $testsPath) -or !(Test-Path -LiteralPath $reportPath)) {
    throw 'The collector did not produce both test results and coverage; no percentage can be reported.'
}

[xml]$testResults = Get-Content -LiteralPath $testsPath -Raw
$suites = @($testResults.SelectNodes('//testsuite'))
if (!$suites.Count) {
    throw 'The test result file contains no test suites.'
}
foreach ($suite in $suites) {
    if ([int]$suite.failures -gt 0 -or [int]$suite.errors -gt 0) {
        throw 'NotificationService tests failed. See NotificationService.tests.xml.'
    }
}

[xml]$coverage = Get-Content -LiteralPath $reportPath -Raw
$classes = @($coverage.SelectNodes('//class') | Where-Object {
    $_.filename -match '(^|[\\/])NotificationService\.(cpp|h)$'
})
if (!$classes.Count) {
    throw 'No NotificationService source was measured. Check debug symbols and coverage settings.'
}

$rows = @('# NotificationService coverage', '', '| Source | Covered lines | Total lines | Line coverage |',
    '| --- | ---: | ---: | ---: |')
foreach ($file in ($classes | Group-Object filename)) {
    $lines = @{}
    foreach ($class in $file.Group) {
        foreach ($line in $class.SelectNodes('./lines/line')) {
            $lineNumber = [int]$line.number
            $lines[$lineNumber] = $lines[$lineNumber] -or ([int]$line.hits -gt 0)
        }
    }
    if (!$lines.Count) {
        throw "No executable lines were recorded for $($file.Name)."
    }
    $covered = @($lines.Values | Where-Object { $_ }).Count
    $percentage = (100.0 * $covered / $lines.Count).ToString('F2', [System.Globalization.CultureInfo]::InvariantCulture)
    $name = Split-Path -Leaf $file.Name
    $rows += "| $name | $covered | $($lines.Count) | $percentage% |"
}
$testCount = $testResults.SelectNodes('//testcase').Count
if (!$testCount) {
    throw 'The test result file contains no test cases.'
}
$rows += '', "NotificationService test cases passed: $testCount.", '',
    'Measured by Microsoft.CodeCoverage.Console with native instrumentation. Source filters include NotificationService.cpp and NotificationService.h.'
$rows | Set-Content -LiteralPath $summaryPath -Encoding utf8
$rows
