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

## 2. タブの開き直し

`.vscode/tasks.json` から `claude-sessions.ps1 -Mode restore` を呼びます。

- VSCode でこのフォルダーを開くと、いちばん最近 Claude を使ったPCの記録から、最後のやりとりの3時間前までに使った会話をタブで開く。
- 最後に使ったのがこのPCなら何もしない（VSCode が自分でタブを戻すため）。同じ記録では2回開かない。
- 別のPCで30分以内に使っていたときは、開く前に確認を出す。
- 初回は VSCode が「自動タスクを許可するか」を聞くので「許可」を選ぶ（`Tasks: Manage Automatic Tasks` で変えられる）。

## 3. アーカイブの引き継ぎ

アーカイブしたかどうかは会話ファイルではなく、PCごとの VSCode の保存領域（`%APPDATA%\Code\User\globalStorage\state.vscdb` の Claude 拡張の `hiddenSessionIds`）にあります。`claude-sessions.ps1` と `vscode-state.ps1` で扱います。

- **記録**: push のたびに、このリポジトリの会話のアーカイブ状態が変わっていたら、変わった時刻と一緒に「アーカイブの記録」（`%APPDATA%\claude-sessions\archive-Action\<PC名>.json`）に書き、`01_ClaudeSessions/Action-archive/` へ送る。
- **反映**（`-Mode archive-apply`）: 全PCの記録から、会話ごとにいちばん新しい状態を選んで書き込む。VSCode が開いている間は拡張に上書きされるので、**VSCode を閉じているときだけ**書き込む。
  - セッション開始時・終了時に、VSCode が閉じるのを待つ係を裏で起動し（同時に1つだけ）、閉じたらその時点で届いている記録を反映する（最長1日待つ）。
  - VSCode を開いていないとき（ターミナルの Claude など）は、セッション開始時の pull で反映する。
- そのため、別のPCでしたアーカイブは、そのあとこのPCで VSCode を閉じたときに書き込まれ、次に開いたときに一覧に出る。
- 書き込む前の保存領域の控えを `%APPDATA%\claude-sessions\state.vscdb.before-archive-apply` に残す。

## 4. 会話の削除

アーカイブした会話は、`.claude\scripts\delete-archived-sessions.cmd`（`claude-sessions.ps1 -Mode delete-archived`）を手で実行したときだけ消します。自動では消しません。

- **選ぶ**: 全PCのアーカイブの記録を合わせ、いまアーカイブされている会話を全部選ぶ。消す会話（ID・最後に使った日・大きさ）と合計を見せ、`y` を入力したら消す。`-DryRun` で見せるだけ、`-Yes` で確認を飛ばす。
- **記録**: 消す前に「消した記録」（`%APPDATA%\claude-sessions\deleted-Action\<PC名>.json`、会話の ID と消した時刻）に書き、`01_ClaudeSessions/Action-deleted/` へ送る。送れなければ何も消さない。
- **消す**: このPCの `<ID>.jsonl` と同じ名前のフォルダー、個人用 OneDrive の同じもの。OneDrive で消したものはごみ箱に約30日残る（rclone は標準ではごみ箱へ送る）。
- **ほかのPC**: pull・push・restore のたびに全PCの「消した記録」を見て、このPCに残っている会話を消し、送受信の対象からも外す（`rclone copy --exclude-from`）。pull では個人用 OneDrive からも消す（新しいスクリプトをまだ取ってきていないPCが送り戻した分を消すため）。
- 消した記録は消さずに残す。OneDrive のごみ箱から戻しても、記録があるかぎり次の pull でまた消える。

## 5. 使い方

- **はじめるとき**: 「バックアップがまだ設定されていません」と出たPCで、`.claude\scripts\setup-sessions.cmd` を1回実行する（rclone のインストールと、ブラウザでの個人アカウントの許可）。
- **アーカイブした会話を消すとき**: `.claude\scripts\delete-archived-sessions.cmd` を実行する。
- **そのPCだけやめるとき**: `.claude\scripts\remove-sessions.cmd`（rclone の設定と本体を消す）。
- **全PCまとめてやめるとき**: https://account.live.com/consent/Manage で rclone の許可を取り消す。

## 関連するページ

- [スマホへの通知の設計](../スマホへの通知/スマホへの通知の設計.md)
