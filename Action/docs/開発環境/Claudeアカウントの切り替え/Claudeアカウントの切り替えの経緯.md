# Claudeアカウントの切り替えの経緯

設計: [Claudeアカウントの切り替えの設計](Claudeアカウントの切り替えの設計.md)

何をして、なぜ今の形になったかの記録です。日付ごとに書き足します。

---

## 2026-11-19 アクティビティバーからアカウントを切り替える拡張機能を作る

- アクティビティバーにアカウントの一覧を出し、押したら切り替えたい、という頼みで作った。
- 切り替え方は2つを比べ、2にした。
  1. ログアウトしてログインし直す（`/logout` → `/login`）。毎回ブラウザでアカウントを選ぶことになり、一覧はどれでログインするかのメモにしかならない。
  2. ログイン情報を保存しておき、ファイルを入れ替える。ブラウザはアカウントごとに最初の1回だけ。Claude Code の公式な決まりではないので、更新で効かなくなるかもしれない。
- 入れ替えるファイルを決めるため、`~/.claude/.credentials.json` と `~/.claude.json` の**項目の名前だけ**を見た（中身の鍵は見ていない）。`.credentials.json` は `claudeAiOauth` の中にアクセストークン・リフレッシュトークン・期限など、`~/.claude.json` の `oauthAccount` には `accountUuid`・`emailAddress`・`organizationUuid` などが入っていた。鍵だけを替えると、表示されるアカウントと使う鍵が食い違うので、`oauthAccount` も一緒に替えることにした。
- 使っている間にリフレッシュトークンが取り替えられることがあるので、切り替える前に、使用中のアカウントを今の `.credentials.json` で保存し直すことにした。
- このPCには Node.js がないので、ビルドの要らない JavaScript 1ファイルで書き、VSCode にはジャンクションで読ませた（`.vsix` を作る道具 `vsce` は Node.js が要る）。
- Windows PowerShell 5.1 は、BOM のない `.ps1` の日本語を読み違えて文法の誤りになった。リポジトリのほかの `.ps1` と同じく、UTF-8（BOM 付き）で保存した。

### どう確かめたか

1. **VSCode が拡張機能として読むか。** `install-claude-accounts.ps1` でジャンクションを作り、`code --list-extensions --show-versions` に `local.claude-accounts@0.0.1` が出ることを見た。
2. **保存・切り替え・消すの動き。** 本物のログイン情報に触らないよう、使い捨てのフォルダを `USERPROFILE` にし、偽の `.credentials.json`・`.claude.json` を置いた。`vscode` の部分を差し替えた試験のスクリプトから `extension.js` を動かした（VSCode の `Code.exe` を `ELECTRON_RUN_AS_NODE=1` で動かすと Node.js として使える。窓は出さない）。結果は次のとおりで、どれも想定どおりだった。

| 試したこと | 結果 |
|---|---|
| A を保存 → B でログインしたことにして B を保存 | 一覧に A・B、B に「（使用中）」とチェック |
| B の鍵を B2 に変えて（使っている間の取り替え）から A に切り替え | `.credentials.json` が A の鍵に、`oauthAccount` が A に。`~/.claude.json` のほかの項目（`numStartups`）はそのまま |
| A に切り替えたあとの B の保存 | B2（取り替えたあとの鍵）になっていた |
| B に戻す | `.credentials.json` が B2 |
| B のまま「+」で名前を「B仕事」にする | ファイルは増えず（`uA.json`・`uB.json` のまま）、名前だけ替わった |
| A を消す | 一覧から A が消えた |
| 保存していない C でログインしたことにして切り替え、確かめで「やめる」 | `.credentials.json` は C のまま |

3. **手順書。** 手順書の枠（`<!-- new: -->`・`<!-- add: -->`）を作業用の道具（スクラッチの JavaScript）で上から順に空のフォルダへ当て、手順ごとに `package.json` が JSON として読めること・`extension.js` が JavaScript の文法として通ることを見た。最後に、4つのファイル（`package.json`・`accounts.svg`・`install-claude-accounts.ps1`・`extension.js`）が今のファイルと一字一句同じになり、全文の枠とも同じだった（枠 41 個、食い違い 0）。
4. **画面での確かめ。** 動いている VSCode の再読み込みは、使っている会話を止めてしまうので、ユーザーに頼んだ。

**用語:**

| 用語 | 何のことか |
|---|---|
| リフレッシュトークン | アクセストークン（短い期限の鍵）が切れたときに、新しい鍵をもらうための鍵。もらうたびに新しいものに取り替わることがある |
| `ELECTRON_RUN_AS_NODE` | VSCode（Electron）の `Code.exe` を、画面を出さずに Node.js として動かすための環境変数 |
