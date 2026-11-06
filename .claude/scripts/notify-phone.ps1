# Claude の返答が終わったとき（Stop フック）に、ntfy.sh 経由でスマホへ通知する。
#
# トピック名（スマホの ntfy アプリで購読する名前）はリポジトリに入れない（公開リポジトリのため）。
#   1. このPCの %APPDATA%\claude-notify\ntfy-topic.txt
#   2. 1 がなければ、個人用 OneDrive の 01_ClaudeSessions/ntfy-topic.txt を rclone で取ってきて 1 に保存
#   3. どちらにもなければ、推測されにくい名前を作って 1 に保存し、OneDrive にも置く（ほかのPCで使い回すため）
#
# ntfy.sh は公開サービスで、トピック名を知っていれば誰でも読めるので、通知の文面には作業の内容を書かない。
#
# 使い方:
#   -Mode notify : 通知を送る（フックから呼ぶ）
#   -Mode topic  : トピック名を表示する（スマホで購読するときに手で実行）

param([ValidateSet('notify', 'topic')][string]$Mode = 'notify')

$ErrorActionPreference = 'Continue'

$RemoteFile = 'claude-personal:01_ClaudeSessions/ntfy-topic.txt'
$LocalDir   = Join-Path $env:APPDATA 'claude-notify'
$LocalFile  = Join-Path $LocalDir 'ntfy-topic.txt'
$PathFile   = Join-Path $env:APPDATA 'claude-sessions\rclone-path.txt'

function Find-Rclone {
    if (Test-Path $PathFile) {
        $saved = (Get-Content $PathFile -Raw).Trim()
        if ($saved -and (Test-Path $saved)) { return $saved }
    }
    $cmd = Get-Command rclone.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $link = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Links\rclone.exe'
    if (Test-Path $link) { return $link }
    $pkg = Get-ChildItem (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages') -Directory -Filter 'Rclone.Rclone*' -ErrorAction SilentlyContinue |
           Get-ChildItem -Recurse -Filter rclone.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($pkg) { return $pkg.FullName }
    return $null
}

function Save-Local([string]$topic) {
    New-Item -ItemType Directory -Force $LocalDir | Out-Null
    [IO.File]::WriteAllText($LocalFile, $topic)
}

function Get-Topic {
    if (Test-Path $LocalFile) {
        $t = (Get-Content $LocalFile -Raw).Trim()
        if ($t) { return $t }
    }
    $rclone = Find-Rclone
    if ($rclone) {
        $t = (& $rclone cat $RemoteFile 2>$null | Out-String).Trim()
        if ($LASTEXITCODE -eq 0 -and $t) { Save-Local $t; return $t }
    }
    # どこにもないので新しく作る。英数字だけの長い名前にして、推測されにくくする。
    $chars = 'abcdefghijkmnpqrstuvwxyz23456789'.ToCharArray()
    $bytes = New-Object byte[] 24
    [Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
    $t = 'claude-' + (-join ($bytes | ForEach-Object { $chars[$_ % $chars.Length] }))
    Save-Local $t
    if ($rclone) { $t | & $rclone rcat $RemoteFile 2>$null }
    return $t
}

$topic = Get-Topic

if ($Mode -eq 'topic') {
    Write-Output $topic
    exit 0
}

$body = "Claude の作業が終わりました（$env:COMPUTERNAME）"
try {
    Invoke-RestMethod -Uri "https://ntfy.sh/$topic" -Method Post `
        -Body ([Text.Encoding]::UTF8.GetBytes($body)) `
        -ContentType 'text/plain; charset=utf-8' `
        -Headers @{ Title = 'Action'; Tags = 'white_check_mark' } `
        -TimeoutSec 10 | Out-Null
} catch {
    # 通知に失敗しても作業は止めない。
}
exit 0
