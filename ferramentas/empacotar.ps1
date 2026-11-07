# Monta o pacote para os amigos: pacote\AbaixoDaCidadeDeLatao-v<versão>.zip (seção 14 da spec)
# Compila em Release, confere que não depende do Visual C++ Redistributable e testa o zip extraído.
# Uso: .\ferramentas\empacotar.ps1 1.4   (sem versão: pacote\AbaixoDaCidadeDeLatao-teste.zip, para testar)
param([string]$Versao = "")
$ErrorActionPreference = "Stop"
if ($Versao -and $Versao -notmatch '^\d+(\.\d+)+$') { "versão inválida: $Versao (use números, ex.: 1.4)"; exit 1 }
Set-Location (Split-Path $PSScriptRoot)
$nome = "AbaixoDaCidadeDeLatao"

& .\ferramentas\compilar.ps1 release | Out-Host
if ($LASTEXITCODE -ne 0) { "PACOTE NÃO GERADO: compilação ou testes falharam"; exit 1 }

# Pasta do pacote
$pasta = "pacote\$nome"
if (Test-Path pacote) { Remove-Item pacote -Recurse -Force }
New-Item -ItemType Directory -Force $pasta | Out-Null
Copy-Item build\Release\Roguelike.exe "$pasta\$nome.exe"
Copy-Item build\Release\SDL2.dll, build\Release\SDL2_mixer.dll $pasta
Copy-Item assets $pasta -Recurse  # da fonte: a pasta do build guarda arquivos que já saíram do jogo
Copy-Item assets\LICENCAS.txt $pasta
# O Bloco de Notas mostra os acentos com certeza quando o arquivo tem BOM
$guia = Get-Content docs\COMO_JOGAR.txt -Raw -Encoding UTF8
[IO.File]::WriteAllText("$PWD\$pasta\COMO_JOGAR.txt", $guia, (New-Object Text.UTF8Encoding $true))

# Sem runtime do Visual C++ (o exe e as DLLs usam /MT)
$dumpbin = Get-ChildItem "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" |
    Select-Object -First 1
$proibidas = @()
foreach ($arquivo in "$nome.exe", "SDL2.dll", "SDL2_mixer.dll") {
    $deps = & $dumpbin.FullName /dependents "$pasta\$arquivo" | Where-Object { $_ -match "^\s+\S+\.dll$" } | ForEach-Object { $_.Trim() }
    "$arquivo -> $($deps -join ', ')"
    $proibidas += $deps | Where-Object { $_ -match "^(VCRUNTIME|MSVCP|api-ms-win-crt-)" }
}
if ($proibidas) { "PACOTE NÃO GERADO: depende do runtime do Visual C++: $($proibidas -join ', ')"; exit 1 }
"dependências ok"

$zip = if ($Versao) { "pacote\$nome-v$Versao.zip" } else { "pacote\$nome-teste.zip" }
# Entrada por entrada com "/" (o Compress-Archive do PowerShell 5.1 grava caminhos com \, que outros programas estranham)
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$arquivoZip = [IO.Compression.ZipFile]::Open("$PWD\$zip", [IO.Compression.ZipArchiveMode]::Create)
try {
    $base = (Resolve-Path pacote).Path
    foreach ($f in Get-ChildItem $pasta -Recurse -File) {
        $entrada = $f.FullName.Substring($base.Length + 1).Replace("\", "/")
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($arquivoZip, $f.FullName, $entrada,
            [IO.Compression.CompressionLevel]::Optimal)
    }
} finally {
    $arquivoZip.Dispose()
}

# Teste de fumaça: o zip extraído em outra pasta roda a demo sozinho
$teste = Join-Path $env:TEMP "teste_pacote_$(Get-Random)"
Expand-Archive $zip $teste
$env:SDL_AUDIODRIVER = "dummy"
$jogo = Join-Path $teste "$nome\$nome.exe"
$p = Start-Process $jogo -ArgumentList "--demo", "12345", "--dados", (Join-Path $teste "dados") -Wait -PassThru
$capturas = @(Get-ChildItem (Join-Path $teste "$nome\capturas") -Filter *.bmp -ErrorAction SilentlyContinue).Count
Remove-Item $teste -Recurse -Force
if ($p.ExitCode -ne 0 -or $capturas -eq 0) { "TESTE DE FUMAÇA FALHOU (saída $($p.ExitCode), $capturas capturas)"; exit 1 }
"teste de fumaça ok ($capturas capturas)"
"$zip  $([math]::Round((Get-Item $zip).Length / 1MB, 1)) MB"
