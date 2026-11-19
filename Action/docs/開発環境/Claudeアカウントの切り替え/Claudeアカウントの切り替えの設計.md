# Claudeアカウントの切り替えの設計

作成日: 2026-11-19
対象: [extension.js](../../../../.claude/extensions/claude-accounts/extension.js)・[package.json](../../../../.claude/extensions/claude-accounts/package.json)・[install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1)
経緯: [Claudeアカウントの切り替えの経緯](Claudeアカウントの切り替えの経緯.md)
手順: [Claudeアカウントの切り替えの手順](Claudeアカウントの切り替えの手順.md)

VSCode のアクティビティバーに「Claude アカウント」を足し、一覧の名前を押すだけで Claude Code のアカウントを切り替える、自分用の拡張機能です。各アカウントの残りの使用量（5時間・週）と、復活する時刻も出します。

- **ログアウト・ログインはしない。** アカウントごとに一度だけ普通にログインし、そのログイン情報を保存しておいて、押したらファイルを入れ替える。ブラウザは開かない。
- **保存したログイン情報は、各PCの `~/.claude/account-profiles/` にだけ置く。** リポジトリには入れない。
- **入れ替えたあとは、ウィンドウの再読み込みで開いている Claude に効かせる。**

**用語:**

| 用語 | 何のことか |
|---|---|
| アクティビティバー | VSCode の左端の、アイコンの縦の列 |
| ログイン情報 | Claude のサーバーに入るための鍵（アクセストークンと、それを取り替えるためのリフレッシュトークン）。`~/.claude/.credentials.json` に入っている |
| `oauthAccount` | `~/.claude.json` の中の、だれでログインしているか（番号 `accountUuid`・メールアドレスなど） |
| ジャンクション | Windows の、フォルダへのリンク。管理者の権限なしで作れる |

---

## 1. 全体像

```
アクティビティバーのアイコン
  └ 一覧（保存したアカウント。使用中に印。並びは↑↓で変えられる）
      ├ 見出しの「+」     … 今のアカウントを保存（名前を付ける）
      ├ 見出しの回る矢印 … 一覧と使用量を読み直す
      ├ 名前を押す       … そのアカウントに切り替え → 「再読み込み」
      ├ 行の↑↓           … 1つ上・1つ下と入れ替える
      ├ 行のゴミ箱       … 保存を消す（ログアウトはしない）
      └ 行の下           … 5時間: 残り 92%・17:00 に復活（あと 4時間38分）
                           週: 残り 53%・11/23(月) 20:00 に復活（あと 4日7時間）
```

| 置き場所 | 中身 | 書くもの |
|---|---|---|
| `~/.claude/.credentials.json` | ログイン情報 | Claude Code・切り替え |
| `~/.claude.json` | Claude Code の設定。`oauthAccount` だけを切り替えで書く | Claude Code・切り替え |
| `~/.claude/account-profiles/<accountUuid>.json` | アカウント1つ分 `{ name, oauthAccount, credentials }` | 保存・切り替え |
| VSCode の `globalState` の `order` | 並び順（`accountUuid` を上から並べた配列） | ↑↓ |
| VSCode の `globalState` の `usage` | アカウントごとの、最後に取れた使用量 | 使用量の取り直し |

`~` は `%USERPROFILE%`（`C:\Users\<ユーザー名>`）です。`globalState` は、VSCode がこのPCに覚えておく場所です（どのフォルダを開いても同じ）。

## 2. 切り替えの流れ

1. 押したのが使用中のアカウントなら、何もしない。
2. 使用中のアカウントが保存済みなら、今の `.credentials.json` で**保存し直す**。保存していなければ「戻すときにもう一度ログインが要る」と聞き、やめることもできる。
3. 押したアカウントの `credentials` を `.credentials.json` に、`oauthAccount` を `~/.claude.json` に書く（`~/.claude.json` のほかの設定はそのまま）。
4. 「再読み込み」のボタン付きで知らせる。

2 で保存し直すのは、Claude Code が使っている間にリフレッシュトークンを新しいものに取り替えることがあり、そのとき古いものは使えなくなることがあるためです。保存した時点のままにしておくと、戻したときにログインし直しになります。

## 3. 使用量の出し方

- Claude Code の `/usage` と同じ API（`https://api.anthropic.com/api/oauth/usage`）に、アカウントの鍵（使用中は `.credentials.json`、ほかは保存した鍵）を付けて聞く。返ってくる `five_hour`・`seven_day` の `utilization`（使った割合）と `resets_at`（復活する時刻）を使う。
- 残りは `100 − utilization`。復活の時刻は、16:59:59.68 のような値を次の分（17:00）に切り上げて出す。週の枠だけ日付も出す。時刻を過ぎていれば「残り 100%（復活済み）」。
- 取り直すのは、一覧が見えていて、前に取ってから5分たったとき（読み直しのボタン・保存・切り替え・消したあとは、すぐ）。「あと〇分」は1分ごとに描き直す。
- 期限の切れた鍵では聞かず、前に取れた値を「〇時〇分に取った値」と添えて出す。使っていない鍵をこちらで新しくする（リフレッシュ）ことはしない。
- 401（鍵が無効）が返ったら「保存した鍵が無効」と出す。そのあと 401 ではない理由（429 の混雑・期限切れなど）で取れなければ「取れなかった」に替える（「無効」を残すと、鍵が使えるようになっても変わらないため）。
- 保存のボタンで保存し直したら、そのアカウントの前の値を消して、新しい鍵ですぐ取り直す。

| 出る行 | いつ |
|---|---|
| `5時間: 残り 92%・17:00 に復活（あと 4時間38分）` | ふつう |
| `（11/19 11:21 に取った値。鍵の期限切れなどで取り直せていない）` | 10分より前の値のまま |
| `使用量: まだ取れていない（このアカウントに切り替えると取れる）` | 1回も取れていない |
| `保存した鍵が無効（ログインし直して、保存し直す）` | 401 が返った |
| `使用量: 取れなかった（混んでいる・鍵の期限切れなど。5分ごとに取り直す）` | 「無効」のあと、401 ではない理由で取れなかった |

## 4. 実装方法

拡張機能は JavaScript 1ファイル（[extension.js](../../../../.claude/extensions/claude-accounts/extension.js)）で、ビルドも Node.js のインストールも要りません。画面の部品（アイコン・一覧・ボタン・コマンド）は [package.json](../../../../.claude/extensions/claude-accounts/package.json) の `contributes` に書き、動きは `activate` でコマンドに結びつけます。

**アカウントは番号で見分ける。** 一覧の印・保存し直す先は、`oauthAccount.accountUuid`（アカウントごとに変わらない番号）で決めます。保存ファイルの名前も `<accountUuid>.json` なので、同じアカウントを2回保存しても1つのファイルに上書きされます（名前を付け直すのにも使えます）。

```js
function findProfile(accountUuid) {
  return loadProfiles().find((p) => p.oauthAccount.accountUuid === accountUuid);
}
```

**書きかけを読ませない。** Claude Code が読んでいる最中のファイルを書くので、`〇〇.tmp` に書いてから名前を変えて置き換えます。読む側は前の中身か新しい中身のどちらかしか見ません。

```js
function writeJson(file, value) {
  const temp = file + '.tmp';
  fs.writeFileSync(temp, JSON.stringify(value, null, 2), 'utf8');
  fs.renameSync(temp, file);   // Node.js の fs。同じ名前があれば置き換える
}
```

**`~/.claude.json` は `oauthAccount` だけを替える。** このファイルにはプロジェクトごとの設定なども入っているので、読んで `oauthAccount` だけを差し替えて書き戻します（`applyProfile`）。

**一覧は `TreeDataProvider`。** `AccountsProvider` の `getChildren` が、保存したアカウントを1行（`vscode.TreeItem`）ずつにします。行を押すと `claudeAccounts.switch` がそのアカウントを受け取り、ゴミ箱のボタンは行（`item.profile` に持たせたアカウント）を受け取ります。保存・切り替え・消したあとは `refresh()`（`EventEmitter.fire`）で描き直します。行の種類（`contextValue = 'account'`）を付け、↑↓とゴミ箱はアカウントの行にだけ出します。

**使用量は、アカウントの行の下（子）の行。** アカウントの行は開いた形（`TreeItemCollapsibleState.Expanded`）にし、VSCode が子を聞いてきたら（`getChildren(element)`）使用量の行を返します。一番上の行を描くときに、5分たっていれば取り直しを始めます（待たずに先へ進み、取れたら描き直す）。

```js
getChildren(element) {
  if (element) return usageItems(element.profile);
  if (Date.now() - usageFetchedAt > USAGE_INTERVAL_MS) updateUsage(this);
  ...
```

`refresh()` は `usageFetchedAt = 0` にしてから描き直すので、読み直し・保存・切り替えのあとはすぐ取り直します。1分ごとの描き直しと並べ替えは `changed.fire()` だけで、取り直しは5分ごとのままです。`fetch` は Node.js 18 からなので、`engines` は VSCode 1.82 以降にしています。

**並び順は番号の配列。** `sortByOrder` が `globalState` の `order` の順に並べ、`order` にないアカウント（新しく保存したもの）は後ろに、名前の順のまま置きます（`sort` は同じ順位の前後を変えない）。↑↓は、画面の今の並びを `order` として覚え直します。

**入れ方はジャンクション。** [install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1) が `%USERPROFILE%\.vscode\extensions\local.claude-accounts-0.0.1` に、リポジトリの `.claude\extensions\claude-accounts` を指すジャンクションを作ります。git pull で中身が新しくなっても、入れ直さずに再読み込みだけで効きます。

## 5. 使い方

- **このPCに入れる**（PCごとに1回）: リポジトリのフォルダで `powershell -ExecutionPolicy Bypass -File .claude\scripts\install-claude-accounts.ps1` を動かし、`Ctrl+Shift+P` →「Developer: Reload Window」。外すときは `-Uninstall` を付ける。
- **アカウントを足す**: 足したいアカウントでログインし、一覧の「+」で名前を付けて保存する。**保存したアカウントで `/logout` しない**（その鍵がサーバーで無効になり、保存した鍵も使えなくなる。6. 既知の課題）。
- **切り替える**: 一覧の名前を押し、「再読み込み」を押す。ほかの VSCode のウィンドウでも Claude を開いているときは、そちらも再読み込みする。
- **並べ替える**: 行にマウスを乗せて↑↓を押す。
- **残りの使用量を見る**: 各アカウントの下の2行を見る。すぐ新しくしたいときは、見出しの回る矢印を押す。
- **消す**: 行にマウスを乗せてゴミ箱を押す。保存ファイルが消えるだけで、ログアウトはしない。

## 6. 決めたこと

- **ログアウトしてログインし直す形にしない。** その形だと毎回ブラウザでアカウントを選ぶことになり、一覧はメモにしかならないため。
- **保存はPCごと。** ログイン情報をリポジトリや OneDrive に置くと、ほかの人や別のPCに鍵を渡すことになる。別のPCでは、そのPCで一度ずつログインして保存する。
- **VSCode のストア向けの形（`.vsix`）にしない。** 自分のPCで使うだけなので、ジャンクションで直接読ませる。
- **並び順と使用量は保存ファイルに書かない。** 保存ファイルは切り替えのたびに書き直すので、見た目のための値は `globalState` に分けた。
- **使っていないアカウントの鍵は新しくしない。** こちらで新しくすると、Claude Code の知らない所で鍵が取り替わる。期限が切れたら前の値を出し、そのアカウントに切り替えたときに取り直す。

## 7. 既知の課題

- `.credentials.json` と `oauthAccount` の置き場所は、Claude Code の公式の決まりではない。Claude Code の更新で置き場所が変わると、切り替えが効かなくなる。
- 再読み込みしない Claude（ほかのウィンドウなど）は、前のアカウントのログイン情報を持ったまま動く。その Claude が鍵を取り替えると、`.credentials.json` を前のアカウントで書き戻すことがある。切り替えたら、Claude を開いているウィンドウを全部再読み込みする。
- 保存ファイルは、もとの `.credentials.json` と同じく暗号化していない。
- **`/logout` すると、そのアカウントの保存した鍵も無効になる**（2026-11-19 に 401「OAuth access token has been revoked」で確かめた）。保存したあと、別のアカウントでログインするときに、`/logout` せずに済む方法はまだ作っていない（`/login` だけで前の鍵が無効になるかも、まだ確かめていない）。
- 使用量の API（`/api/oauth/usage`）も公式に公開された決まりではない。返事の形が変わると、使用量が出なくなる。
- 使っていないアカウントの値は、鍵の期限（数時間。`.credentials.json` の `expiresAt`）が切れると取り直せない。そのアカウントをほかの所（別のPC・claude.ai）で使っていると、出ている残りより実際は少ないことがある。

## 関連するページ

- [Claudeの会話の引き継ぎの設計](../Claudeの会話の引き継ぎ/Claudeの会話の引き継ぎの設計.md)（会話ファイルの引き継ぎ。アカウントの保存は引き継がない）
