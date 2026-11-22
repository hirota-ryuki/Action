# Claude アカウントの切り替え（.claude/extensions/claude-accounts）を、このPCの VSCode に入れる。
# VSCode の拡張機能フォルダに、リポジトリのフォルダを指すジャンクションを作る（git pull すれば新しい版になる）。
# -Uninstall で外す。どちらも、そのあと VSCode のウィンドウを再読み込みすると効く。
param([switch]$Uninstall)

$source = Join-Path $PSScriptRoot '..\extensions\claude-accounts' | Resolve-Path
$link = Join-Path $env:USERPROFILE '.vscode\extensions\local.claude-accounts-0.0.1'

if (Test-Path $link) {
    # ジャンクションだけを消す（-Recurse を付けないので、リポジトリの中身は消えない）
    (Get-Item $link).Delete()
}
if ($Uninstall) {
    Write-Output '外しました。VSCode のウィンドウを再読み込みしてください。'
    return
}
New-Item -ItemType Junction -Path $link -Target $source | Out-Null
Write-Output "入れました: $link -> $source"
Write-Output 'VSCode のウィンドウを再読み込みすると、アクティビティバーに「Claude アカウント」が出ます。'
