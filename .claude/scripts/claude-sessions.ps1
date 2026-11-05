# このリポジトリの Claude の会話ファイルを、個人用 OneDrive にバックアップ・共有する。
#
#   -Mode setup  : rclone を入れて個人用 OneDrive への許可を取る（PCごとに1回、手で実行）
#   -Mode pull   : 個人用 OneDrive から新しい会話を取ってくる（セッション開始時にフックが実行）
#   -Mode push   : 個人用 OneDrive へ会話を送る（やりとりの区切りでフックが実行）
#   -Mode remove : このPCから rclone の設定と rclone 本体を消す（手で実行）
#
# 会話ファイルの場所: <Claude の設定フォルダ>\projects\<リポジトリのパスを変換した名前>\
# 送り先          : 個人用 OneDrive の 12_ClaudeSessions\<リポジトリのフォルダ名>\
# 送受信は「新しい方で上書きするコピー」だけで、どちらかで消したファイルを相手側で消すことはしない。

param(
    [ValidateSet('setup', 'pull', 'push', 'remove')]
    [string]$Mode = 'pull',
    [string]$RepoDir = $env:CLAUDE_PROJECT_DIR
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$tag = '[会話バックアップ]'

$RemoteName = 'claude-personal'
$WingetId   = 'Rclone.Rclone'

if (-not $RepoDir) { $RepoDir = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path }
$RepoDir    = $RepoDir.TrimEnd('\', '/')
$configDir  = if ($env:CLAUDE_CONFIG_DIR) { $env:CLAUDE_CONFIG_DIR } else { Join-Path $env:USERPROFILE '.claude' }
$LocalDir   = Join-Path $configDir ('projects\' + ($RepoDir -replace '[^A-Za-z0-9]', '-'))
$RemotePath = "${RemoteName}:12_ClaudeSessions/" + (Split-Path $RepoDir -Leaf)

function Find-Rclone {
    $cmd = Get-Command rclone.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $link = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Links\rclone.exe'
    if (Test-Path $link) { return $link }
    $pkg = Get-ChildItem (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages') -Directory -Filter "$WingetId*" -ErrorAction SilentlyContinue |
           Get-ChildItem -Recurse -Filter rclone.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($pkg) { return $pkg.FullName }
    return $null
}

function Get-RemoteDriveType([string]$rclone) {
    $text = & $rclone config show $RemoteName 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $text) { return $null }
    $line = $text | Where-Object { $_ -match '^\s*drive_type\s*=' } | Select-Object -First 1
    if ($line) { return ($line -split '=', 2)[1].Trim() }
    return ''
}

function Invoke-Copy([string]$rclone, [string]$from, [string]$to) {
    & $rclone copy $from $to --update --retries 2 --low-level-retries 3 --log-level ERROR
    if ($LASTEXITCODE -ne 0) { throw "rclone copy に失敗しました（$LASTEXITCODE）: $from -> $to" }
}

# 複数のタブが同時に送受信しないようにするロック。取れなければ今回は何もしない。
function Enter-Lock {
    $lockPath = Join-Path $env:TEMP ('claude-sessions-' + ($RepoDir -replace '[^A-Za-z0-9]', '-') + '.lock')
    try { return [System.IO.File]::Open($lockPath, 'OpenOrCreate', 'ReadWrite', 'None') } catch { return $null }
}

switch ($Mode) {

    'setup' {
        $rclone = Find-Rclone
        if (-not $rclone) {
            Write-Output "$tag rclone をインストールします（winget）..."
            winget install --id $WingetId -e --accept-source-agreements --accept-package-agreements
            $rclone = Find-Rclone
            if (-not $rclone) { throw 'rclone のインストール後に rclone.exe が見つかりません。' }
        }
        Write-Output "$tag rclone: $rclone"

        $type = Get-RemoteDriveType $rclone
        if ($null -eq $type) {
            Write-Output ''
            Write-Output "$tag ブラウザが開きます。【個人用】の Microsoft アカウントでサインインして「はい（許可）」を押してください。"
            Write-Output "$tag ※ 会社・学校のアカウントは選ばないでください。"
            & $rclone config create $RemoteName onedrive config_type=onedrive
            if ($LASTEXITCODE -ne 0) { throw '個人用 OneDrive の許可が完了しませんでした。' }
            $type = Get-RemoteDriveType $rclone
        }
        if ($type -ne 'personal') {
            & $rclone config delete $RemoteName | Out-Null
            throw "許可したアカウントが個人用ではありません（drive_type=$type）。設定を消しました。個人用アカウントでやり直してください。"
        }

        & $rclone mkdir $RemotePath
        if (-not (Test-Path $LocalDir)) { New-Item -ItemType Directory -Force -Path $LocalDir | Out-Null }
        Invoke-Copy $rclone $RemotePath $LocalDir
        Invoke-Copy $rclone $LocalDir $RemotePath
        Write-Output ''
        Write-Output "$tag 完了しました。会話は個人用 OneDrive の 12_ClaudeSessions に保存されます。"
        Write-Output "$tag VSCode の Claude を開き直すと、過去の会話一覧に他のPCの会話も出ます。"
    }

    { $_ -in 'pull', 'push' } {
        $rclone = Find-Rclone
        if (-not $rclone -or (Get-RemoteDriveType $rclone) -ne 'personal') {
            if ($Mode -eq 'pull') {
                Write-Output "$tag このPCでは個人用 OneDrive へのバックアップがまだ設定されていません。使う場合は .claude\scripts\setup-sessions.cmd を1回実行してください。"
            }
            exit 0
        }
        $lock = Enter-Lock
        if (-not $lock) { exit 0 }
        try {
            if (-not (Test-Path $LocalDir)) { New-Item -ItemType Directory -Force -Path $LocalDir | Out-Null }
            if ($Mode -eq 'pull') { Invoke-Copy $rclone $RemotePath $LocalDir }
            else                  { Invoke-Copy $rclone $LocalDir $RemotePath }
        } catch {
            Write-Output "$tag $($_.Exception.Message)"
        } finally {
            $lock.Dispose()
        }
    }

    'remove' {
        $rclone = Find-Rclone
        if ($rclone) {
            & $rclone config delete $RemoteName 2>$null | Out-Null
            Write-Output "$tag このPCから個人用 OneDrive への鍵（rclone の設定）を消しました。"
            winget uninstall --id $WingetId -e
            Write-Output "$tag rclone をアンインストールしました。"
        } else {
            Write-Output "$tag このPCには rclone がありません。"
        }
        Write-Output ''
        Write-Output "$tag 全部のPCをまとめて止めるには、次のページで rclone の許可を取り消してください:"
        Write-Output '  https://account.live.com/consent/Manage'
        Write-Output "$tag 会話ファイル（このPCと個人用 OneDrive の 12_ClaudeSessions）は消していません。"
    }
}
