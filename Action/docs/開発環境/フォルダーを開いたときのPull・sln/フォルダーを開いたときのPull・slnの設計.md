# フォルダーを開いたときのPull・slnの設計

作成日: 2026-11-09
対象: [pull-and-open-sln.ps1](../../../../.claude/scripts/pull-and-open-sln.ps1)・[tasks.json](../../../../.vscode/tasks.json) の「Pull して sln を開く」
経緯: [フォルダーを開いたときのPull・slnの経緯](フォルダーを開いたときのPull・slnの経緯.md)

このリポジトリのフォルダーを VSCode で開くと、リモートの最新を取り込んでから `Action/Game/Game.sln` を Visual Studio で開く仕組みです。

- **ブランチは切り替えない。** 早送りで追いつけるブランチだけを進める（CLAUDE.md の「開いているフォルダのブランチを切り替えない」に合わせる）。
- **ローカルの作業は消さない。** ローカルの方が進んでいる・分かれている・変更中のファイルとぶつかるときは何もせず、結果に出すだけ。
- **取り込めなくても sln は開く**（オフラインなど）。

---

## 1. 全体像

VSCode でフォルダーを開く → `.vscode/tasks.json`（`runOn: folderOpen`）→ `pull-and-open-sln.ps1` → git fetch・ブランチの早送り → Game.sln を開く。

同じときに「Claude のタブを開き直す」（[Claudeの会話の引き継ぎの設計](../Claudeの会話の引き継ぎ/Claudeの会話の引き継ぎの設計.md)）も動く。2つは別のタスクで、互いに待たない。

**用語:**

| 用語 | 何のことか |
|---|---|
| タスク | VSCode の `.vscode/tasks.json` に書く、コマンドを動かす設定。フォルダーを開いたときに自動で動かすこともできる |
| 早送り（fast-forward） | 進める側のブランチが、今のブランチの続きになっているとき、ブランチの指す先を進めるだけで追いつくこと。マージコミットは作らない |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `runOn`（`folderOpen`） | フォルダーを開いたときにタスクを動かすため | `"runOn": "folderOpen"` と書くと、フォルダーを開いたときに自動で動く | VSCode の `tasks.json`（[tasks.json](../../../../.vscode/tasks.json) の `runOptions`） |
| 関数 | `git fetch --all --prune` | リモートの最新を取ってくるため | 全部のリモートの最新を取り、リモートで消えたブランチの印も消す。作業中のファイルやブランチは変えない | Git のコマンド |

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

**用語:**

| 用語 | 何のことか |
|---|---|
| worktree | 同じリポジトリを、別のフォルダーに別のブランチで取り出したもの（`git worktree`）。フォルダーごとに、開いているブランチが違う |
| 追いかける | ローカルのブランチに、対応するリモートのブランチ（`origin/名前`）が決まっていること。「遅れている・進んでいる」は、この相手との比べ方 |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `git merge --ff-only` | 早送りできるときだけ、ブランチを進めるため | 早送りできれば進め、できなければ何もしない（分かれているときにマージコミットを作らない） | Git のコマンド |
| 関数 | `git fetch . origin/<名前>:<名前>` | どこでも開いていないブランチの指す先だけを進めるため | リモートのブランチの先を、同じ名前のローカルのブランチへ取り込む。早送りでなければ git が断る | Git のコマンド |

## 3. sln を開く

- `Start-Process` で `Action/Game/Game.sln` を開く（.sln に関連付けた Visual Studio が開く）。
- すでに開いていれば、もう1つは開かない。次のどちらかなら「開いている」とみなす。
  - `devenv.exe` のコマンドラインに、このフォルダーの Game.sln のパスがある（ダブルクリックで開いたとき）
  - `devenv.exe` のウィンドウ名が `Game - …` で始まる（Visual Studio の「最近使ったもの」から開いたとき）

**用語:**

| 用語 | 何のことか |
|---|---|
| コマンドライン | プログラムを起動するときに、後ろに付けて渡した文字列（開くファイルのパスなど） |
| `.sln` | Visual Studio のソリューションのファイル。プロジェクトをまとめて開く |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Start-Process` | Game.sln を、関連付けた Visual Studio で開くため | ファイルやプログラムを、別のプロセスとして起動する | PowerShell のコマンドレット |
| 値 | `devenv.exe` | Visual Studio がもう開いているかを調べるため | Visual Studio の本体のプログラムの名前。動いているものの、コマンドラインとウィンドウ名を見る | Visual Studio |

## 4. 実装方法

- PowerShell 5.1 では、`$ErrorActionPreference = 'Stop'` にすると git が標準エラーに出す進み具合だけで止まるので、`Continue` にして `$LASTEXITCODE` を見る。
- git は `Invoke-Git` で呼ぶ（`-C <フォルダー>` と `core.quotepath=false` を付け、日本語のパスをそのまま出す）。関数の名前を `Git` にすると、中の `git` が自分を呼んでしまうため。
- どのブランチがどの worktree で開かれているかは `git worktree list --porcelain` の `worktree`・`branch` の行から作る。
- 遅れているかどうかは `git for-each-ref` の `%(upstream:track)`（`[behind 3]`・`[ahead 1, behind 2]`・`[gone]`）で見る。
- 日本語のコメントがあるので、ファイルは BOM 付き UTF-8 にする（PowerShell 5.1 が BOM なしを ANSI として読むため）。

**用語:**

| 用語 | 何のことか |
|---|---|
| PowerShell 5.1 | Windows に最初から入っている PowerShell の版（`powershell.exe`）。新しい PowerShell 7（`pwsh`）とは別 |
| 標準エラー | プログラムが、エラーや進み具合を出す先。ふつうの出力（標準出力）とは別になっている |
| BOM | UTF-8 のファイルの先頭に付ける、文字コードを知らせる印（3バイト） |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `$ErrorActionPreference` | エラーが出たときの動きを決めるため | `'Stop'` だと、エラーが出た時点で止まる。`'Continue'` だと、表示して続ける | PowerShell の自動の設定変数 |
| 変数 | `$LASTEXITCODE` | 直前の git が成功したかを見るため | 直前に動かした外部のコマンドの終了コード（0 が成功） | PowerShell の自動変数 |
| 関数 | `Invoke-Git` | git を、いつも同じ付け方で呼ぶため | `-C <フォルダー>` と `core.quotepath=false` を付けて git を動かし、出力を文字列で返す | 開発環境の [pull-and-open-sln.ps1](../../../../.claude/scripts/pull-and-open-sln.ps1)（自分で作った関数） |
| 引数 | `-C <フォルダー>` | どのフォルダーのリポジトリで動かすかを指すため | git が作業するフォルダーを指定する | Git のオプション |
| 値 | `core.quotepath=false` | 日本語のパスをそのまま出すため | 日本語のパスを `\343\201...` のような数字にせず、そのまま出す設定（`-c` で渡す） | Git の設定 |
| 関数 | `git worktree list --porcelain` | どのブランチがどの worktree で開かれているかを知るため | worktree ごとに、`worktree <パス>`・`branch <名前>` の行を出す | Git のコマンド |
| 関数 | `git for-each-ref` | ブランチの一覧と、遅れ・進みを読むため | ブランチごとに、`--format` で決めた項目を1行で出す | Git のコマンド |
| 値 | `%(upstream:track)` | 遅れているか、進んでいるかを知るため | `[behind 3]`・`[ahead 1, behind 2]`・`[gone]` のような文字を返す | Git の `for-each-ref` の書式 |

## 5. 使い方

- **初めて開いたPC**: VSCode が「自動タスクを許可するか」を聞くので「許可」を選ぶ（`Tasks: Manage Automatic Tasks` で変えられる）。
- **結果を見る**: VSCode のターミナルの一覧にある「Pull して sln を開く」を開く。前には出さない。
- **手で動かす**: `powershell -ExecutionPolicy Bypass -File .claude\scripts\pull-and-open-sln.ps1`（`-NoOpen` を付けると sln を開かない）。

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 引数 | `-ExecutionPolicy Bypass` | スクリプトを動かせるようにするため | PowerShell のスクリプトの実行を制限する設定を、今回だけ無視する | Windows の PowerShell（`powershell.exe`）のオプション |
| 引数 | `-File` | 動かすスクリプトを指すため | 指した `.ps1` のファイルを動かす | Windows の PowerShell（`powershell.exe`）のオプション |
| 引数 | `-NoOpen` | 取り込みだけを確かめるため | 付けると、取り込みだけして sln を開かない | 開発環境の [pull-and-open-sln.ps1](../../../../.claude/scripts/pull-and-open-sln.ps1) の `param`（`[switch]`） |

## 6. 既知の課題

- ウィンドウ名で見る判定は、ほかのフォルダー（`Action-try` など）の Game.sln を開いている Visual Studio も「開いている」とみなす。
- 分かれているブランチは知らせるだけで、通知などは出さない。ターミナルを開いて見る。

## 関連するページ

- [Claudeの会話の引き継ぎの設計](../Claudeの会話の引き継ぎ/Claudeの会話の引き継ぎの設計.md)（同じ tasks.json で、フォルダーを開いたときに動く）
