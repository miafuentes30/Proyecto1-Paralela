$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$executable = Join-Path $repoRoot "build\seed-validation\fruitcat-chaos.exe"
$outputDirectory = Join-Path $repoRoot "resultados\benchmark-oficial"
$seed = 20260828
$frames = 120
$repetitions = 10
$catCounts = @(500, 1000, 2000)
$threadCounts = @(1, 2, 4, 8)
$invariantCulture = [System.Globalization.CultureInfo]::InvariantCulture

if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "No existe el ejecutable requerido: $executable"
}

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

# Las mediciones oficiales nunca se sobrescriben ni se mezclan con otro lote.
$existingOfficialFiles = @(Get-ChildItem -LiteralPath $outputDirectory -File -ErrorAction SilentlyContinue)
if ($existingOfficialFiles.Count -gt 0) {
    throw "Ya existen archivos oficiales en $outputDirectory. No se sobrescribira el lote anterior."
}

function ConvertTo-InvariantDouble {
    param([Parameter(Mandatory = $true)][string]$Value)

    return [double]::Parse($Value, $invariantCulture)
}

function Test-PositiveFinite {
    param([Parameter(Mandatory = $true)][string]$Value)

    $number = ConvertTo-InvariantDouble $Value
    return $number -gt 0.0 -and -not [double]::IsNaN($number) -and -not [double]::IsInfinity($number)
}

$summaryRows = @()
$approvedConfigurations = 0
$batchStart = Get-Date

foreach ($n in $catCounts) {
    foreach ($threads in $threadCounts) {
        $baseName = "benchmark_N${n}_T${threads}"
        $csvPath = Join-Path $outputDirectory "${baseName}.csv"
        $txtPath = Join-Path $outputDirectory "${baseName}.txt"

        Write-Host "Ejecutando N=$n, hilos=$threads..."
        & $executable $n benchmark $threads $seed $frames $repetitions $csvPath 2>&1 |
            Tee-Object -FilePath $txtPath
        if ($LASTEXITCODE -ne 0) {
            throw "Fallo la configuracion N=$n, hilos=$threads con codigo $LASTEXITCODE."
        }

        $textOutput = Get-Content -LiteralPath $txtPath -Raw
        if ($textOutput -notmatch "Equivalencia secuencial/paralela: OK" -or
            $textOutput -notmatch "BENCHMARK_OK") {
            throw "La salida no confirmo equivalencia o finalizacion para N=$n, hilos=$threads."
        }

        $rows = @(Import-Csv -LiteralPath $csvPath)
        $measurements = @($rows | Where-Object { $_.tipo -eq "medicion" })
        $averages = @($rows | Where-Object { $_.tipo -eq "promedio" })
        if ($measurements.Count -ne 10 -or $averages.Count -ne 1) {
            throw "Conteos CSV invalidos para N=$n, hilos=${threads}: mediciones=$($measurements.Count), promedios=$($averages.Count)."
        }

        foreach ($row in $rows) {
            $positiveFields = @(
                "tiempo_secuencial_ms",
                "tiempo_paralelo_ms",
                "tiempo_secuencial_ms_por_frame",
                "tiempo_paralelo_ms_por_frame",
                "fps_secuencial",
                "fps_paralelo",
                "speedup",
                "eficiencia_porcentaje"
            )
            foreach ($field in $positiveFields) {
                if (-not (Test-PositiveFinite $row.$field)) {
                    throw "Valor no positivo o no finito en $field para N=$n, hilos=$threads, tipo=$($row.tipo)."
                }
            }
            if ([int]$row.frames -ne $frames -or [int]$row.hilos -ne $threads) {
                throw "Frames o hilos incorrectos para N=$n, hilos=$threads, tipo=$($row.tipo)."
            }
        }

        $average = $averages[0]
        if ([uint32]$average.semilla -ne $seed -or [int]$average.n -ne $n -or
            [int]$average.repeticion -ne $repetitions) {
            throw "Metadatos de promedio incorrectos para N=$n, hilos=$threads."
        }

        # El resumen conserva literalmente las metricas calculadas por el benchmark.
        $summaryRows += [pscustomobject][ordered]@{
            n = $average.n
            hilos = $average.hilos
            semilla = $average.semilla
            frames = $average.frames
            repeticiones = $average.repeticion
            tiempo_secuencial_ms = $average.tiempo_secuencial_ms
            tiempo_paralelo_ms = $average.tiempo_paralelo_ms
            tiempo_secuencial_ms_por_frame = $average.tiempo_secuencial_ms_por_frame
            tiempo_paralelo_ms_por_frame = $average.tiempo_paralelo_ms_por_frame
            fps_secuencial = $average.fps_secuencial
            fps_paralelo = $average.fps_paralelo
            speedup = $average.speedup
            eficiencia_porcentaje = $average.eficiencia_porcentaje
        }
        $approvedConfigurations++
    }
}

$summaryPath = Join-Path $outputDirectory "resumen_benchmark.csv"
$summaryRows |
    Sort-Object @{ Expression = { [int]$_.n } }, @{ Expression = { [int]$_.hilos } } |
    Export-Csv -LiteralPath $summaryPath -NoTypeInformation

$individualCsvCount = @(Get-ChildItem -LiteralPath $outputDirectory -Filter "benchmark_N*_T*.csv" -File).Count
$individualTxtCount = @(Get-ChildItem -LiteralPath $outputDirectory -Filter "benchmark_N*_T*.txt" -File).Count
$summaryCount = @(Import-Csv -LiteralPath $summaryPath).Count
if ($individualCsvCount -ne 12 -or $individualTxtCount -ne 12 -or
    $summaryCount -ne 12 -or $approvedConfigurations -ne 12) {
    throw "Conteos finales invalidos: CSV=$individualCsvCount, TXT=$individualTxtCount, resumen=$summaryCount, aprobadas=$approvedConfigurations."
}

$elapsed = (Get-Date) - $batchStart
Write-Host "CSV individuales: $individualCsvCount"
Write-Host "TXT individuales: $individualTxtCount"
Write-Host "Filas del resumen: $summaryCount"
Write-Host "Configuraciones aprobadas: $approvedConfigurations/12"
Write-Host ("Tiempo total: {0:hh\:mm\:ss}" -f $elapsed)
