# フォルダーを開いたときのPull・slnの設計

作成日: 2026-11-09
対象: [pull-and-open-sln.ps1](../../../.claude/scripts/pull-and-open-sln.ps1)・[tasks.json](../../../.vscode/tasks.json) の「Pull して sln を開く」
経緯: [フォルダーを開いたときのPull・slnの経緯](フォルダーを開いたときのPull・slnの経緯.md)

このリポジトリのフォルダーを VSCode で開くと、リモートの最新を取り込んでから `Action/Game/Game.sln` を Visual Studio で開く仕組みです。

- **ブランチは切り替えない。** 早送りで追いつけるブランチだけを進める（CLAUDE.md の「開いているフォルダのブランチを切り替えない」に合わせる）。
- **ローカルの作業は消さない。** ローカルの方が進んでいる・分かれている・変更中のファイルとぶつかるときは何もせず、結果に出すだけ。
- **取り込めなくても sln は開く**（オフラインなど）。

---

## 1. 全体像

VSCode でフォルダーを開く → `.vscode/tasks.json`（`runOn: folderOpen`）→ `pull-and-open-sln.ps1` → git fetch・ブランチの早送り → Game.sln を開く。

同じときに「Claude のタブを開き直す」（[Claudeの会話の引き継ぎの設計](Claudeの会話の引き継ぎの設計.md)）も動く。2つは別のタスクで、互いに待たない。

## 2. ブランチごとの扱い

最初に `git fetch --all --prune` でリモートの最新を取ってから、ローカルのブランチを1本ずつ見る。

| ブランチの状態 | すること |
|---|---|
| リモートより遅れている・開いているフォルダーのブランチ | `git merge --ff-only`。変更中のファイルとぶつかれば何もしない |
| 遅れている・ほかの worktree で開いているブランチ | その worktree で `git merge --ff-only` |
| 遅れている・どこでも開いていないブランチ | `git fetch . origin/<名前>:<名前>` で指す先だけ進める（早送りでなければ git が断る） |
| ローカルとリモートが分かれている | 何もしない。手でマージするよう結果に出す |
| ローカルの方が進んでいる・最新 | 何もしない |
| リモートのブランチが消えている | 何もしない（ローカルのブランチは消さない） |
| リモートにしかない | 同じ名前で追いかけるローカルのブランチを作る |

リモートを追いかけていないブランチ（まだ push していない作業ブランチ）は見ない。main を作業ブランチへ取り込むこと（`git merge main`）もしない。

## 3. sln を開く

- `Start-Process` で `Action/Game/Game.sln` を開く（.sln に関連付けた Visual Studio が開く）。
- すでに開いていれば、もう1つは開かない。次のどちらかなら「開いている」とみなす。
  - `devenv.exe` のコマンドラインに、このフォルダーの Game.sln のパスがある（ダブルクリックで開いたとき）
  - `devenv.exe` のウィンドウ名が `Game - …` で始まる（Visual Studio の「最近使ったもの」から開いたとき）

## 4. 実装方法

- PowerShell 5.1 では、`$ErrorActionPreference = 'Stop'` にすると git が標準エラーに出す進み具合だけで止まるので、`Continue` にして `$LASTEXITCODE` を見る。
- git は `Invoke-Git` で呼ぶ（`-C <フォルダー>` と `core.quotepath=false` を付け、日本語のパスをそのまま出す）。関数の名前を `Git` にすると、中の `git` が自分を呼んでしまうため。
- どのブランチがどの worktree で開かれているかは `git worktree list --porcelain` の `worktree`・`branch` の行から作る。
- 遅れているかどうかは `git for-each-ref` の `%(upstream:track)`（`[behind 3]`・`[ahead 1, behind 2]`・`[gone]`）で見る。
- 日本語のコメントがあるので、ファイルは BOM 付き UTF-8 にする（PowerShell 5.1 が BOM なしを ANSI として読むため）。

## 5. 使い方

- **初めて開いたPC**: VSCode が「自動タスクを許可するか」を聞くので「許可」を選ぶ（`Tasks: Manage Automatic Tasks` で変えられる）。
- **結果を見る**: VSCode のターミナルの一覧にある「Pull して sln を開く」を開く。前には出さない。
- **手で動かす**: `powershell -ExecutionPolicy Bypass -File .claude\scripts\pull-and-open-sln.ps1`（`-NoOpen` を付けると sln を開かない）。

## 6. 既知の課題

- ウィンドウ名で見る判定は、ほかのフォルダー（`Action-try` など）の Game.sln を開いている Visual Studio も「開いている」とみなす。
- 分かれているブランチは知らせるだけで、通知などは出さない。ターミナルを開いて見る。

## 関連するページ

- [Claudeの会話の引き継ぎの設計](Claudeの会話の引き継ぎの設計.md)（同じ tasks.json で、フォルダーを開いたときに動く）
