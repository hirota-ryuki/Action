# このリポジトリの Claude の会話ファイルを、個人用 OneDrive にバックアップ・共有する。
#
#   -Mode setup  : rclone を入れて個人用 OneDrive への許可を取る（PCごとに1回、手で実行）
#   -Mode pull   : 個人用 OneDrive から新しい会話を取ってくる（セッション開始時にフックが実行）
#   -Mode push   : 個人用 OneDrive へ会話を送る（やりとりの区切りでフックが実行）
#   -Mode restore: 別のPCで最近使っていた会話を、タブで開き直す（フォルダーを開いたときに .vscode/tasks.json が実行）
#   -Mode remove : このPCから rclone の設定と rclone 本体を消す（手で実行）
#   -Mode archive-apply : ほかのPCでのアーカイブを、このPCの VSCode に反映する（VSCode を閉じているときだけ）
#                         -Wait を付けると、VSCode が閉じるまで待ってから反映する（SessionEnd で自動で起動する）
#
# 会話ファイルの場所: <Claude の設定フォルダ>\projects\<リポジトリのパスを変換した名前>\
# 送り先          : 個人用 OneDrive の 01_ClaudeSessions\Action\（どのPCでも同じ）
# 送受信は「新しい方で上書きするコピー」だけで、どちらかで消したファイルを相手側で消すことはしない。
#
# タブの記録: Stop フック（やりとりの区切り）で、その会話の ID と時刻を
#   %APPDATA%\claude-sessions\tabs-Action\<PC名>.json に書き、個人用 OneDrive の 01_ClaudeSessions\Action-tabs\ へ送る。
#   restore では、いちばん最近使われたPCの記録から、最後のやりとりの $TabWindowHours 時間前までに使った会話を開く。
#
# アーカイブの記録: アーカイブしたかどうかは会話ファイルではなく、PCごとの VSCode の保存領域にある（vscode-state.ps1 を参照）。
#   push のたびに、このリポジトリの会話のアーカイブ状態が変わっていたら、変わった時刻と一緒に
#   %APPDATA%\claude-sessions\archive-Action\<PC名>.json に書き、個人用 OneDrive の 01_ClaudeSessions\Action-archive\ へ送る。
#   archive-apply では、全PCの記録から会話ごとにいちばん新しい状態を選び、このPCの VSCode に書き込む。
#   VSCode が開いている間は書き込めない（拡張が上書きする）ので、VSCode が閉じてから書き込む。

param(
    [ValidateSet('setup', 'pull', 'push', 'restore', 'remove', 'archive-apply')]
    [string]$Mode = 'pull',
    [string]$RepoDir = $env:CLAUDE_PROJECT_DIR,
    [double]$TabWindowHours = 3,
    [switch]$Wait
)

# フックから呼ばれたときは、標準入力にフックの情報（JSON）が来る。最初に読んでおく。
$hookInput = $null
if ([Console]::IsInputRedirected) {
    try { $hookInput = [Console]::In.ReadToEnd() | ConvertFrom-Json } catch { $hookInput = $null }
}

# 'Stop' にすると、rclone や winget が標準エラーに出す通知だけで止まってしまう（PowerShell 5.1）。
# 失敗は終了コードを見て throw する。
$ErrorActionPreference = 'Continue'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$tag = '[会話バックアップ]'

$RemoteName = 'claude-personal'
$WingetId   = 'Rclone.Rclone'

if (-not $RepoDir) { $RepoDir = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path }
$RepoDir    = $RepoDir.TrimEnd('\', '/')
$configDir  = if ($env:CLAUDE_CONFIG_DIR) { $env:CLAUDE_CONFIG_DIR } else { Join-Path $env:USERPROFILE '.claude' }
$LocalDir   = Join-Path $configDir ('projects\' + ($RepoDir -replace '[^A-Za-z0-9]', '-'))
# 送り先はリポジトリ名で固定する。クローン先のフォルダ名（Action / 01_Action など）はPCごとに違うことがあるため。
$RemotePath = "${RemoteName}:01_ClaudeSessions/Action"

# setup でインストール先を選んだPCでは、その場所をここに覚えておく（リポジトリには入れない）。
$PathFile = Join-Path $env:APPDATA 'claude-sessions\rclone-path.txt'

# タブの記録（どの会話をいつ使ったか）。PCごとに別のファイルにして、取り合いにならないようにする。
$RemoteTabs   = "${RemoteName}:01_ClaudeSessions/Action-tabs"
$LocalTabs    = Join-Path $env:APPDATA 'claude-sessions\tabs-Action'
$PcName       = $env:COMPUTERNAME
$MyTabFile    = Join-Path $LocalTabs "$PcName.json"
# restore で最後に開いた記録（同じ記録で何度も開かないため）
$RestoredFile = Join-Path $env:APPDATA 'claude-sessions\restored-Action.txt'

# アーカイブの記録。これもPCごとに別のファイル。
$RemoteArchive = "${RemoteName}:01_ClaudeSessions/Action-archive"
$LocalArchive  = Join-Path $env:APPDATA 'claude-sessions\archive-Action'
$MyArchiveFile = Join-Path $LocalArchive "$PcName.json"

. (Join-Path $PSScriptRoot 'vscode-state.ps1')

function Get-NowUnix { return [int64]([DateTimeOffset]::UtcNow.ToUnixTimeSeconds()) }

# 1つのPCのタブ記録を読む。返り値: @{ 会話ID = 使った時刻(Unix秒) }
function Read-TabFile([string]$path) {
    $map = @{}
    if (-not (Test-Path $path)) { return $map }
    try {
        $obj = Get-Content $path -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($p in $obj.sessions.PSObject.Properties) { $map[$p.Name] = [int64]$p.Value }
    } catch { }
    return $map
}

# この会話を「今使った」と記録する。7日より古い記録は消す。
function Write-TabRecord([string]$sessionId) {
    if ($sessionId -notmatch '^[0-9a-fA-F-]{36}$') { return }
    $now = Get-NowUnix
    $map = Read-TabFile $MyTabFile
    $map[$sessionId] = $now
    $keep = [ordered]@{}
    foreach ($k in $map.Keys) { if ($now - $map[$k] -lt 7 * 86400) { $keep[$k] = $map[$k] } }
    New-Item -ItemType Directory -Force -Path $LocalTabs | Out-Null
    $json = [ordered]@{ pc = $PcName; sessions = $keep } | ConvertTo-Json -Depth 3
    [System.IO.File]::WriteAllText($MyTabFile, $json, (New-Object System.Text.UTF8Encoding $false))
}

function Find-Rclone {
    if (Test-Path $PathFile) {
        $saved = (Get-Content $PathFile -Raw).Trim()
        if ($saved -and (Test-Path $saved)) { return $saved }
    }
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
    # 設定ファイルがないと rclone が通知を出すので、空のファイルを先に作っておく。
    $conf = Join-Path $env:APPDATA 'rclone\rclone.conf'
    if (-not (Test-Path $conf)) { New-Item -ItemType File -Force -Path $conf | Out-Null }
    # 未設定でも config show は終了コード 0 を返すので、listremotes で有無を見る。
    $remotes = & $rclone listremotes 2>$null
    if (-not ($remotes | Where-Object { $_.Trim() -eq "${RemoteName}:" })) { return $null }
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

# ---- アーカイブの記録 ----

# 1つのPCのアーカイブ記録を読む。返り値: @{ 会話ID = @{ archived = $true/$false; at = 変わった時刻(Unix秒) } }
function Read-ArchiveFile([string]$path) {
    $map = @{}
    if (-not (Test-Path $path)) { return $map }
    try {
        $obj = Get-Content $path -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($p in $obj.sessions.PSObject.Properties) {
            $map[$p.Name] = @{ archived = [bool]$p.Value.archived; at = [int64]$p.Value.at }
        }
    } catch { }
    return $map
}

function Write-ArchiveFile($map) {
    $sessions = [ordered]@{}
    foreach ($k in ($map.Keys | Sort-Object)) {
        $sessions[$k] = [ordered]@{ archived = $map[$k].archived; at = $map[$k].at }
    }
    New-Item -ItemType Directory -Force -Path $LocalArchive | Out-Null
    $json = [ordered]@{ pc = $PcName; sessions = $sessions } | ConvertTo-Json -Depth 4
    [System.IO.File]::WriteAllText($MyArchiveFile, $json, (New-Object System.Text.UTF8Encoding $false))
}

# このPCにある、このリポジトリの会話の ID
function Get-LocalSessionIds {
    $set = New-Object 'System.Collections.Generic.HashSet[string]'
    foreach ($f in Get-ChildItem $LocalDir -Filter *.jsonl -ErrorAction SilentlyContinue) { [void]$set.Add($f.BaseName) }
    return ,$set
}

# VSCode の保存領域から、アーカイブしている会話の ID と、アーカイブを戻した時刻(Unix秒)を取り出す
function Get-ArchiveState($ext) {
    $hidden = New-Object 'System.Collections.Generic.HashSet[string]'
    $unarchivedAt = @{}
    if ($ext.State.ContainsKey('hiddenSessionIds')) {
        foreach ($id in @($ext.State['hiddenSessionIds'])) { if ($id) { [void]$hidden.Add([string]$id) } }
    }
    if ($ext.State.ContainsKey('sessionUnarchivedAt') -and $ext.State['sessionUnarchivedAt'] -is [System.Collections.IDictionary]) {
        foreach ($k in $ext.State['sessionUnarchivedAt'].Keys) {
            try { $unarchivedAt[$k] = [int64]([double]$ext.State['sessionUnarchivedAt'][$k] / 1000) } catch { }
        }
    }
    return @{ Hidden = $hidden; UnarchivedAt = $unarchivedAt }
}

# このPCのアーカイブ状態を見て、前回の記録から変わった会話だけ時刻を付けて記録し直す。返り値: 記録
function Update-ArchiveRecord($ext) {
    $now = Get-NowUnix
    $st = Get-ArchiveState $ext
    $map = Read-ArchiveFile $MyArchiveFile
    $changed = $false
    foreach ($id in (Get-LocalSessionIds)) {
        $cur = $st.Hidden.Contains($id)
        $prev = $map[$id]
        if (-not $prev) {
            # 記録がなく、アーカイブもしていない会話は書かない（何もしていない＝アーカイブしていない）
            if ($cur) { $map[$id] = @{ archived = $true; at = $now }; $changed = $true }
        } elseif ($prev.archived -ne $cur) {
            $at = $now
            if (-not $cur -and $st.UnarchivedAt.ContainsKey($id) -and $st.UnarchivedAt[$id] -gt $prev.at) { $at = $st.UnarchivedAt[$id] }
            $map[$id] = @{ archived = $cur; at = $at }
            $changed = $true
        }
    }
    if ($changed -or -not (Test-Path $MyArchiveFile)) { Write-ArchiveFile $map }
    return $map
}

# push のついでに、このPCのアーカイブ状態を記録して送る（VSCode が開いていても読むだけなのでよい）
function Push-ArchiveRecord([string]$rclone) {
    try {
        $ext = Get-ClaudeExtensionState
        if (-not $ext) { return }
        [void](Update-ArchiveRecord $ext)
        & $rclone copyto $MyArchiveFile "$RemoteArchive/$PcName.json" --retries 2 --low-level-retries 3 --log-level ERROR
    } catch {
        Write-Output "$tag アーカイブの記録: $($_.Exception.Message)"
    }
}

# 全PCの記録を合わせて、このPCの VSCode に書き込む。VSCode が開いていたら何もしない。
function Invoke-ArchiveApply([string]$rclone) {
    if (Test-VscodeRunning) { return }
    New-Item -ItemType Directory -Force -Path $LocalArchive | Out-Null
    # 自分の記録は自分のPCの方が正しいので、取ってこない。フォルダーがまだないときの失敗は無視する。
    & $rclone copy $RemoteArchive $LocalArchive --update --exclude "$PcName.json" --retries 2 --low-level-retries 3 --log-level ERROR 2>$null

    $ext = Get-ClaudeExtensionState
    if (-not $ext) { return }
    $mine = Update-ArchiveRecord $ext

    # 会話ごとに、いちばん新しい記録を選ぶ（同じ時刻ならこのPCの記録を優先）
    $winner = @{}
    foreach ($id in $mine.Keys) { $winner[$id] = $mine[$id] }
    foreach ($f in Get-ChildItem $LocalArchive -Filter *.json -ErrorAction SilentlyContinue) {
        if ($f.BaseName -eq $PcName) { continue }
        $other = Read-ArchiveFile $f.FullName
        foreach ($id in $other.Keys) {
            if (-not $winner.ContainsKey($id) -or $other[$id].at -gt $winner[$id].at) { $winner[$id] = $other[$id] }
        }
    }

    $st = Get-ArchiveState $ext
    $local = Get-LocalSessionIds
    $toArchive = @(); $toUnarchive = @()
    foreach ($id in @($winner.Keys)) {
        if (-not $local.Contains($id)) { continue }   # 会話ファイルがまだ届いていないものは次の機会に
        $cur = $st.Hidden.Contains($id)
        if ($winner[$id].archived -and -not $cur) { $toArchive += $id }
        elseif (-not $winner[$id].archived -and $cur) { $toUnarchive += $id }
        $mine[$id] = $winner[$id]
    }

    if ($toArchive.Count -gt 0 -or $toUnarchive.Count -gt 0) {
        $list = New-Object 'System.Collections.Generic.List[object]'
        if ($ext.State.ContainsKey('hiddenSessionIds')) {
            foreach ($id in @($ext.State['hiddenSessionIds'])) { if ($id -and $toUnarchive -notcontains $id) { $list.Add($id) } }
        }
        foreach ($id in $toArchive) { $list.Add($id) }
        $ext.State['hiddenSessionIds'] = $list.ToArray()
        if ($toUnarchive.Count -gt 0) {
            # 戻した時刻も入れておく（拡張の「しばらく使っていない会話の自動アーカイブ」がすぐにアーカイブし直さないように）
            $ua = $null
            if ($ext.State.ContainsKey('sessionUnarchivedAt')) { $ua = $ext.State['sessionUnarchivedAt'] }
            if ($ua -isnot [System.Collections.IDictionary]) { $ua = New-Object 'System.Collections.Generic.Dictionary[string,object]' }
            foreach ($id in $toUnarchive) { $ua[$id] = [int64]$winner[$id].at * 1000 }
            $ext.State['sessionUnarchivedAt'] = $ua
        }
        if (Test-VscodeRunning) { return }   # 書く直前にもう一度確かめる
        Copy-Item $VscodeStateDb (Join-Path $env:APPDATA 'claude-sessions\state.vscdb.before-archive-apply') -Force
        Set-ClaudeExtensionState $ext
        Write-Output "$tag アーカイブを反映しました（アーカイブ $($toArchive.Count) 件、戻す $($toUnarchive.Count) 件）。"
    }
    Write-ArchiveFile $mine
    & $rclone copyto $MyArchiveFile "$RemoteArchive/$PcName.json" --retries 2 --low-level-retries 3 --log-level ERROR
}

# VSCode が閉じるのを待ってから反映する係を、裏で1つだけ動かす。
# VSCode と一緒に終わらされないように、WMI 経由で VSCode とは関係のないプロセスとして起動する。
function Start-ArchiveWaiter {
    $cmd = 'powershell.exe -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "{0}" -Mode archive-apply -Wait -RepoDir "{1}"' -f $PSCommandPath, $RepoDir
    try {
        [void](Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{ CommandLine = $cmd })
    } catch {
        Write-Output "$tag アーカイブ反映の待機を始められませんでした: $($_.Exception.Message)"
    }
}

switch ($Mode) {

    'setup' {
        $rclone = Find-Rclone
        if (-not $rclone) {
            Write-Output "$tag rclone のインストール先を選んでください（キャンセルすると標準の場所に入れます）。"
            $installDir = $null
            try {
                Add-Type -AssemblyName System.Windows.Forms
                $dlg = New-Object System.Windows.Forms.FolderBrowserDialog
                $dlg.Description = 'rclone のインストール先フォルダ（キャンセルで標準の場所）'
                $dlg.ShowNewFolderButton = $true
                if ($dlg.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) { $installDir = $dlg.SelectedPath }
            } catch {
                $installDir = Read-Host "$tag インストール先のフォルダ（空のまま Enter で標準の場所）"
            }

            Write-Output "$tag rclone をインストールします（winget）..."
            if ($installDir) {
                Write-Output "$tag インストール先: $installDir"
                winget install --id $WingetId -e --location $installDir --accept-source-agreements --accept-package-agreements
                $exe = Get-ChildItem $installDir -Recurse -Filter rclone.exe -ErrorAction SilentlyContinue | Select-Object -First 1
                if ($exe) {
                    New-Item -ItemType Directory -Force -Path (Split-Path $PathFile) | Out-Null
                    Set-Content -Path $PathFile -Value $exe.FullName -Encoding UTF8
                }
            } else {
                winget install --id $WingetId -e --accept-source-agreements --accept-package-agreements
            }
            $rclone = Find-Rclone
            if (-not $rclone) { throw 'rclone のインストール後に rclone.exe が見つかりません。' }
        }
        Write-Output "$tag rclone: $rclone"

        $type = Get-RemoteDriveType $rclone
        if ($null -eq $type) {
            Write-Output ''
            Write-Output "$tag ブラウザが開きます。【個人用】の Microsoft アカウントでサインインして「はい（許可）」を押してください。"
            Write-Output "$tag ※ 会社・学校のアカウントは選ばないでください。"
            # 1) ブラウザで許可を取り、トークンだけを受け取る（ここでは質問が出ない）
            & $rclone authorize onedrive 2>&1 | ForEach-Object { "$_" } | Tee-Object -Variable authOut | Out-Host
            $token = ([regex]::Match(($authOut -join "`n"), '\{.*\}')).Value
            if (-not $token) { throw '個人用 OneDrive の許可が完了しませんでした。' }
            # 2) 設定を作る。質問には自動で答える。
            #    ドライブは「OneDrive (personal)」を選ぶ（既定値は普段見えないシステム用のドライブのことがある）
            $out = & $rclone config create $RemoteName onedrive ('token=' + ($token -replace '"', '\"')) config_type=onedrive --non-interactive 2>$null
            for ($i = 0; $i -lt 10; $i++) {
                $q = ($out -join "`n") | ConvertFrom-Json
                if (-not $q.State) { break }
                $answer = $q.Option.DefaultStr
                if ($q.Option.Name -eq 'config_driveid') {
                    $pick = $q.Option.Examples | Where-Object { $_.Help -eq 'OneDrive (personal)' } | Select-Object -First 1
                    if (-not $pick) { $pick = $q.Option.Examples | Where-Object { $_.Value -notlike 'b!*' } | Select-Object -First 1 }
                    if ($pick) { $answer = $pick.Value }
                } elseif ($q.Option.Name -eq 'config_drive_ok') {
                    $answer = 'true'
                } elseif ($q.Option.Name -eq 'config_refresh_token') {
                    $answer = 'false'   # トークンは 1) で取ったので取り直さない
                }
                $out = & $rclone config update $RemoteName --continue --state $q.State --result $answer --non-interactive 2>$null
            }
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
        Write-Output "$tag 完了しました。会話は個人用 OneDrive の 01_ClaudeSessions に保存されます。"
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
            if ($Mode -eq 'pull') {
                Invoke-Copy $rclone $RemotePath $LocalDir
                # ターミナルの Claude など、VSCode を開いていないときはここで反映できる。
                # 開いているときは、閉じたあとに反映する係を起こしておく（VSCode を閉じると SessionEnd が来ないことがあるため）
                if (Test-VscodeRunning) { Start-ArchiveWaiter }
                else { try { Invoke-ArchiveApply $rclone } catch { Write-Output "$tag アーカイブの反映: $($_.Exception.Message)" } }
            } else {
                Invoke-Copy $rclone $LocalDir $RemotePath
                # 会話ファイルを送ったあとにタブの記録を送る（記録だけ先に届いて会話がない、を避ける）
                if ($hookInput -and $hookInput.hook_event_name -eq 'Stop' -and $hookInput.session_id) {
                    Write-TabRecord $hookInput.session_id
                }
                if (Test-Path $MyTabFile) {
                    & $rclone copyto $MyTabFile "$RemoteTabs/$PcName.json" --retries 2 --low-level-retries 3 --log-level ERROR
                }
                Push-ArchiveRecord $rclone
                # セッションを閉じたとき（VSCode を閉じたときも含む）は、VSCode が閉じたあとに反映する係を起こす
                if ($hookInput -and $hookInput.hook_event_name -eq 'SessionEnd') { Start-ArchiveWaiter }
            }
        } catch {
            Write-Output "$tag $($_.Exception.Message)"
        } finally {
            $lock.Dispose()
        }
    }

    'restore' {
        $rclone = Find-Rclone
        if (-not $rclone -or (Get-RemoteDriveType $rclone) -ne 'personal') { exit 0 }
        $lock = Enter-Lock
        if (-not $lock) { exit 0 }
        try {
            if (-not (Test-Path $LocalDir)) { New-Item -ItemType Directory -Force -Path $LocalDir | Out-Null }
            New-Item -ItemType Directory -Force -Path $LocalTabs | Out-Null
            Invoke-Copy $rclone $RemotePath $LocalDir
            # 記録のフォルダーがまだない（どのPCもまだ記録していない）ときは失敗するので、エラーは無視する
            & $rclone copy $RemoteTabs $LocalTabs --update --retries 2 --low-level-retries 3 --log-level ERROR 2>$null
        } catch {
            Write-Output "$tag $($_.Exception.Message)"
            exit 0
        } finally {
            $lock.Dispose()
        }

        # いちばん最近やりとりがあったPCを探す
        $latestPc = $null; $latestTime = [int64]0; $latestMap = $null
        foreach ($f in Get-ChildItem $LocalTabs -Filter *.json -ErrorAction SilentlyContinue) {
            $map = Read-TabFile $f.FullName
            foreach ($t in $map.Values) {
                if ($t -gt $latestTime) { $latestTime = $t; $latestPc = $f.BaseName; $latestMap = $map }
            }
        }
        if (-not $latestPc) { Write-Output "$tag タブの記録がまだありません。"; exit 0 }
        if ($latestPc -eq $PcName) {
            # 最後に使ったのがこのPCなら、VSCode が自分でタブを戻すので何もしない
            Write-Output "$tag 最後に使ったのはこのPC（$PcName）なので、タブは開きません。"
            exit 0
        }
        $marker = "$latestPc $latestTime"
        if ((Test-Path $RestoredFile) -and ((Get-Content $RestoredFile -Raw).Trim() -eq $marker)) {
            Write-Output "$tag $latestPc の記録はもう開いています。"
            exit 0
        }

        # そのPCで、最後のやりとりの $TabWindowHours 時間前までに使った会話（このPCに会話ファイルがあるものだけ）
        $from = $latestTime - [int64]($TabWindowHours * 3600)
        $ids = $latestMap.GetEnumerator() |
               Where-Object { $_.Value -ge $from -and (Test-Path (Join-Path $LocalDir "$($_.Key).jsonl")) } |
               Sort-Object Value | ForEach-Object { $_.Key }
        if (-not $ids) { Write-Output "$tag 開く会話がありません。"; exit 0 }

        # 別のPCでついさっきまで使っていたなら、同時に進めてしまわないように確認する
        $minutes = [int](((Get-NowUnix) - $latestTime) / 60)
        if ($minutes -lt 30) {
            Add-Type -AssemblyName System.Windows.Forms
            $msg = "$latestPc で $minutes 分前まで Claude の会話を使っていました。`n" +
                   "同じ会話を2台で同時に進めると、片方の続きが消えます。`n`n" +
                   "$latestPc の Claude を閉じてから「はい」を押してください。`n" +
                   "（「いいえ」で今回は開きません）"
            $ans = [System.Windows.Forms.MessageBox]::Show($msg, 'Claude のタブを開き直す', 'YesNo', 'Warning')
            if ($ans -ne [System.Windows.Forms.DialogResult]::Yes) { exit 0 }
        }

        foreach ($id in $ids) {
            Start-Process "vscode://anthropic.claude-code/open?session=$id"
            Start-Sleep -Milliseconds 800
        }
        Set-Content -Path $RestoredFile -Value $marker -Encoding UTF8
        Write-Output "$tag $latestPc で使っていた会話を $(@($ids).Count) 個開きました。"
    }

    'remove' {
        $rclone = Find-Rclone
        if ($rclone) {
            & $rclone config delete $RemoteName 2>$null | Out-Null
            Write-Output "$tag このPCから個人用 OneDrive への鍵（rclone の設定）を消しました。"
            winget uninstall --id $WingetId -e
            if (Test-Path $PathFile) { Remove-Item (Split-Path $PathFile) -Recurse -Force }
            Write-Output "$tag rclone をアンインストールしました。"
        } else {
            Write-Output "$tag このPCには rclone がありません。"
        }
        Write-Output ''
        Write-Output "$tag 全部のPCをまとめて止めるには、次のページで rclone の許可を取り消してください:"
        Write-Output '  https://account.live.com/consent/Manage'
        Write-Output "$tag 会話ファイル（このPCと個人用 OneDrive の 01_ClaudeSessions）は消していません。"
    }

    'archive-apply' {
        $rclone = Find-Rclone
        if (-not $rclone -or (Get-RemoteDriveType $rclone) -ne 'personal') { exit 0 }
        # 待つ係は1つだけにする
        $mutex = New-Object System.Threading.Mutex($false, ('Global\claude-archive-apply-' + ($RepoDir -replace '[^A-Za-z0-9]', '-')))
        if (-not $mutex.WaitOne(0)) { exit 0 }
        try {
            if ($Wait) {
                # VSCode が閉じるまで待つ（最長1日）。閉じかけで残るプロセスもあるので、2回続けていなければ閉じたとみなす
                $deadline = (Get-Date).AddDays(1); $quiet = 0
                while ($quiet -lt 2) {
                    if ((Get-Date) -gt $deadline) { exit 0 }
                    Start-Sleep -Seconds 10
                    if (Test-VscodeRunning) { $quiet = 0 } else { $quiet++ }
                }
            }
            $lock = $null
            for ($i = 0; $i -lt 30 -and -not $lock; $i++) { $lock = Enter-Lock; if (-not $lock) { Start-Sleep -Seconds 2 } }
            if (-not $lock) { exit 0 }
            try {
                Invoke-ArchiveApply $rclone
            } catch {
                Write-Output "$tag アーカイブの反映: $($_.Exception.Message)"
            } finally {
                $lock.Dispose()
            }
        } finally {
            $mutex.ReleaseMutex()
        }
    }
}
