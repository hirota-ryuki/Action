# Claude の返答が終わったとき（Stop フック）に、ntfy.sh 経由でスマホへ通知する。
#
# トピック名（スマホの ntfy アプリで購読する名前）はリポジトリに入れない（公開リポジトリのため）。
#   1. このPCの %APPDATA%\claude-notify\ntfy-topic.txt
#   2. 1 がなければ、個人用 OneDrive の 01_ClaudeSessions/ntfy-topic.txt を rclone で取ってきて 1 に保存
#   3. どちらにもなければ、推測されにくい名前を作って 1 に保存し、OneDrive にも置く（ほかのPCで使い回すため）
#
# 通知の文面は、その回の Claude の返答の1行目（「〇〇を実装しました」のような結果の1文）にする。
# ntfy.sh は公開サービスで、トピック名を知っていれば誰でも読めるので、返答の1行目にパスワードなどを書かないこと。
#
# 使い方:
#   -Mode notify : 通知を送る（フックから呼ぶ）
#   -Mode topic  : トピック名を表示する（スマホで購読するときに手で実行）
#   -Topic <名前> : 送り先のトピックを変える。動作確認はこれで使い捨てのトピックへ送り、
#                   作業の途中でユーザーのスマホに通知が届かないようにする。

param([ValidateSet('notify', 'topic')][string]$Mode = 'notify', [string]$Topic = '')

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

$topic = if ($Topic) { $Topic } else { Get-Topic }

if ($Mode -eq 'topic') {
    Write-Output $topic
    exit 0
}

# フックから渡される JSON を読み、最後の返答の文章を取り出す。
function Get-LastReply {
    try {
        [Console]::InputEncoding = [Text.Encoding]::UTF8
        $raw = [Console]::In.ReadToEnd()
        if (-not $raw) { return $null }
        $in = $raw | ConvertFrom-Json
    } catch { return $null }
    if ($in.last_assistant_message) { return [string]$in.last_assistant_message }
    # 古い Claude Code では last_assistant_message がないので、会話ファイルの末尾から探す。
    if ($in.transcript_path -and (Test-Path $in.transcript_path)) {
        $lines = Get-Content $in.transcript_path -Tail 300 -Encoding UTF8
        for ($i = $lines.Count - 1; $i -ge 0; $i--) {
            try { $e = $lines[$i] | ConvertFrom-Json } catch { continue }
            if ($e.type -ne 'assistant' -or -not $e.message.content) { continue }
            $texts = @($e.message.content | Where-Object { $_.type -eq 'text' -and $_.text.Trim() } | ForEach-Object { $_.text })
            if ($texts.Count -gt 0) { return $texts[-1] }
        }
    }
    return $null
}

# 返答の最初の文を、Markdown の記号を外して1行にする。
function Get-Headline([string]$reply) {
    if (-not $reply) { return $null }
    foreach ($line in ($reply -split "`r?`n")) {
        $t = $line.Trim()
        if (-not $t -or $t.StartsWith('```')) { continue }
        $t = $t -replace '\*\*|`', ''
        $t = $t -replace '^(#+\s*|[-*>]\s+|\d+\.\s+)', ''
        $t = $t -replace '\[([^\]]*)\]\([^)]*\)', '$1'
        $t = $t.Trim()
        if (-not $t) { continue }
        # 1行目が長いときは、最初の「。」までにする。
        $dot = $t.IndexOf('。')
        if ($dot -ge 0 -and $dot -lt $t.Length - 1) { $t = $t.Substring(0, $dot + 1) }
        if ($t.Length -gt 120) { $t = $t.Substring(0, 119) + '…' }
        return $t
    }
    return $null
}

$body = Get-Headline (Get-LastReply)
if (-not $body) { $body = 'Claude の返答が終わりました' }
# どのフォルダー（Action / Action-try など）の Claude が終わったか分かるように、題名にフォルダー名を入れる。
$folder = if ($env:CLAUDE_PROJECT_DIR) { Split-Path $env:CLAUDE_PROJECT_DIR -Leaf } else { 'Action' }
$title = "$folder（$env:COMPUTERNAME）"
# ヘッダーには日本語をそのまま入れられないので、RFC 2047 の形にする（ntfy が対応している）。
$titleEnc = '=?UTF-8?B?' + [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($title)) + '?='
try {
    Invoke-RestMethod -Uri "https://ntfy.sh/$topic" -Method Post `
        -Body ([Text.Encoding]::UTF8.GetBytes($body)) `
        -ContentType 'text/plain; charset=utf-8' `
        -Headers @{ Title = $titleEnc; Tags = 'white_check_mark' } `
        -TimeoutSec 10 | Out-Null
} catch {
    # 通知に失敗しても作業は止めない。
}
exit 0
