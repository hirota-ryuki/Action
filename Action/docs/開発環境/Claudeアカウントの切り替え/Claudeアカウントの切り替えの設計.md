# Claudeアカウントの切り替えの設計

作成日: 2026-11-19
対象: [extension.js](../../../../.claude/extensions/claude-accounts/extension.js)・[package.json](../../../../.claude/extensions/claude-accounts/package.json)・[install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1)
経緯: [Claudeアカウントの切り替えの経緯](Claudeアカウントの切り替えの経緯.md)
手順: [Claudeアカウントの切り替えの手順](Claudeアカウントの切り替えの手順.md)

VSCode のアクティビティバーに「Claude アカウント」を足し、一覧の名前を押すだけで Claude Code のアカウントを切り替える、自分用の拡張機能です。

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
  └ 一覧（保存したアカウント。使用中に印）
      ├ 見出しの「+」 … 今のアカウントを保存（名前を付ける）
      ├ 名前を押す   … そのアカウントに切り替え → 「再読み込み」
      └ 行のゴミ箱   … 保存を消す（ログアウトはしない）
```

| ファイル | 中身 | 書くもの |
|---|---|---|
| `~/.claude/.credentials.json` | ログイン情報 | Claude Code・切り替え |
| `~/.claude.json` | Claude Code の設定。`oauthAccount` だけを切り替えで書く | Claude Code・切り替え |
| `~/.claude/account-profiles/<accountUuid>.json` | アカウント1つ分 `{ name, oauthAccount, credentials }` | 保存・切り替え |

`~` は `%USERPROFILE%`（`C:\Users\<ユーザー名>`）です。

## 2. 切り替えの流れ

1. 押したのが使用中のアカウントなら、何もしない。
2. 使用中のアカウントが保存済みなら、今の `.credentials.json` で**保存し直す**。保存していなければ「戻すときにもう一度ログインが要る」と聞き、やめることもできる。
3. 押したアカウントの `credentials` を `.credentials.json` に、`oauthAccount` を `~/.claude.json` に書く（`~/.claude.json` のほかの設定はそのまま）。
4. 「再読み込み」のボタン付きで知らせる。

2 で保存し直すのは、Claude Code が使っている間にリフレッシュトークンを新しいものに取り替えることがあり、そのとき古いものは使えなくなることがあるためです。保存した時点のままにしておくと、戻したときにログインし直しになります。

## 3. 実装方法

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

**一覧は `TreeDataProvider`。** `AccountsProvider` の `getChildren` が、保存したアカウントを1行（`vscode.TreeItem`）ずつにします。行を押すと `claudeAccounts.switch` がそのアカウントを受け取り、ゴミ箱のボタンは行（`item.profile` に持たせたアカウント）を受け取ります。保存・切り替え・消したあとは `refresh()`（`EventEmitter.fire`）で描き直します。

**入れ方はジャンクション。** [install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1) が `%USERPROFILE%\.vscode\extensions\local.claude-accounts-0.0.1` に、リポジトリの `.claude\extensions\claude-accounts` を指すジャンクションを作ります。git pull で中身が新しくなっても、入れ直さずに再読み込みだけで効きます。

## 4. 使い方

- **このPCに入れる**（PCごとに1回）: リポジトリのフォルダで `powershell -ExecutionPolicy Bypass -File .claude\scripts\install-claude-accounts.ps1` を動かし、`Ctrl+Shift+P` →「Developer: Reload Window」。外すときは `-Uninstall` を付ける。
- **アカウントを足す**: Claude の会話で `/login`（使用中なら `/logout` のあと）して、足したいアカウントでログインし、一覧の「+」で名前を付けて保存する。最初のアカウントも同じように「+」で保存する。
- **切り替える**: 一覧の名前を押し、「再読み込み」を押す。ほかの VSCode のウィンドウでも Claude を開いているときは、そちらも再読み込みする。
- **消す**: 行にマウスを乗せてゴミ箱を押す。保存ファイルが消えるだけで、ログアウトはしない。

## 5. 決めたこと

- **ログアウトしてログインし直す形にしない。** その形だと毎回ブラウザでアカウントを選ぶことになり、一覧はメモにしかならないため。
- **保存はPCごと。** ログイン情報をリポジトリや OneDrive に置くと、ほかの人や別のPCに鍵を渡すことになる。別のPCでは、そのPCで一度ずつログインして保存する。
- **VSCode のストア向けの形（`.vsix`）にしない。** 自分のPCで使うだけなので、ジャンクションで直接読ませる。

## 6. 既知の課題

- `.credentials.json` と `oauthAccount` の置き場所は、Claude Code の公式の決まりではない。Claude Code の更新で置き場所が変わると、切り替えが効かなくなる。
- 再読み込みしない Claude（ほかのウィンドウなど）は、前のアカウントのログイン情報を持ったまま動く。その Claude が鍵を取り替えると、`.credentials.json` を前のアカウントで書き戻すことがある。切り替えたら、Claude を開いているウィンドウを全部再読み込みする。
- 保存ファイルは、もとの `.credentials.json` と同じく暗号化していない。

## 関連するページ

- [Claudeの会話の引き継ぎの設計](../Claudeの会話の引き継ぎ/Claudeの会話の引き継ぎの設計.md)（会話ファイルの引き継ぎ。アカウントの保存は引き継がない）
