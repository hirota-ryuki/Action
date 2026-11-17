# VSCode で今開いている Claude の会話のタブの名前を、左の窓から順に1行ずつ出す。
# 「別PC」と言われたときに Claude が実行し、出た名前を .claude/開いていたタブ.md に保存する（CLAUDE.md の「別PC」）。
#
# VSCode はフォルダごとに %APPDATA%\Code\User\workspaceStorage\<ハッシュ>\state.vscdb を持ち、
# その ItemTable の memento/workbench.parts.editor に、開いているタブの並びを JSON で入れている。
# Claude の会話のタブは viewType が mainThreadWebview-claudeVSCodePanel の webview で、title がタブの名前。
# VSCode はタブを変えて数秒たつとこのファイルに書き出すので、開いたばかりのタブは少し待ってから読む。

# -RepoRoot: VSCode で開いているフォルダ。省くとこのスクリプトのあるリポジトリ（.claude の1つ上）。
param([string]$RepoRoot = (Join-Path $PSScriptRoot '..\..'))

. (Join-Path $PSScriptRoot 'vscode-state.ps1')

# そのフォルダを開いている VSCode の workspaceStorage を探す
$repoRoot = (Resolve-Path $RepoRoot).Path.TrimEnd('\')
$storage  = Join-Path $env:APPDATA 'Code\User\workspaceStorage'
$db = $null
foreach ($ws in Get-ChildItem $storage -Directory -ErrorAction SilentlyContinue) {
    $wsJson = Join-Path $ws.FullName 'workspace.json'
    $state  = Join-Path $ws.FullName 'state.vscdb'
    if (-not (Test-Path $wsJson) -or -not (Test-Path $state)) { continue }
    $folder = ((New-JsonSerializer).DeserializeObject((Get-Content $wsJson -Raw -Encoding UTF8)))['folder']
    if (-not $folder) { continue }
    $path = [Uri]::UnescapeDataString(($folder -replace '^file:///', '')).Replace('/', '\').TrimEnd('\')
    if ($path -ieq $repoRoot) { $db = $state; break }
}
if (-not $db) { Write-Error "このフォルダ（$repoRoot）を開いている VSCode の保存領域が見つかりません"; exit 1 }

$row = [ClaudeSessions.VsState]::Get($db, 'memento/workbench.parts.editor')
if (-not $row) { exit 0 }
$json = New-JsonSerializer

# 窓の分け方（serializedGrid）をたどって、各窓のタブを並び順に集める
function Get-Tabs($node) {
    if ($node -is [System.Collections.IDictionary]) {
        if ($node.ContainsKey('editors')) {
            foreach ($e in $node['editors']) {
                if ($e['id'] -ne 'workbench.editors.webviewInput') { continue }
                $v = $json.DeserializeObject($e['value'])
                if ($v['providedId'] -eq 'claudeVSCodePanel') { $v['title'] }
            }
        }
        foreach ($k in @($node.Keys)) { if ($k -ne 'editors') { Get-Tabs $node[$k] } }
    } elseif ($node -is [System.Collections.IEnumerable] -and $node -isnot [string]) {
        foreach ($x in $node) { Get-Tabs $x }
    }
}

Get-Tabs ($json.DeserializeObject($row[1])['editorpart.state'])
