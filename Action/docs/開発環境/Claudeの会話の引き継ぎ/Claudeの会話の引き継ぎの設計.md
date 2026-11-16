# Claudeの会話の引き継ぎの設計

作成日: 2026-11-09
対象: [claude-sessions.ps1](../../../../.claude/scripts/claude-sessions.ps1)・[delete-archived-sessions.cmd](../../../../.claude/scripts/delete-archived-sessions.cmd)・[vscode-state.ps1](../../../../.claude/scripts/vscode-state.ps1)・[settings.json](../../../../.claude/settings.json) のフック・`.vscode/tasks.json`
経緯: [Claudeの会話の引き継ぎの経緯](Claudeの会話の引き継ぎの経緯.md)

Claude の会話（タブ）を、どのPCでも続けられるようにする仕組みです。会話ファイル・開いていたタブ・アーカイブの状態・消した会話を、個人用 OneDrive を通してPCの間でそろえます。

- **会話ファイルは各PCに置いたまま、rclone で個人用 OneDrive にコピーする。** 会社用（学校法人河原学園）の OneDrive には置かない。個人用 OneDrive のアプリにはサインインしない。
- **コピーは「新しい方で上書き」だけ。** ファイルを消しても相手側には伝わらない。会話を消すときは「消した記録」を残し、それを見て全PCで消す。
- **同じ会話を2台のPCで同時に進めない。** 新しい方で上書きされ、もう片方の続きが消える。

---

## 1. 会話ファイルのコピー

会話ファイルは各PCの `~/.claude/projects/<このリポジトリのパス>/` にあり、個人用 OneDrive の `01_ClaudeSessions/Action/` にコピーします。`.claude/settings.json` のフックから `claude-sessions.ps1` を呼びます。

| いつ | 何をするか |
|---|---|
| セッション開始時 | 個人用 OneDrive から新しい会話を取ってくる（pull） |
| やりとりの区切り・セッション終了時 | 個人用 OneDrive へ送る（push） |
| やりとりの区切り | その会話の ID と時刻を「タブの記録」（`%APPDATA%\claude-sessions\tabs-Action\<PC名>.json`）に書き、`01_ClaudeSessions/Action-tabs/` へ送る |

**用語:**

| 用語 | 何のことか |
|---|---|
| 個人用 OneDrive | 個人の Microsoft アカウントのクラウドの保存場所。会社用（学校法人河原学園）のものとは別 |
| フック | Claude が決まったとき（セッションの開始・やりとりの区切り・終了）に、設定したコマンドを自動で動かす仕組み。`.claude/settings.json` に書く |
| セッション・会話ファイル | Claude との1つの会話がセッション。その中身は、1行が1つの JSON の `.jsonl` ファイル（`<会話のID>.jsonl`）に入る |
| pull・push | ここでは、個人用 OneDrive から取ってくること（pull）と、送ること（push）。Git の pull・push とは別 |
| `%APPDATA%` | Windows の、ユーザーごとのアプリの設定を置くフォルダー（`C:\Users\<ユーザー名>\AppData\Roaming`）を指す環境変数 |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `rclone` | 会話ファイルを個人用 OneDrive へコピーするため | クラウドの保存場所とフォルダーの間で、ファイルをコピーするコマンド。ブラウザーで1回許可すると、アプリにサインインせずに送れる | 外部ツール（rclone。`-Mode setup` で `winget` から入れる） |
| 引数 | `-Mode` | 1つのスクリプトで、いくつもの動きを切り替えるため | `setup`・`pull`・`push`・`restore`・`remove`・`archive-apply`・`delete-archived` のどれかを選ぶ。省略すると `pull` | 開発環境の [claude-sessions.ps1](../../../../.claude/scripts/claude-sessions.ps1) の `param` |
| 値 | `~/.claude/projects/<このリポジトリのパス>/` | 各PCの会話ファイルの置き場所 | リポジトリのパスの、英数字以外を `-` にした名前のフォルダー | Claude の設定フォルダー（`~/.claude`） |

## 2. タブの開き直し

`.vscode/tasks.json` から `claude-sessions.ps1 -Mode restore` を呼びます。

- VSCode でこのフォルダーを開くと、いちばん最近 Claude を使ったPCの記録から、最後のやりとりの3時間前までに使った会話をタブで開く。
- 最後に使ったのがこのPCなら何もしない（VSCode が自分でタブを戻すため）。同じ記録では2回開かない。
- 別のPCで30分以内に使っていたときは、開く前に確認を出す。
- 初回は VSCode が「自動タスクを許可するか」を聞くので「許可」を選ぶ（`Tasks: Manage Automatic Tasks` で変えられる）。

**用語:**

| 用語 | 何のことか |
|---|---|
| タスク | VSCode の `.vscode/tasks.json` に書く、コマンドを動かす設定。フォルダーを開いたときに自動で動かすこともできる |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `restore` | 別のPCのタブを、このPCで開き直すため | `-Mode` の値の1つ。会話をタブで開く | [claude-sessions.ps1](../../../../.claude/scripts/claude-sessions.ps1) の `-Mode` |
| 値 | `runOn`（`folderOpen`） | フォルダーを開いたときにタスクを動かすため | `"runOn": "folderOpen"` と書くと、フォルダーを開いたときに自動で動く | VSCode の `tasks.json`（[tasks.json](../../../../.vscode/tasks.json) の `runOptions`） |
| 値 | `Tasks: Manage Automatic Tasks` | 自動で動くタスクを許可・禁止するため | VSCode のコマンド（`Ctrl+Shift+P` で出す）。自動タスクを許可するかを変える | VSCode |

## 3. アーカイブの引き継ぎ

アーカイブしたかどうかは会話ファイルではなく、PCごとの VSCode の保存領域（`%APPDATA%\Code\User\globalStorage\state.vscdb` の Claude 拡張の `hiddenSessionIds`）にあります。`claude-sessions.ps1` と `vscode-state.ps1` で扱います。

- **記録**: push のたびに、このリポジトリの会話のアーカイブ状態が変わっていたら、変わった時刻と一緒に「アーカイブの記録」（`%APPDATA%\claude-sessions\archive-Action\<PC名>.json`）に書き、`01_ClaudeSessions/Action-archive/` へ送る。
- **反映**（`-Mode archive-apply`）: 全PCの記録から、会話ごとにいちばん新しい状態を選んで書き込む。VSCode が開いている間は拡張に上書きされるので、**VSCode を閉じているときだけ**書き込む。
  - セッション開始時・終了時に、VSCode が閉じるのを待つ係を裏で起動し（同時に1つだけ）、閉じたらその時点で届いている記録を反映する（最長1日待つ）。
  - VSCode を開いていないとき（ターミナルの Claude など）は、セッション開始時の pull で反映する。
- そのため、別のPCでしたアーカイブは、そのあとこのPCで VSCode を閉じたときに書き込まれ、次に開いたときに一覧に出る。
- 書き込む前の保存領域の控えを `%APPDATA%\claude-sessions\state.vscdb.before-archive-apply` に残す。

**用語:**

| 用語 | 何のことか |
|---|---|
| 拡張（拡張機能） | VSCode に機能を足すもの。ここでは Claude Code の拡張 |
| SQLite | 1つのファイルに表の形でデータを入れておくデータベース。`state.vscdb` がこの形式 |
| globalState | VSCode の拡張が、PC全体で覚えておく値の入れ物。拡張ごとの JSON で、`state.vscdb` に入っている |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `state.vscdb` | アーカイブしたかを読み書きするため | VSCode が拡張のために使う保存領域のファイル（`%APPDATA%\Code\User\globalStorage\`） | VSCode |
| 値 | `hiddenSessionIds` | アーカイブした会話を見分けるため | Claude の拡張の globalState の中の、アーカイブした会話の ID の配列（JSON のキー） | Claude Code の VSCode 拡張 |
| 値 | `archive-apply` | ほかのPCのアーカイブを、このPCへ書き込むため | `-Mode` の値の1つ | [claude-sessions.ps1](../../../../.claude/scripts/claude-sessions.ps1) の `-Mode` |

## 4. 会話の削除

アーカイブした会話は、`.claude\scripts\delete-archived-sessions.cmd`（`claude-sessions.ps1 -Mode delete-archived`）を手で実行したときだけ消します。自動では消しません。

- **選ぶ**: 全PCのアーカイブの記録を合わせ、いまアーカイブされている会話を全部選ぶ。消す会話（ID・最後に使った日・大きさ）と合計を見せ、`y` を入力したら消す。`-DryRun` で見せるだけ、`-Yes` で確認を飛ばす。
- **記録**: 消す前に「消した記録」（`%APPDATA%\claude-sessions\deleted-Action\<PC名>.json`、会話の ID と消した時刻）に書き、`01_ClaudeSessions/Action-deleted/` へ送る。送れなければ何も消さない。
- **消す**: このPCの `<ID>.jsonl` と同じ名前のフォルダー、個人用 OneDrive の同じもの。OneDrive で消したものはごみ箱に約30日残る（rclone は標準ではごみ箱へ送る）。
- **ほかのPC**: pull・push・restore のたびに全PCの「消した記録」を見て、このPCに残っている会話を消し、送受信の対象からも外す（`rclone copy --exclude-from`）。pull では個人用 OneDrive からも消す（新しいスクリプトをまだ取ってきていないPCが送り戻した分を消すため）。
- 消した記録は消さずに残す。OneDrive のごみ箱から戻しても、記録があるかぎり次の pull でまた消える。

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `delete-archived` | アーカイブした会話を消すため | `-Mode` の値の1つ。`delete-archived-sessions.cmd` から手で動かす | [claude-sessions.ps1](../../../../.claude/scripts/claude-sessions.ps1) の `-Mode` |
| 引数 | `-DryRun` | 消す前に、何が消えるかを確かめるため | 付けると、消す会話を見せるだけで消さない | [claude-sessions.ps1](../../../../.claude/scripts/claude-sessions.ps1) の `param`（`[switch]`） |
| 引数 | `-Yes` | 確認の入力を省くため | 付けると、`y` の確認を飛ばして消す | [claude-sessions.ps1](../../../../.claude/scripts/claude-sessions.ps1) の `param`（`[switch]`） |
| 引数 | `--exclude-from` | 消した会話を、送受信の対象から外すため | 外すファイルの名前の一覧を、ファイルで渡す（`/<ID>.jsonl`・`/<ID>/**`）。`rclone copy` に付ける | 外部ツール（rclone）のオプション |

## 5. 使い方

- **はじめるとき**: 「バックアップがまだ設定されていません」と出たPCで、`.claude\scripts\setup-sessions.cmd` を1回実行する（rclone のインストールと、ブラウザでの個人アカウントの許可）。
- **アーカイブした会話を消すとき**: `.claude\scripts\delete-archived-sessions.cmd` を実行する。
- **そのPCだけやめるとき**: `.claude\scripts\remove-sessions.cmd`（rclone の設定と本体を消す）。
- **全PCまとめてやめるとき**: https://account.live.com/consent/Manage で rclone の許可を取り消す。

## 関連するページ

- [スマホへの通知の設計](../スマホへの通知/スマホへの通知の設計.md)
