# フォルダーを VSCode で開いたときに、リモートの最新を取り込んでから Game.sln を開く。
# .vscode/tasks.json が、フォルダーを開いたとき（runOn: folderOpen）に実行する。
#
#   1. git fetch --all --prune でリモートの最新を取ってくる
#   2. ローカルのブランチを、リモートに追いつかせる（早送りできるものだけ。ブランチは切り替えない）
#        - 開いているフォルダーのブランチ        : git merge --ff-only（変更中のファイルとぶつかるなら何もしない）
#        - ほかの worktree で開いているブランチ : その worktree で git merge --ff-only
#        - どこでも開いていないブランチ          : ブランチの指す先だけを進める
#        - ローカルの方が先に進んでいる・分かれている : 何もしない（一覧に出すだけ）
#   3. リモートにしかないブランチは、同じ名前で追いかけるローカルのブランチを作る
#   4. Action/Game/Game.sln を開く（Visual Studio ですでに開いていれば開かない）
#
# 取り込めなくても（オフラインなど）、sln は開く。結果は VSCode のターミナルの「Pull して sln を開く」に出る。

param(
    [string]$RepoDir = (Resolve-Path "$PSScriptRoot\..\..").Path,
    [switch]$NoOpen
)

# 'Stop' にすると、git が標準エラーに出す進み具合だけで止まってしまう（PowerShell 5.1）。終了コードを見る。
$ErrorActionPreference = 'Continue'
[Console]::OutputEncoding = [Text.Encoding]::UTF8

$results = New-Object System.Collections.Generic.List[string]
function Add-Result([string]$text) { $results.Add($text); Write-Host $text }

function Invoke-Git {
    # 標準エラーも文字列にして返す。終了コードは $LASTEXITCODE に残る。
    $out = & git -C $RepoDir -c core.quotepath=false @args 2>&1 | ForEach-Object { "$_" }
    return $out
}

# --- 1. リモートの最新を取ってくる ---
Write-Host "git fetch --all --prune ..."
$null = Invoke-Git fetch --all --prune
$fetched = ($LASTEXITCODE -eq 0)
if (-not $fetched) {
    Add-Result "リモートから取ってこられませんでした（オフラインなど）。Pull は飛ばします。"
}

if ($fetched) {
    # どのブランチが、どの worktree で開かれているか
    $checkedOut = @{}
    $wtPath = $null
    foreach ($line in (Invoke-Git worktree list --porcelain)) {
        if ($line -like 'worktree *') { $wtPath = $line.Substring(9) }
        elseif ($line -like 'branch refs/heads/*') { $checkedOut[$line.Substring(18)] = $wtPath }
    }
    $here = (Invoke-Git rev-parse --abbrev-ref HEAD | Select-Object -First 1)

    # --- 2. ローカルのブランチを、リモートに追いつかせる ---
    $branches = Invoke-Git for-each-ref --format='%(refname:short)|%(upstream:short)|%(upstream:track)' refs/heads
    foreach ($b in $branches) {
        $name, $upstream, $track = $b.Split('|')
        if (-not $upstream) { continue }                       # リモートを追いかけていない
        if ($track -eq '[gone]') { Add-Result "・$name : リモートのブランチが消えています（そのまま）"; continue }
        if ($track -notmatch 'behind') { continue }           # 遅れていない
        if ($track -match 'ahead') { Add-Result "・$name : ローカルとリモートが分かれています（そのまま。手でマージしてください） $track"; continue }

        if ($name -eq $here) {
            $out = Invoke-Git merge --ff-only $upstream
            if ($LASTEXITCODE -eq 0) { Add-Result "・$name : 追いつかせました（開いているフォルダー） $track" }
            else { Add-Result "・$name : 変更中のファイルとぶつかるので取り込めませんでした $track"; $out | ForEach-Object { Write-Host "    $_" } }
        }
        elseif ($checkedOut.ContainsKey($name)) {
            $wt = $checkedOut[$name]
            $out = & git -C $wt merge --ff-only $upstream 2>&1 | ForEach-Object { "$_" }
            if ($LASTEXITCODE -eq 0) { Add-Result "・$name : 追いつかせました（worktree $wt） $track" }
            else { Add-Result "・$name : worktree $wt の変更中のファイルとぶつかるので取り込めませんでした $track" }
        }
        else {
            # 開かれていないブランチは、早送りだけを許す fetch で指す先を進める
            $null = Invoke-Git fetch . "refs/remotes/${upstream}:refs/heads/$name"
            if ($LASTEXITCODE -eq 0) { Add-Result "・$name : 追いつかせました $track" }
            else { Add-Result "・$name : 追いつかせられませんでした $track" }
        }
    }

    # --- 3. リモートにしかないブランチを、ローカルにも作る ---
    $locals = @(Invoke-Git for-each-ref --format='%(refname:short)' refs/heads)
    foreach ($r in (Invoke-Git for-each-ref --format='%(refname:short)' refs/remotes)) {
        if ($r -notmatch '^([^/]+)/(.+)$') { continue }
        $remote, $name = $Matches[1], $Matches[2]
        if ($name -eq 'HEAD' -or $locals -contains $name) { continue }
        $null = Invoke-Git branch --track $name $r
        if ($LASTEXITCODE -eq 0) { Add-Result "・$name : リモートにしかなかったので、ローカルにも作りました"; $locals += $name }
    }

    if ($results.Count -eq 0) { Add-Result "どのブランチも最新でした。" }
}

# --- 4. Game.sln を開く ---
if (-not $NoOpen) {
    $sln = Join-Path $RepoDir 'Action\Game\Game.sln'
    if (-not (Test-Path $sln)) {
        Add-Result "$sln がありません。"
    }
    else {
        # このフォルダーの Game.sln を開いている Visual Studio があれば開かない。
        # ダブルクリックで開いたものはコマンドラインにパスが入る。最近使ったものから開いたものはウィンドウ名（Game - Microsoft Visual Studio）で見る。
        $opened = $false
        foreach ($p in (Get-CimInstance Win32_Process -Filter "Name='devenv.exe'" -ErrorAction SilentlyContinue)) {
            if ($p.CommandLine -and $p.CommandLine.ToLower().Contains($sln.ToLower())) { $opened = $true }
        }
        if (-not $opened) {
            foreach ($p in (Get-Process devenv -ErrorAction SilentlyContinue)) {
                if ($p.MainWindowTitle -like 'Game - *' -or $p.MainWindowTitle -like 'Game (*') { $opened = $true }
            }
        }
        if ($opened) { Add-Result "Game.sln は Visual Studio ですでに開いています。" }
        else { Start-Process $sln; Add-Result "Game.sln を開きました。" }
    }
}
