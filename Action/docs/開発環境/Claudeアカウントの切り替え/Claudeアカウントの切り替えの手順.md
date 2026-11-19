# Claudeアカウントの切り替えの手順

設計: [Claudeアカウントの切り替えの設計](Claudeアカウントの切り替えの設計.md)
経緯: [Claudeアカウントの切り替えの経緯](Claudeアカウントの切り替えの経緯.md)

VSCode のアクティビティバーに「Claude アカウント」のアイコンを足し、保存しておいたログイン情報を入れ替えて Claude のアカウントを切り替える拡張機能を、一から作る手順です。

**出来上がるもの:** アクティビティバーのアイコンを押すと、保存したアカウントの一覧が出ます。今のアカウントを「+」で保存し、一覧の名前を押すとそのアカウントに切り替わります。ブラウザでのログインは、アカウントごとに最初の1回だけです。

**前提:**

- VSCode 1.80 以降。拡張機能は JavaScript で書き、VSCode がそのまま読むので、ビルドや Node.js のインストールは要らない。
- Claude Code（VSCode の拡張機能）で、切り替えたいアカウントにそれぞれ1回ログインできる。
- 新しく作る `.ps1` は UTF-8（BOM 付き）、`.json`・`.js`・`.svg` は UTF-8（BOM なし）で保存する。Windows の PowerShell 5.1 は、BOM のない `.ps1` の日本語を読み違えるため。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | アクティビティバーのアイコンと、空の一覧（`package.json`・`accounts.svg`） | 手順2で入れてから見る |
| 2 | このPCの VSCode に入れるスクリプト | アイコンが出る |
| 3 | 保存したアカウントの一覧を出す（`extension.js`） | エラーなく一覧が開く |
| 4 | 今のアカウントを保存する | 保存したアカウントが一覧に出る |
| 5 | アカウントを切り替える | 押したアカウントに替わる |
| 6 | 保存を消す | 一覧から消える |

**読み方:**

- 新しいファイルは、小さく始めて少しずつ足します。
- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる名前を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、この手順で作るもの・VSCode の拡張機能の決まり・Node.js の標準ライブラリ・JavaScript の書き方・PowerShell のどれかがわかります。
- [前提知識](../../前提知識.md) は C++ が中心で、JavaScript・PowerShell・VSCode の拡張機能は載っていないので、知らない前提で書きます。C++ と同じ意味のもの（`if`・`return`・`class`・`!`・`&&` など）は載せません。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 新しく作る（あとの手順で書き足す） | 1・3・4・5・6 |
| [accounts.svg](../../../../.claude/extensions/claude-accounts/media/accounts.svg) | 新しく作る | 1 |
| [install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1) | 新しく作る | 2 |
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 新しく作る（あとの手順で書き足す） | 3・4・5・6 |

VSCode の外のファイルで、拡張機能が読み書きするものは次の3つです（どれもリポジトリには入れません）。

| ファイル | 中身 | 書くもの |
|---|---|---|
| `%USERPROFILE%\.claude\.credentials.json` | ログイン情報（Claude のサーバーに入るための鍵） | Claude Code（手順5で、この拡張機能も書く） |
| `%USERPROFILE%\.claude.json` | Claude Code の設定。`oauthAccount` に、だれでログインしているか（メールアドレスなど）が入っている | Claude Code（手順5で、この拡張機能も `oauthAccount` だけを書く） |
| `%USERPROFILE%\.claude\account-profiles\〇〇.json` | 保存したアカウント1つ分（名前・`oauthAccount`・ログイン情報） | この拡張機能（手順4） |

---

## 手順1: アクティビティバーにアイコンを出す

VSCode の拡張機能は、`package.json` に「画面のどこに何を足すか」を書き、`extension.js` に「押されたら何をするか」を書きます。この手順では `package.json` だけで、アクティビティバーのアイコンと、押すと開く空の一覧を作ります。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 新しく作る（1-2 で骨組み、1-3 でアイコンと一覧を足す） |
| [accounts.svg](../../../../.claude/extensions/claude-accounts/media/accounts.svg) | 新しく作る（1-4） |

### 1-1. フォルダを作る

リポジトリの `.claude` の下に `extensions\claude-accounts` フォルダを作り、その中に `media` フォルダを作ります。拡張機能のファイルは全部 `claude-accounts` の中に置きます。

### 1-2. package.json の骨組みを書く

拡張機能の名前と、動く VSCode の版を書きます。VSCode に足すもの（`contributes`）は、まだ空にしておきます。

[package.json](../../../../.claude/extensions/claude-accounts/package.json) に、次の10行を書きます（1〜10行目）。

<!-- new: .claude/extensions/claude-accounts/package.json -->
<table class="code-lines"><tr><td>

```json
1
2
3
4
5
6
7
8
9
10
```

</td><td>

```json
{
  "name": "claude-accounts",
  "displayName": "Claude アカウント",
  "description": "保存した Claude のログイン情報を入れ替えて、アカウントを切り替える",
  "version": "0.0.1",
  "publisher": "local",
  "engines": { "vscode": "^1.80.0" },
  "contributes": {
  }
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 2 | 拡張機能の名前（英小文字）。6行目の `publisher` とつないだ `local.claude-accounts` が、この拡張機能の ID になる |
| 3 | 拡張機能の一覧に出る名前 |
| 4 | 拡張機能の一覧に出る説明 |
| 5 | 版。手順2のフォルダの名前にも使う |
| 6 | 作った人の名前。ストアに出さないので `local` にする |
| 7 | 動く VSCode の版。`^1.80.0` は「1.80.0 以降の 1.x」 |
| 8〜9 | VSCode に足すもの（画面の部品・コマンド）を、この `{ }` の中に書く。1-3 から足す |

- **なぜ:** VSCode は、拡張機能のフォルダにある `package.json` を読んで、拡張機能があることを知る。名前と版がないと拡張機能として読まれない。
- **効果:** 手順2で入れたとき、VSCode が `local.claude-accounts` という拡張機能として読む。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `package.json` | 拡張機能があることを VSCode に知らせるため | 拡張機能の名前・版・画面に足すものを書く JSON のファイル | [VSCode の拡張機能の決まり（Extension Manifest）](https://code.visualstudio.com/api/references/extension-manifest) |
| 値 | `name`・`displayName`・`description`・`version` | 拡張機能を見分ける・一覧に出すため | 名前（ID の後ろ半分）・表示名・説明・版 | 同上 |
| 値 | `publisher` | 拡張機能を見分けるため | 作った人の名前。`name` とつないで ID になる | 同上 |
| 値 | `engines` | 古い VSCode で動かさないため | 動く VSCode の版を書く | 同上 |
| 値 | `contributes` | VSCode の画面に部品を足すため | アクティビティバーのアイコン・一覧・コマンド・ボタンなどを書く | 同上 |

### 1-3. アイコンと一覧を足す

アクティビティバーにアイコンを1つ足し、押すと開く欄（サイドバー）に、アカウントの一覧を1つ置きます。

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **8行目** `"contributes": {` の下、`}` の上に、10行足します（9〜18行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 9 -->
<table class="code-lines"><tr><td>

```json
9
10
11
12
13
14
15
16
17
18
```

</td><td>

```json
    "viewsContainers": {
      "activitybar": [
        { "id": "claudeAccounts", "title": "Claude アカウント", "icon": "media/accounts.svg" }
      ]
    },
    "views": {
      "claudeAccounts": [
        { "id": "claudeAccounts.list", "name": "アカウント" }
      ]
    }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 9〜13 | アクティビティバーに入れ物を1つ足す。`id` は入れ物の名前、`title` はアイコンにマウスを乗せたときの名前、`icon` は 1-4 で作る絵 |
| 14〜18 | 入れ物 `claudeAccounts` の中に、一覧を1つ置く。`claudeAccounts.list` の名前で、手順3 から中身（アカウント）を渡す |

- **なぜ:** アクティビティバーのアイコンは「入れ物（`viewsContainers`）」、その中の一覧は「ビュー（`views`）」として別々に書く決まりのため。
- **効果:** アクティビティバーにアイコンが出て、押すと「アカウント」の欄が開く。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `viewsContainers` | アクティビティバーにアイコンを足すため | 入れ物（アイコンと、押すと開くサイドバー）を書く | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.viewsContainers) |
| 値 | `activitybar` | 入れ物を置く場所を決めるため | アクティビティバー（VSCode の左端のアイコンの列）に置く | 同上 |
| 値 | `claudeAccounts` | 入れ物を見分けるため | アクティビティバーに足す入れ物の名前。14行目でこの名前を使う | この手順で作る |
| 値 | `views` | 入れ物の中に一覧を置くため | 入れ物の名前ごとに、置くビュー（一覧）を書く | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.views) |
| 値 | `claudeAccounts.list` | 一覧を見分けるため | アカウントの一覧のビューの名前。`extension.js` からこの名前で中身を渡す | この手順で作る |

### 1-4. アイコンの絵を作る

アクティビティバーに出す絵を SVG で作ります。

[accounts.svg](../../../../.claude/extensions/claude-accounts/media/accounts.svg) に、次の6行を書きます（1〜6行目）。

<!-- new: .claude/extensions/claude-accounts/media/accounts.svg -->
<table class="code-lines"><tr><td>

```xml
1
2
3
4
5
6
```

</td><td>

```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
  <circle cx="8" cy="8" r="3"/>
  <path d="M2.5 19c0-3.2 2.5-5.5 5.5-5.5s5.5 2.3 5.5 5.5"/>
  <path d="M15 6h6l-2-2"/>
  <path d="M21 10h-6l2 2"/>
</svg>
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 1 | 24×24 の絵。線の色は `currentColor` にする |
| 2〜3 | 人（頭と肩） |
| 4〜5 | 入れ替えを表す、右向きと左向きの矢印 |

- **なぜ:** アクティビティバーのアイコンは、VSCode がテーマの色で塗る。色を決めずに `currentColor` にすると、明るいテーマでも暗いテーマでも見える。
- **効果:** アクティビティバーに、ほかのアイコンと同じ色の「人と矢印」の絵が出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `currentColor` | テーマの色で線を描くため | 線の色を、まわりの文字の色と同じにする | SVG の書き方 |

**確かめ方:** まだ VSCode に入れていないので、手順2のあとで確かめます。

---

## 手順2: このPCの VSCode に入れる

VSCode は `%USERPROFILE%\.vscode\extensions` の中のフォルダを拡張機能として読みます。そこに、リポジトリの `claude-accounts` フォルダを指す**ジャンクション**（Windows の、フォルダへのリンク）を作るスクリプトを書きます。コピーではなくリンクにするので、git pull で拡張機能が新しくなっても入れ直しは要りません（ウィンドウの再読み込みだけで効きます）。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1) | 新しく作る（2-2〜2-5） |

### 2-1. ファイルを作る

`.claude\scripts` に `install-claude-accounts.ps1` を作ります。UTF-8（BOM 付き）で保存します。

### 2-2. ジャンクションを作る

リポジトリのフォルダと、VSCode の拡張機能フォルダの中に作るリンクの場所を決め、ジャンクションを作ります。

[install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1) に、次の8行を書きます（1〜8行目）。

<!-- new: .claude/scripts/install-claude-accounts.ps1 -->
<table class="code-lines"><tr><td>

```powershell
1
2
3
4
5
6
7
8
```

</td><td>

```powershell
# Claude アカウントの切り替え（.claude/extensions/claude-accounts）を、このPCの VSCode に入れる。
# VSCode の拡張機能フォルダに、リポジトリのフォルダを指すジャンクションを作る（git pull すれば新しい版になる）。

$source = Join-Path $PSScriptRoot '..\extensions\claude-accounts' | Resolve-Path
$link = Join-Path $env:USERPROFILE '.vscode\extensions\local.claude-accounts-0.0.1'
New-Item -ItemType Junction -Path $link -Target $source | Out-Null
Write-Output "入れました: $link -> $source"
Write-Output 'VSCode のウィンドウを再読み込みすると、アクティビティバーに「Claude アカウント」が出ます。'
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 4 | このスクリプトのフォルダ（`.claude\scripts`）から `..\extensions\claude-accounts` をたどり、`C:\` から始まる形のパスにする |
| 5 | リンクを作る場所。フォルダの名前は `publisher.name-version`（1-2 の `local`・`claude-accounts`・`0.0.1`） |
| 6 | 5行目の場所に、4行目のフォルダを指すジャンクションを作る。作ったことを知らせる文は出さない |
| 7〜8 | 入れた場所と、次にすることを出す |

- **なぜ:** ジャンクションは管理者の権限なしで作れる。リンクにしておけば、リポジトリの中身を変えるたびにコピーし直さなくてよい。
- **効果:** スクリプトを1回動かすと、VSCode の拡張機能フォルダから、リポジトリの拡張機能が見える。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `$source` | リンク先を覚えておくため | リポジトリの `claude-accounts` フォルダのパス | この手順で作る |
| 変数 | `$PSScriptRoot` | スクリプトがどこから動いても、リポジトリのフォルダを探せるようにするため | 動いているスクリプトが置いてあるフォルダのパス | PowerShell |
| コマンド | `Join-Path` | パスをつなぐため | 2つのパスを `\` でつなぐ | PowerShell |
| コマンド | `Resolve-Path` | `..` を含まないパスにするため | `..` などをたどった、`C:\` から始まる本当のパスにする | PowerShell |
| 変数 | `$link` | リンクを作る場所を覚えておくため | `%USERPROFILE%\.vscode\extensions\local.claude-accounts-0.0.1` | この手順で作る |
| 環境変数 | `$env:USERPROFILE` | ユーザーのフォルダを探すため | `C:\Users\<ユーザー名>` | Windows |
| コマンド | `New-Item` | ジャンクションを作るため | ファイル・フォルダ・リンクを作る。`-ItemType Junction` でジャンクションを作り、`-Path` が作る場所、`-Target` が指す先 | PowerShell |
| コマンド | `Out-Null` | 余計な表示を出さないため | 前のコマンドの結果を捨てる | PowerShell |
| コマンド | `Write-Output` | 結果を知らせるため | 文を表示する | PowerShell |

### 2-3. 前に作ったリンクを消してから作る

2回目に動かすと、すでにリンクがあるので `New-Item` が失敗します。先に古いリンクだけを消します。

[install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1) の **5行目** `$link = Join-Path $env:USERPROFILE '.vscode\extensions\local.claude-accounts-0.0.1'` の下、`New-Item -ItemType Junction -Path $link -Target $source | Out-Null` の上に、5行足します（6〜10行目になります）。

<!-- add: .claude/scripts/install-claude-accounts.ps1 6 -->
<table class="code-lines"><tr><td>

```powershell
6
7
8
9
10
```

</td><td>

```powershell

if (Test-Path $link) {
    # ジャンクションだけを消す（-Recurse を付けないので、リポジトリの中身は消えない）
    (Get-Item $link).Delete()
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 7 | リンクがもうあるときだけ |
| 9 | リンクそのものを消す |

- **なぜ:** `Remove-Item -Recurse` で消すと、リンクの先（リポジトリの中身）まで消えることがある。`Delete()` はリンクだけを消す。
- **効果:** 何度動かしても、新しいリンクに作り直せる（ほかの作業用フォルダから動かしたときに、指す先を替えられる）。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| コマンド | `Test-Path` | リンクがあるかを調べるため | そのパスにファイル・フォルダがあれば `$true` を返す | PowerShell |
| コマンド | `Get-Item` | リンクを消すため | そのパスのファイル・フォルダを取り出す | PowerShell |
| 関数 | `Delete()` | リンクだけを消すため | 取り出したフォルダ（ここではジャンクション）を消す。中身はたどらない | .NET（PowerShell の中で動く）の `DirectoryInfo` |

### 2-4. 外すための引数を受け取る

あとで拡張機能を外せるように、`-Uninstall` を付けて動かせるようにします。

[install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1) の **2行目** `# VSCode の拡張機能フォルダに、リポジトリのフォルダを指すジャンクションを作る（git pull すれば新しい版になる）。` の下、（空の行） の上に、2行足します（3〜4行目になります）。

<!-- add: .claude/scripts/install-claude-accounts.ps1 3 -->
<table class="code-lines"><tr><td>

```powershell
3
4
```

</td><td>

```powershell
# -Uninstall で外す。どちらも、そのあと VSCode のウィンドウを再読み込みすると効く。
param([switch]$Uninstall)
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 3 | 使い方の説明 |
| 4 | `-Uninstall` を付けて動かしたかを受け取る |

- **なぜ:** 外すときに、ジャンクションを手で探して消さなくて済むようにする。
- **効果:** 2-5 で、`-Uninstall` を付けたら外すだけにできる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `param(...)` | スクリプトが受け取る引数を決めるため | スクリプトの先頭に書くと、`-名前` で値を渡せる（C++ の関数の引数に当たる） | PowerShell の書き方 |
| 引数 | `[switch]$Uninstall` | 外すときだけ付けてもらうため | `[switch]` は、付けたら `$true`、付けなければ `$false` になる引数 | この手順で作る |

### 2-5. 外すときは作らずに終わる

[install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1) の **12行目** `}` の下、`New-Item -ItemType Junction -Path $link -Target $source | Out-Null` の上に、4行足します（13〜16行目になります）。

<!-- add: .claude/scripts/install-claude-accounts.ps1 13 -->
<table class="code-lines"><tr><td>

```powershell
13
14
15
16
```

</td><td>

```powershell
if ($Uninstall) {
    Write-Output '外しました。VSCode のウィンドウを再読み込みしてください。'
    return
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13〜16 | `-Uninstall` のときは、2-3 で古いリンクを消したところで終わる |

- **なぜ:** 消す処理（2-3）は入れるときと同じなので、作る前で止めるだけで外せる。
- **効果:** `-Uninstall` を付けて動かすと、拡張機能が外れる。

### 2-6. 動かして入れる

リポジトリのフォルダで、PowerShell から次を動かします。

```powershell
powershell -ExecutionPolicy Bypass -File .claude\scripts\install-claude-accounts.ps1
```

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 引数 | `-ExecutionPolicy Bypass` | スクリプトを動かせるようにするため | PowerShell のスクリプトの実行を制限する設定を、今回だけ無視する | Windows の PowerShell（`powershell.exe`） |
| 引数 | `-File` | 動かすスクリプトを指すため | 指した `.ps1` を動かす | 同上 |

**確かめ方:**

1. `入れました: ...` と出る。
2. `code --list-extensions` を動かすと、`local.claude-accounts` が出る。
3. VSCode で `Ctrl+Shift+P` →「Developer: Reload Window」（ウィンドウの再読み込み）を選ぶと、アクティビティバーに人と矢印のアイコンが出る。押すと「アカウント」の欄が開く（中身を渡していないので、英語で「データを出すものがない」という文が出る）。

---

## 手順3: 保存したアカウントの一覧を出す

`extension.js` を作り、`~/.claude/account-profiles/` に保存したアカウントを読んで、1-3 の一覧に出します。保存はまだできないので（手順4）、この手順では一覧は空のままです。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 追加（3-1 で、動かすファイルの名前） |
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 新しく作る（3-2〜3-8） |

### 3-1. 動かすファイルを書く

VSCode に、拡張機能の動き（JavaScript）がどのファイルにあるかを知らせます。

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **7行目** `"engines": { "vscode": "^1.80.0" },` の下、`"contributes": {` の上に、1行足します（8行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 8 -->
<table class="code-lines"><tr><td>

```json
8
```

</td><td>

```json
  "main": "./extension.js",
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 8 | 拡張機能の動きは `extension.js` に書く |

- **なぜ:** `main` がないと、VSCode は画面の部品（手順1）を出すだけで、JavaScript を読まない。
- **効果:** 一覧を開いたときに、VSCode が `extension.js` を読んで動かす。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `main` | 拡張機能の動きを読ませるため | 最初に読む JavaScript のファイル | [VSCode の拡張機能の決まり（Extension Manifest）](https://code.visualstudio.com/api/references/extension-manifest) |

### 3-2. 使う道具と、ファイルの場所を決める

`extension.js` を作り、使う道具（VSCode・ファイル・パス）を読み込んで、Claude のログイン情報と保存先のパスを決めます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) に、次の12行を書きます（1〜12行目）。

<!-- new: .claude/extensions/claude-accounts/extension.js -->
<table class="code-lines"><tr><td>

```js
1
2
3
4
5
6
7
8
9
10
11
12
```

</td><td>

```js
// Claude アカウントの切り替え。
// ログイン情報（~/.claude/.credentials.json）と、だれでログインしているか（~/.claude.json の oauthAccount）を
// アカウントごとに ~/.claude/account-profiles/ へ保存しておき、押されたら入れ替える。
const vscode = require('vscode');
const fs = require('fs');
const os = require('os');
const path = require('path');

const HOME = os.homedir();
const CREDENTIALS_PATH = path.join(HOME, '.claude', '.credentials.json');
const CONFIG_PATH = path.join(HOME, '.claude.json');
const PROFILES_DIR = path.join(HOME, '.claude', 'account-profiles');
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 1〜3 | このファイルが何をするかの説明 |
| 4 | VSCode の画面を動かす道具を読み込む |
| 5〜7 | ファイルを読み書きする道具・PC の情報を取る道具・パスをつなぐ道具を読み込む |
| 9 | ユーザーのフォルダ（`C:\Users\<ユーザー名>`） |
| 10 | Claude のログイン情報のファイル |
| 11 | Claude の設定のファイル（だれでログインしているかが入っている） |
| 12 | 保存したアカウントを置くフォルダ |

- **なぜ:** パスを1か所で決めておけば、あとの関数はこの名前で読める。名前は定数の決まりどおり、全部大文字を `_` でつなぐ。
- **効果:** あとの手順で、どのファイルを読み書きするかがひと目でわかる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `const` | 変えない値に名前を付けるため | 名前を作り、あとで別の値を入れられないようにする。型は書かない（入れた値で決まる） | JavaScript の書き方 |
| 関数 | `require` | 道具を読み込むため | 名前を渡すと、その道具（モジュール）を返す。C++ の `#include` に近い | Node.js（VSCode の拡張機能は Node.js で動く） |
| 変数 | `vscode` | VSCode の画面を動かすため | VSCode の拡張機能の API。一覧・コマンド・メッセージなどを出す | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api) |
| 変数 | `fs` | ファイルを読み書きするため | ファイルの読み書き・消す・名前を変えるなどの関数を持つ | Node.js の標準ライブラリ（[fs](https://nodejs.org/api/fs.html)） |
| 変数 | `os` | ユーザーのフォルダを知るため | PC の情報を取る関数を持つ | Node.js の標準ライブラリ（[os](https://nodejs.org/api/os.html)） |
| 変数 | `path` | パスをつなぐため | パスをつなぐ・名前を取り出すなどの関数を持つ | Node.js の標準ライブラリ（[path](https://nodejs.org/api/path.html)） |
| 変数 | `HOME` | ファイルの場所の元にするため | ユーザーのフォルダのパス | この手順で作る |
| 関数 | `os.homedir()` | ユーザーのフォルダを知るため | `C:\Users\<ユーザー名>` を返す | Node.js の標準ライブラリ（os） |
| 変数 | `CREDENTIALS_PATH` | ログイン情報を読み書きするため | `~/.claude/.credentials.json` のパス | この手順で作る |
| 関数 | `path.join` | パスをつなぐため | 渡したものを `\` でつないで1つのパスにする | Node.js の標準ライブラリ（path） |
| 変数 | `CONFIG_PATH` | だれでログインしているかを読み書きするため | `~/.claude.json` のパス | この手順で作る |
| 変数 | `PROFILES_DIR` | 保存したアカウントを置くため | `~/.claude/account-profiles` のパス | この手順で作る |

### 3-3. JSON を読む関数と、今のアカウントを知る関数を足す

Claude のファイルはどれも JSON なので、読む関数を1つ作ります。それを使って、今だれでログインしているかを返す関数を作ります。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **12行目** `const PROFILES_DIR = path.join(HOME, '.claude', 'account-profiles');` の下に、13行足します（13〜25行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 13 -->
<table class="code-lines"><tr><td>

```js
13
14
15
16
17
18
19
20
21
22
23
24
25
```

</td><td>

```js

function readJson(file) {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

// 今ログインしているアカウント（~/.claude.json の oauthAccount）。ログインしていなければ null。
function currentAccount() {
  try {
    return readJson(CONFIG_PATH).oauthAccount || null;
  } catch {
    return null;
  }
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 14〜16 | ファイルを文字として読み、JSON として中身を取り出して返す |
| 19 | 今ログインしているアカウントを返す |
| 21 | `~/.claude.json` の `oauthAccount` を返す。ないときは `null` を返す（`||` は、左がないときに右を使う） |
| 22〜23 | ファイルがない・読めないときも `null` を返す |

- **なぜ:** `oauthAccount` には、アカウントごとに変わらない番号（`accountUuid`）とメールアドレスが入っている。これで「今のアカウントが保存したどれか」を見分けられる。
- **効果:** あとの手順で、今のアカウントに印を付けたり（3-7）、保存したり（手順4）できる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `function` | 関数を作るため | `function 名前(引数) { }` で関数を作る。戻り値と引数の型は書かない | JavaScript の書き方 |
| 関数 | `readJson` | JSON のファイルを読むため | ファイルを読み、中身（オブジェクト）を返す | この手順で作る |
| 関数 | `JSON.parse` | 文字を中身にするため | JSON の文字を、名前で値を引けるオブジェクトにする | JavaScript の標準 |
| 関数 | `fs.readFileSync` | ファイルを読むため | ファイルを全部読む。`'utf8'` を渡すと文字で返す | Node.js の標準ライブラリ（fs） |
| 関数 | `currentAccount` | 今のアカウントを知るため | `oauthAccount` を返す。ログインしていなければ `null` | この手順で作る |
| C++ の書き方 | `try { } catch { }` | 読めないときに止まらないようにするため | `try` の中で失敗（例外）が起きたら、`catch` の中を動かす | JavaScript の書き方 |
| 値 | `oauthAccount` | だれでログインしているかを知るため | `~/.claude.json` の中の、アカウントの番号（`accountUuid`）・メールアドレス（`emailAddress`）など | Claude Code が書く |
| 値 | `null` | 「ない」を表すため | 値がないことを表す（C++ の `nullptr` に近い） | JavaScript の書き方 |

### 3-4. 保存したアカウントを読む関数を足す

`account-profiles` の中の JSON を全部読み、名前の順に並べて返します。1つのファイルに、アカウント1つ分（名前・`oauthAccount`・ログイン情報）が入っています（書くのは手順4）。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **25行目** `}` の下に、9行足します（26〜34行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 26 -->
<table class="code-lines"><tr><td>

```js
26
27
28
29
30
31
32
33
34
```

</td><td>

```js

// 保存したアカウントの一覧。1つのファイルに { name, oauthAccount, credentials } が入っている。
function loadProfiles() {
  if (!fs.existsSync(PROFILES_DIR)) return [];
  return fs.readdirSync(PROFILES_DIR)
    .filter((f) => f.endsWith('.json'))
    .map((f) => ({ file: path.join(PROFILES_DIR, f), ...readJson(path.join(PROFILES_DIR, f)) }))
    .sort((a, b) => a.name.localeCompare(b.name));
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 29 | フォルダがまだなければ（1つも保存していなければ）、空の配列を返す |
| 30 | フォルダの中のファイルの名前を全部取る |
| 31 | `.json` で終わるものだけにする |
| 32 | ファイルを読み、中身に、ファイルのパス（`file`）を足したものにする |
| 33 | 名前（`name`）の順に並べる |

- **なぜ:** 消す（手順6）ときにファイルのパスが要るので、読んだときに `file` として持たせておく。
- **効果:** 保存したアカウントの一覧が、名前の順に取れる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `loadProfiles` | 保存したアカウントを全部読むため | `{ file, name, oauthAccount, credentials }` の配列を返す | この手順で作る |
| 関数 | `fs.existsSync` | フォルダがあるかを調べるため | あれば `true` を返す | Node.js の標準ライブラリ（fs） |
| 関数 | `fs.readdirSync` | フォルダの中を見るため | 中のファイルの名前の配列を返す | Node.js の標準ライブラリ（fs） |
| 関数 | `filter` | 要るものだけ残すため | 配列の1つずつに関数を当て、`true` を返したものだけの配列を作る | JavaScript の標準（配列） |
| C++ の書き方 | `(f) => ...` | その場で小さな関数を作るため | 引数 `f` を受け取り、`=>` の右を返す関数（C++ のラムダ式に近い） | JavaScript の書き方 |
| 関数 | `endsWith` | `.json` のファイルを選ぶため | 文字がその文字で終わっていれば `true` | JavaScript の標準（文字） |
| 関数 | `map` | 1つずつ作り替えるため | 配列の1つずつに関数を当て、返ったものの配列を作る | JavaScript の標準（配列） |
| 値 | `file` | 保存のファイルを消す・書き直すため | 保存したファイルのパス。読んだときに足す | この手順で作る |
| C++ の書き方 | `{ a: 1, ...b }` | 中身を混ぜたオブジェクトを作るため | `{ }` でオブジェクトを作る。`...b` は、`b` の中身を全部その場に並べる | JavaScript の書き方 |
| 関数 | `sort` | 名前の順に並べるため | 渡した関数で2つを比べて並べ替える | JavaScript の標準（配列） |
| 値 | `name` | 一覧に出す名前 | 保存するときに付けた名前（手順4） | この手順で作る |
| 関数 | `localeCompare` | 名前を比べるため | 文字を辞書の順で比べ、前なら負、後なら正を返す | JavaScript の標準（文字） |

### 3-5. 一覧に中身を渡すクラスを作る

VSCode の一覧は、決まった名前の関数を持つクラスから中身をもらいます。まず、中身が変わったことを VSCode に知らせる仕組みだけを持つクラスを作ります。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **34行目** `}` の下に、8行足します（35〜42行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 35 -->
<table class="code-lines"><tr><td>

```js
35
36
37
38
39
40
41
42
```

</td><td>

```js

// アクティビティバーに出す一覧。今のアカウントには印を付ける。
class AccountsProvider {
  constructor() {
    this.changed = new vscode.EventEmitter();
    this.onDidChangeTreeData = this.changed.event;
  }
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 37 | 一覧に中身を渡すクラス |
| 38〜41 | コンストラクタ。「変わった」と知らせる道具を作り、VSCode が見る名前（`onDidChangeTreeData`）で置く |

- **なぜ:** 保存や切り替えのあとに一覧を描き直してもらうには、VSCode に「中身が変わった」と知らせる道具が要る。VSCode はこの名前の値を見に来る。
- **効果:** 3-6 の `refresh()` で、一覧を描き直せるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `AccountsProvider` | 一覧に中身を渡すため | 保存したアカウントを、一覧の1行ずつにして VSCode に渡す | この手順で作る |
| 関数 | `constructor` | 作ったときに用意するため | クラスのコンストラクタ。`new` で作ったときに呼ばれる | JavaScript の書き方 |
| C++ の書き方 | `this.名前 = ...` | メンバー変数を作るため | JavaScript のクラスでは、宣言せずに `this.名前` に入れるとメンバー変数ができる | JavaScript の書き方 |
| メンバー変数 | `changed` | 「変わった」と知らせるため | `vscode.EventEmitter` | この手順で作る |
| 型 | `vscode.EventEmitter` | 出来事を知らせるため | `fire()` を呼ぶと、`event` に登録した相手（ここでは VSCode）に知らせる | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#EventEmitter) |
| メンバー変数 | `onDidChangeTreeData` | VSCode に変わったことを伝えるため | この名前にしておくと、VSCode が登録して、知らせが来たら一覧を描き直す | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#TreeDataProvider)（`TreeDataProvider` の決まり） |
| メンバー変数 | `event` | 知らせを受け取る相手を登録するため | `EventEmitter` が持つ、登録の口 | VSCode の拡張機能の API（`EventEmitter`） |

### 3-6. 描き直す関数と、1行を渡す関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **41行目** `}` の下、`}` の上に、8行足します（42〜49行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 42 -->
<table class="code-lines"><tr><td>

```js
42
43
44
45
46
47
48
49
```

</td><td>

```js

  refresh() {
    this.changed.fire();
  }

  getTreeItem(item) {
    return item;
  }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 43〜45 | 「変わった」と知らせて、一覧を描き直してもらう |
| 47〜49 | VSCode に聞かれた1行を、そのまま返す（3-7 で、もう1行の形にして渡すため） |

- **なぜ:** `getTreeItem` は、VSCode が1行ずつ「どう見せるか」を聞いてくる関数で、必ず要る。3-7 で最初から見せる形（`TreeItem`）を作るので、受け取ったものを返すだけでよい。
- **効果:** 手順4〜6 で、保存・切り替え・消したあとに `refresh()` を呼べば一覧が新しくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `refresh` | 一覧を描き直すため | `changed.fire()` を呼ぶ | この手順で作る |
| 関数 | `fire` | 知らせを送るため | 登録した相手に、出来事を知らせる | VSCode の拡張機能の API（`EventEmitter`） |
| 関数 | `getTreeItem` | 1行の見せ方を VSCode に渡すため | VSCode が1行ずつ呼ぶ。見せ方（`TreeItem`）を返す | VSCode の拡張機能の API（`TreeDataProvider` の決まり） |

### 3-7. 一覧の中身を作る関数を足す

保存したアカウントを1行ずつにします。今ログインしているアカウントには、印と「（使用中）」を付けます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **49行目** `}` の下、`}` の上に、11行足します（50〜60行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 50 -->
<table class="code-lines"><tr><td>

```js
50
51
52
53
54
55
56
57
58
59
60
```

</td><td>

```js

  getChildren() {
    const current = currentAccount();
    return loadProfiles().map((p) => {
      const isCurrent = current && p.oauthAccount.accountUuid === current.accountUuid;
      const item = new vscode.TreeItem(p.name);
      item.description = p.oauthAccount.emailAddress + (isCurrent ? '（使用中）' : '');
      item.iconPath = new vscode.ThemeIcon(isCurrent ? 'pass-filled' : 'account');
      return item;
    });
  }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 51 | VSCode が一覧を描くときに呼ぶ。1行ずつの配列を返す |
| 52 | 今のアカウントを取る（3-3） |
| 53 | 保存したアカウントを1つずつ、1行に作り替える |
| 54 | 番号（`accountUuid`）が今のアカウントと同じなら、使用中 |
| 55 | 保存した名前を見出しにした1行を作る |
| 56 | 見出しの右に、メールアドレスを薄く出す。使用中なら「（使用中）」を足す |
| 57 | 左の絵。使用中なら丸の中にチェック、それ以外は人の絵 |
| 58 | 作った1行を返す |

- **なぜ:** アカウントは番号（`accountUuid`）で見分ける。メールアドレスや名前は変わることがあるが、番号は変わらない。
- **効果:** 一覧に、保存したアカウントが名前の順に並び、今のアカウントがひと目でわかる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `getChildren` | 一覧の中身を VSCode に渡すため | VSCode が一覧を描くときに呼ぶ。1行ずつの配列を返す | VSCode の拡張機能の API（`TreeDataProvider` の決まり） |
| 変数 | `isCurrent` | 使用中かを覚えておくため | 保存したアカウントが、今ログインしているアカウントなら `true` | この手順で作る |
| 値 | `accountUuid` | アカウントを見分けるため | アカウントごとに決まった、変わらない番号 | Claude Code が書く（`oauthAccount` の中） |
| C++ の書き方 | `===` | 同じかを比べるため | 型も中身も同じなら `true`（C++ の `==` に当たる） | JavaScript の書き方 |
| 型 | `vscode.TreeItem` | 一覧の1行を作るため | 見出し・説明・絵・押したときのコマンドを持つ | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#TreeItem) |
| メンバー変数 | `description` | メールアドレスを出すため | 見出しの右に薄く出る文 | VSCode の拡張機能の API（`TreeItem`） |
| 値 | `emailAddress` | どのアカウントかを人に見せるため | アカウントのメールアドレス | Claude Code が書く（`oauthAccount` の中） |
| メンバー変数 | `iconPath` | 使用中を絵で見せるため | 行の左に出す絵 | VSCode の拡張機能の API（`TreeItem`） |
| 型 | `vscode.ThemeIcon` | VSCode に入っている絵を使うため | 名前を渡すと、その絵（[Codicon](https://code.visualstudio.com/api/references/icons-in-labels#icon-listing)）になる。`'pass-filled'` は丸の中のチェック、`'account'` は人 | VSCode の拡張機能の API |

### 3-8. 拡張機能が動き出したときに、一覧に中身を渡す

VSCode は、一覧を開いたときに `extension.js` の `activate` を呼びます。そこで、3-5〜3-7 のクラスを一覧 `claudeAccounts.list` に渡します。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **61行目** `}` の下に、11行足します（62〜72行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 62 -->
<table class="code-lines"><tr><td>

```js
62
63
64
65
66
67
68
69
70
71
72
```

</td><td>

```js

function activate(context) {
  const provider = new AccountsProvider();
  context.subscriptions.push(
    vscode.window.registerTreeDataProvider('claudeAccounts.list', provider),
  );
}

function deactivate() {}

module.exports = { activate, deactivate };
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 63 | 拡張機能が動き出したときに VSCode が呼ぶ |
| 64 | 一覧に中身を渡すクラスを1つ作る |
| 65〜67 | 一覧 `claudeAccounts.list` の中身を、63行目のクラスからもらうように登録する。登録したものは `subscriptions` に入れておき、拡張機能が止まるときに VSCode に片付けてもらう |
| 70 | 拡張機能が止まるときに VSCode が呼ぶ。片付けは `subscriptions` に任せるので、中身はない |
| 72 | `activate` と `deactivate` を、VSCode から呼べるようにする |

- **なぜ:** 拡張機能の入り口は `activate` と決まっている。登録を `subscriptions` に入れておけば、止めるときの片付けを自分で書かなくてよい。
- **効果:** 一覧を開くと、保存したアカウントが並ぶ（まだ保存できないので、空のまま）。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `activate` | 拡張機能を動き出させるため | 拡張機能が動き出したときに、VSCode が1回呼ぶ | VSCode の拡張機能の決まり |
| 引数 | `context` | 拡張機能の情報を受け取るため | 片付けを任せる `subscriptions` などを持つ | VSCode の拡張機能の API（`ExtensionContext`） |
| 変数 | `provider` | 一覧に中身を渡すクラスを持っておくため | `AccountsProvider` | この手順で作る |
| C++ の書き方 | `new` | クラスを作るため | C++ と同じくクラスを作る。消すのは JavaScript が自動でする（`delete` は要らない） | JavaScript の書き方 |
| メンバー変数 | `subscriptions` | 片付けを任せるため | ここに入れたものを、拡張機能が止まるときに VSCode が片付ける | VSCode の拡張機能の API（`ExtensionContext`） |
| 関数 | `push` | 配列に足すため | 配列の最後に足す（`std::vector` の `push_back` に当たる） | JavaScript の標準（配列） |
| 関数 | `vscode.window.registerTreeDataProvider` | 一覧に中身を渡すクラスを登録するため | ビューの名前と、`getChildren` などを持つクラスを結びつける | VSCode の拡張機能の API |
| 関数 | `deactivate` | 拡張機能を止めるため | 拡張機能が止まるときに、VSCode が呼ぶ | VSCode の拡張機能の決まり |
| 変数 | `module.exports` | 関数をファイルの外から呼べるようにするため | ここに入れたものを、`require` した側（VSCode）が使える | Node.js |

**確かめ方:** VSCode のウィンドウを再読み込みし（2-6）、アイコンを押すと、英語の文が消えて空の一覧になる。`Ctrl+Shift+U`（出力）で「拡張機能ホスト」を選び、`claude-accounts` のエラーが出ていない。

---

## 手順4: 今のアカウントを保存する

一覧の上に「+」のボタンを出し、押すと今ログインしているアカウントのログイン情報を、名前を付けて `account-profiles` に保存します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 追加（4-1 でコマンド、4-8 でボタン、4-9 で一覧が空のときの文） |
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 追加（4-2〜4-7） |

### 4-1. 保存と読み直しのコマンドを決める

ボタンから呼ぶものは、先に「コマンド」として名前と絵を決めます。保存（`+`）と、一覧の読み直し（くるっと回る矢印）の2つです。

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **9行目** `"contributes": {` の下、`"viewsContainers": {` の上に、4行足します（10〜13行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 10 -->
<table class="code-lines"><tr><td>

```json
10
11
12
13
```

</td><td>

```json
    "commands": [
      { "command": "claudeAccounts.save", "title": "今のアカウントを保存", "icon": "$(add)" },
      { "command": "claudeAccounts.refresh", "title": "一覧を読み直す", "icon": "$(refresh)" }
    ],
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 10 | コマンドの一覧 |
| 11 | 保存のコマンド。ボタンの絵は `+` |
| 12 | 読み直しのコマンド。ボタンの絵は回る矢印 |
| 13 | 一覧の終わり。次の `"viewsContainers"` があるので `,` を付ける |

- **なぜ:** VSCode のボタンは、コマンドの名前を指して作る。コマンドを先に決めておくと、ボタン（4-8）や一覧が空のときの文（4-9）から同じ名前で呼べる。
- **効果:** `claudeAccounts.save`・`claudeAccounts.refresh` の名前で、4-7 で動きを結びつけられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `commands` | コマンドを決めるため | コマンドの名前・見出し・絵を並べる | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.commands) |
| 値 | `command`・`title`・`icon` | コマンドを決めるため | コマンドの名前・マウスを乗せたときの見出し・ボタンの絵 | 同上 |
| 値 | `$(add)`・`$(refresh)` | ボタンの絵にするため | `$(名前)` で、VSCode に入っている絵（Codicon）を使う。`add` は `+`、`refresh` は回る矢印 | 同上 |
| 値 | `claudeAccounts.save`・`claudeAccounts.refresh` | コマンドを見分けるため | 保存と読み直しのコマンドの名前 | この手順で作る |

### 4-2. 書きかけを読まれない JSON の書き方を足す

保存と切り替えでは、Claude Code も読むファイルを書きます。書いている途中のファイルを読まれないように、別の名前に書いてから置き換える関数を作ります。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **34行目** `}` の下、（空の行） の上に、7行足します（35〜41行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 35 -->
<table class="code-lines"><tr><td>

```js
35
36
37
38
39
40
41
```

</td><td>

```js

// 書きかけのファイルを Claude に読まれないよう、別の名前に書いてから置き換える。
function writeJson(file, value) {
  const temp = file + '.tmp';
  fs.writeFileSync(temp, JSON.stringify(value, null, 2), 'utf8');
  fs.renameSync(temp, file);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 37 | 書くファイルと、書く中身を受け取る |
| 38 | 書くファイルの名前の後ろに `.tmp` を付けた名前 |
| 39 | 中身を、2文字の字下げを付けた JSON の文字にして、`.tmp` に書く |
| 40 | `.tmp` を本当の名前に変える（もとのファイルは置き換わる） |

- **なぜ:** 本当の名前に直接書くと、書いている途中に Claude Code が読んで、壊れた JSON を読むことがある。名前を変えるのは一瞬で済むので、読む側は「前の中身」か「新しい中身」のどちらかしか見ない。
- **効果:** 保存・切り替えの途中で、Claude Code がログイン情報を読み違えない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `writeJson` | JSON のファイルを書くため | 中身を JSON にして、`.tmp` に書いてから置き換える | この手順で作る |
| 変数 | `temp` | 書きかけを置くため | `〇〇.json.tmp` のパス | この手順で作る |
| 関数 | `fs.writeFileSync` | ファイルを書くため | 文字をファイルに書く（あれば上書き） | Node.js の標準ライブラリ（fs） |
| 関数 | `JSON.stringify` | 中身を文字にするため | オブジェクトを JSON の文字にする。3つ目の `2` は字下げの文字数 | JavaScript の標準 |
| 関数 | `fs.renameSync` | 書いたファイルを本当の名前にするため | ファイルの名前を変える。同じ名前があれば置き換える | Node.js の標準ライブラリ（fs） |

### 4-3. 保存したアカウントを番号で探す関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **41行目** `}` の下、（空の行） の上に、4行足します（42〜45行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 42 -->
<table class="code-lines"><tr><td>

```js
42
43
44
45
```

</td><td>

```js

function findProfile(accountUuid) {
  return loadProfiles().find((p) => p.oauthAccount.accountUuid === accountUuid);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 43〜45 | 保存したアカウントのうち、番号（`accountUuid`）が同じものを返す。なければ `undefined` |

- **なぜ:** 同じアカウントを2回保存したときに、ファイルを2つ作らず、前のファイルに上書きするため。
- **効果:** 4-4（保存）と手順5（切り替え）で、今のアカウントが保存済みかを調べられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `findProfile` | 保存したアカウントを番号で探すため | 見つかれば `{ file, name, oauthAccount, credentials }`、なければ `undefined` を返す | この手順で作る |
| 引数 | `accountUuid` | 探す番号を受け取るため | アカウントの番号 | 使う側が決める |
| 関数 | `find` | 配列から1つ探すため | 関数が `true` を返した最初の1つを返す。なければ `undefined` | JavaScript の標準（配列） |
| 値 | `undefined` | 「見つからない」を表すため | 値が入っていないことを表す。`if` では `false` と同じに扱われる | JavaScript の書き方 |

### 4-4. 今のログイン情報を保存する関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **45行目** `}` の下、（空の行） の上に、9行足します（46〜54行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 46 -->
<table class="code-lines"><tr><td>

```js
46
47
48
49
50
51
52
53
54
```

</td><td>

```js

// 今のログイン情報を、そのアカウントの保存ファイルに書く（ないときは作る）。
function saveCurrent(name) {
  const account = currentAccount();
  const saved = findProfile(account.accountUuid);
  const file = saved ? saved.file : path.join(PROFILES_DIR, account.accountUuid + '.json');
  fs.mkdirSync(PROFILES_DIR, { recursive: true });
  writeJson(file, { name, oauthAccount: account, credentials: readJson(CREDENTIALS_PATH) });
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 48 | 一覧に出す名前を受け取る |
| 49 | 今のアカウントを取る |
| 50 | そのアカウントが保存済みかを探す |
| 51 | 保存済みならそのファイルに、まだなら `番号.json` に書く |
| 52 | 保存のフォルダがなければ作る |
| 53 | 名前・`oauthAccount`・ログイン情報（`.credentials.json` の中身）を1つのファイルに書く |

- **なぜ:** 切り替えには、ログイン情報（`.credentials.json`）と、だれのものか（`oauthAccount`）の両方が要る。片方だけを替えると、Claude Code が別の人の名前で別の人のログイン情報を使うことになる。
- **効果:** 今のアカウントを、あとで戻せる形で取っておける。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `saveCurrent` | 今のアカウントを保存するため | 今のログイン情報と `oauthAccount` を、名前を付けて `account-profiles` に書く | この手順で作る |
| 変数 | `account`・`saved` | 今のアカウントと、その保存を覚えておくため | `currentAccount()`・`findProfile()` の結果 | この手順で作る |
| 関数 | `fs.mkdirSync` | 保存のフォルダを作るため | フォルダを作る。`{ recursive: true }` を渡すと、途中のフォルダも作り、もうあっても失敗しない | Node.js の標準ライブラリ（fs） |
| C++ の書き方 | `{ name, oauthAccount: account }` | 名前と値をまとめるため | `name` だけを書くと、`name: name`（同じ名前の変数を入れる）の略になる | JavaScript の書き方 |
| 値 | `credentials` | ログイン情報を持っておくため | `.credentials.json` の中身そのもの | この手順で作る |

### 4-5. 保存のボタンを押したときの関数を作る

まず、ログインしていないときは知らせて終わるところまでを書きます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **81行目** `}` の下、（空の行） の上に、8行足します（82〜89行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 82 -->
<table class="code-lines"><tr><td>

```js
82
83
84
85
86
87
88
89
```

</td><td>

```js

async function onSave(provider) {
  const account = currentAccount();
  if (!account || !fs.existsSync(CREDENTIALS_PATH)) {
    vscode.window.showWarningMessage('Claude にログインしていません。Claude で /login してから保存してください。');
    return;
  }
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 83 | 保存のボタンを押したときに呼ぶ。描き直しのために一覧のクラスを受け取る |
| 84〜88 | ログインしていない（`oauthAccount` かログイン情報のファイルがない）ときは、黄色の知らせを出して終わる |

- **なぜ:** ログインしていないのに保存すると、中身のないアカウントが一覧に出てしまう。
- **効果:** ログインしていないときは「/login してから」と知らせる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `async` | 中で `await`（4-6）を使うため | 関数を「途中で人の入力を待てる関数」にする | JavaScript の書き方 |
| 関数 | `onSave` | 保存のボタンの動き | 名前を聞いて、今のアカウントを保存する | この手順で作る |
| 関数 | `vscode.window.showWarningMessage` | 注意を知らせるため | 右下に黄色の知らせを出す | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#window.showWarningMessage) |

### 4-6. 名前を聞いて保存する

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **88行目** `}` の下、`}` の上に、8行足します（89〜96行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 89 -->
<table class="code-lines"><tr><td>

```js
89
90
91
92
93
94
95
96
```

</td><td>

```js
  const saved = findProfile(account.accountUuid);
  const name = await vscode.window.showInputBox({
    prompt: `${account.emailAddress} を一覧に出す名前`,
    value: saved ? saved.name : account.emailAddress,
  });
  if (!name) return;
  saveCurrent(name);
  provider.refresh();
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 89 | 保存済みかを探す |
| 90〜93 | 画面の上に入力欄を出し、一覧に出す名前を聞く。初めの値は、保存済みならその名前、まだならメールアドレス |
| 94 | `Esc` で閉じた・空にしたときは保存しない |
| 95〜96 | 保存して、一覧を描き直す |

- **なぜ:** メールアドレスだけだと、どのアカウントか（仕事用・自分用など）がわかりにくいので、名前を付けられるようにする。保存済みのアカウントでもう一度押せば、名前を付け直せる。
- **効果:** 押すと名前を聞かれ、`account-profiles` にファイルができて一覧に出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `name` | 聞いた名前を覚えておくため | 入力欄に打った名前。閉じたら `undefined` | この手順で作る |
| C++ の書き方 | `await` | 人が入力し終わるのを待つため | 右の処理が終わるまで待ち、その結果を受け取る。待つ間も VSCode は止まらない | JavaScript の書き方 |
| 関数 | `vscode.window.showInputBox` | 名前を聞くため | 画面の上に入力欄を出し、打った文字を返す | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#window.showInputBox) |
| 値 | `prompt`・`value` | 入力欄に出すため | 入力欄の下の説明と、初めから入っている文字 | VSCode の拡張機能の API（`InputBoxOptions`） |
| C++ の書き方 | `` `${x} を…` `` | 文字に値を埋め込むため | `` ` `` で囲んだ文字の中の `${ }` に、値が入る | JavaScript の書き方 |

### 4-7. コマンドに動きを結びつける

4-1 で決めたコマンドを押したときに、4-5〜4-6 の関数と 3-6 の `refresh` が動くようにします。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **102行目** `vscode.window.registerTreeDataProvider('claudeAccounts.list', provider),` の下、`);` の上に、2行足します（103〜104行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 103 -->
<table class="code-lines"><tr><td>

```js
103
104
```

</td><td>

```js
    vscode.commands.registerCommand('claudeAccounts.save', () => onSave(provider)),
    vscode.commands.registerCommand('claudeAccounts.refresh', () => provider.refresh()),
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 103 | `claudeAccounts.save` が呼ばれたら、`onSave` を動かす |
| 104 | `claudeAccounts.refresh` が呼ばれたら、一覧を描き直す |

- **なぜ:** `package.json` のコマンドは名前だけなので、動きは `registerCommand` で結びつける。読み直しは、ほかの窓で切り替えたあとなどに使う。
- **効果:** コマンドが呼べるようになる（ボタンは 4-8 で出す）。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `vscode.commands.registerCommand` | コマンドに動きを結びつけるため | コマンドの名前と、呼ばれたときに動かす関数を結びつける | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#commands.registerCommand) |

### 4-8. 一覧の上にボタンを出す

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **13行目** `],` の下、`"viewsContainers": {` の上に、6行足します（14〜19行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 14 -->
<table class="code-lines"><tr><td>

```json
14
15
16
17
18
19
```

</td><td>

```json
    "menus": {
      "view/title": [
        { "command": "claudeAccounts.save", "when": "view == claudeAccounts.list", "group": "navigation" },
        { "command": "claudeAccounts.refresh", "when": "view == claudeAccounts.list", "group": "navigation" }
      ]
    },
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 14 | ボタン（メニュー）の一覧 |
| 15〜18 | 一覧 `claudeAccounts.list` の見出しの右に、保存と読み直しのボタンを出す |
| 19 | 次の `"viewsContainers"` があるので `,` を付ける |

- **なぜ:** `navigation` の組に入れると、`…` のメニューの中ではなく、見出しの右に絵のボタンとして出る。
- **効果:** 一覧の見出しの右に「+」と回る矢印が出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `menus` | ボタンを出すため | どこに、どのコマンドのボタン・メニューを出すかを書く | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.menus) |
| 値 | `view/title` | 一覧の見出しにボタンを出すため | ビューの見出しの所 | 同上 |
| 値 | `when` | どのビューに出すかを決めるため | 条件を書く。`view == claudeAccounts.list` は「この一覧のとき」 | 同上 |
| 値 | `group` | ボタンの出し方を決めるため | `navigation` にすると、見出しの右に絵で出る | 同上 |

### 4-9. 一覧が空のときに、保存の案内を出す

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **19行目** `},` の下、`"viewsContainers": {` の上に、6行足します（20〜25行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 20 -->
<table class="code-lines"><tr><td>

```json
20
21
22
23
24
25
```

</td><td>

```json
    "viewsWelcome": [
      {
        "view": "claudeAccounts.list",
        "contents": "保存したアカウントがありません。\nClaude で /login してから、今のアカウントを保存してください。\n[今のアカウントを保存](command:claudeAccounts.save)"
      }
    ],
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 20〜25 | 一覧 `claudeAccounts.list` が空のとき、説明の文と、保存のコマンドを呼ぶボタンを出す（`\n` で行を分ける） |

- **なぜ:** 初めて開いたときに一覧が空だと、何をすればよいかがわからない。
- **効果:** 1つも保存していないときに「Claude で /login してから、今のアカウントを保存してください。」と、保存のボタンが出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `viewsWelcome` | 空の一覧に案内を出すため | ビューが空のときに出す文を書く。`[文字](command:コマンド名)` と書くと、そのコマンドを呼ぶボタンになる | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.viewsWelcome) |
| 値 | `view`・`contents` | どこに何を出すかを決めるため | 出すビューの名前と、出す文 | 同上 |

**確かめ方:**

1. ウィンドウを再読み込みし、アイコンを押すと、案内の文と「今のアカウントを保存」のボタンが出る。
2. ボタン（または見出しの「+」）を押し、名前を入れて `Enter` を押すと、一覧にその名前とメールアドレス、「（使用中）」とチェックの絵が出る。
3. `%USERPROFILE%\.claude\account-profiles\` に、`番号.json` が1つできている。

---

## 手順5: アカウントを切り替える

一覧の名前を押すと、そのアカウントの保存したログイン情報を、Claude Code が読む2つのファイルに書き戻します。書き戻す前に、今のアカウントの最新のログイン情報を保存し直します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 追加（5-1 でコマンド、5-8 でコマンドパレットに出さない設定） |
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 追加（5-2〜5-7） |

### 5-1. 切り替えのコマンドを決める

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **10行目** `"commands": [` の下、`{ "command": "claudeAccounts.save", "title": "今のアカウントを保存", "icon": "$(add)" },` の上に、1行足します（11行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 11 -->
<table class="code-lines"><tr><td>

```json
11
```

</td><td>

```json
      { "command": "claudeAccounts.switch", "title": "このアカウントに切り替える" },
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 11 | 切り替えのコマンド。一覧の行を押したときに呼ぶので、ボタンの絵はない |

- **なぜ:** 一覧の行を押したときに動かすものも、コマンドとして名前を付ける決まりのため。
- **効果:** `claudeAccounts.switch` の名前で、5-6・5-7 から使える。

### 5-2. 保存したログイン情報を書き戻す関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **54行目** `}` の下、（空の行） の上に、8行足します（55〜62行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 55 -->
<table class="code-lines"><tr><td>

```js
55
56
57
58
59
60
61
62
```

</td><td>

```js

// 保存したアカウントのログイン情報を、Claude が読む2つのファイルに書き戻す。
function applyProfile(profile) {
  writeJson(CREDENTIALS_PATH, profile.credentials);
  const config = readJson(CONFIG_PATH);
  config.oauthAccount = profile.oauthAccount;
  writeJson(CONFIG_PATH, config);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 57 | 書き戻すアカウント（`loadProfiles` の1つ）を受け取る |
| 58 | ログイン情報のファイルを、保存した中身で置き換える |
| 59〜61 | `~/.claude.json` を読み、`oauthAccount` だけを保存したものに替えて書く |

- **なぜ:** `~/.claude.json` には、ほかの設定（開いたプロジェクトなど）もたくさん入っているので、全部は置き換えず、`oauthAccount` だけを替える。
- **効果:** Claude Code が次に読んだとき、切り替えた先のアカウントでログインしていることになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `applyProfile` | 保存したアカウントに切り替えるため | ログイン情報と `oauthAccount` を、Claude Code が読むファイルに書く | この手順で作る |
| 変数 | `config` | `~/.claude.json` の中身を持っておくため | 読んだ設定。`oauthAccount` だけを替えて書き戻す | この手順で作る |

### 5-3. 一覧を押したときの関数を作る

まず、押したのが今のアカウントなら、何もしないところまでを書きます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **105行目** `}` の下、（空の行） の上に、5行足します（106〜110行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 106 -->
<table class="code-lines"><tr><td>

```js
106
107
108
109
110
```

</td><td>

```js

async function onSwitch(provider, profile) {
  const current = currentAccount();
  if (current && current.accountUuid === profile.oauthAccount.accountUuid) return;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 107 | 一覧のクラスと、押したアカウントを受け取る |
| 108 | 今のアカウントを取る |
| 109 | 押したのが今のアカウントなら、何もしない |

- **なぜ:** 同じアカウントに切り替えても意味がなく、再読み込みを勧める知らせが出るだけになるため。
- **効果:** 使用中の行を押しても何も起きない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `onSwitch` | 一覧の行を押したときの動き | 今のアカウントを保存し直してから、押したアカウントに切り替える | この手順で作る |
| 引数 | `profile` | 切り替える先を受け取るため | 押した行のアカウント（`loadProfiles` の1つ） | 5-6 で一覧の行から渡す |
| 変数 | `current` | 今のアカウントを覚えておくため | `currentAccount()` の結果 | この手順で作る |

### 5-4. 切り替える前に、今のアカウントを保存し直す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **109行目** `if (current && current.accountUuid === profile.oauthAccount.accountUuid) return;` の下、`}` の上に、10行足します（110〜119行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 110 -->
<table class="code-lines"><tr><td>

```js
110
111
112
113
114
115
116
117
118
119
```

</td><td>

```js
  // 今のアカウントの最新のログイン情報を先に取っておく（使っている間に Claude が更新しているため）。
  const saved = current && findProfile(current.accountUuid);
  if (saved) {
    saveCurrent(saved.name);
  } else if (current) {
    const answer = await vscode.window.showWarningMessage(
      `${current.emailAddress} は保存されていません。切り替えると、戻すときにもう一度ログインが要ります。`,
      { modal: true }, '切り替える');
    if (answer !== '切り替える') return;
  }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 110 | なぜ保存し直すかの説明 |
| 111 | 今のアカウントが保存済みかを探す |
| 112〜113 | 保存済みなら、今のログイン情報で保存し直す（名前はそのまま） |
| 114〜119 | 保存していないアカウントなら、「戻すときにもう一度ログインが要る」と画面の中央で聞き、「切り替える」を押さなければやめる |

- **なぜ:** Claude Code は、使っている間にログイン情報（鍵）を新しいものに取り替え、古い鍵は使えなくなる。保存したときの古い鍵のままにしておくと、戻したときにログインし直しになる。
- **効果:** 何度行き来しても、どのアカウントもログインし直さずに戻れる。保存していないアカウントを、うっかり失わない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `{ modal: true }` | 必ず答えてもらうため | 知らせを右下ではなく画面の中央に出し、答えるまでほかを触れなくする | VSCode の拡張機能の API（`MessageOptions`） |
| 変数 | `answer` | 押したボタンを知るため | 押したボタンの文字（`'切り替える'`）。閉じたら `undefined` | この手順で作る |

### 5-5. 書き戻して、再読み込みを勧める

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **119行目** `}` の下、`}` の上に、6行足します（120〜125行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 120 -->
<table class="code-lines"><tr><td>

```js
120
121
122
123
124
125
```

</td><td>

```js
  applyProfile(profile);
  provider.refresh();
  const answer = await vscode.window.showInformationMessage(
    `${profile.name} に切り替えました。開いている Claude に効かせるには、ウィンドウを再読み込みしてください。`,
    '再読み込み');
  if (answer === '再読み込み') vscode.commands.executeCommand('workbench.action.reloadWindow');
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 120 | 押したアカウントのログイン情報を書き戻す |
| 121 | 一覧を描き直す（「（使用中）」が移る） |
| 122〜124 | 切り替えたことと、再読み込みが要ることを、「再読み込み」のボタン付きで知らせる |
| 125 | 「再読み込み」を押したら、VSCode のウィンドウを読み込み直す |

- **なぜ:** 開いている Claude の会話は、動き出したときに読んだログイン情報を持ったままで、ファイルを替えても気づかない。読み込み直すと、新しいファイルを読み直す。
- **効果:** ボタンを1つ押すだけで、開いている Claude も切り替わる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `vscode.window.showInformationMessage` | 終わったことを知らせるため | 右下に青の知らせを出す。ボタンの文字を渡すと、押したボタンの文字を返す | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#window.showInformationMessage) |
| 関数 | `vscode.commands.executeCommand` | VSCode のコマンドを呼ぶため | 名前でコマンドを呼ぶ | VSCode の拡張機能の API |
| 値 | `'workbench.action.reloadWindow'` | ウィンドウを読み込み直すため | `Ctrl+Shift+P` の「Developer: Reload Window」と同じコマンド | VSCode に入っているコマンド |

### 5-6. 一覧の行を押したら切り替える

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **85行目** `item.iconPath = new vscode.ThemeIcon(isCurrent ? 'pass-filled' : 'account');` の下、`return item;` の上に、1行足します（86行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 86 -->
<table class="code-lines"><tr><td>

```js
86
```

</td><td>

```js
      item.command = { command: 'claudeAccounts.switch', title: '切り替える', arguments: [p] };
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 86 | 行を押したら `claudeAccounts.switch` を呼び、その行のアカウント `p` を渡す |

- **なぜ:** 行の `command` に入れたコマンドは、行を押したときに VSCode が `arguments` を付けて呼ぶ。
- **効果:** 一覧の名前を押すと、そのアカウントで `onSwitch` が動く（5-7 で結びつける）。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `command` | 行を押したときの動きを決めるため | `{ command: コマンド名, title, arguments: [渡すもの] }` を入れる | VSCode の拡張機能の API（`TreeItem`） |
| 値 | `arguments` | コマンドに渡すため | コマンドの関数に渡す引数の配列 | VSCode の拡張機能の API（`Command`） |

### 5-7. 切り替えのコマンドに動きを結びつける

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **134行目** `vscode.commands.registerCommand('claudeAccounts.refresh', () => provider.refresh()),` の下、`);` の上に、1行足します（135行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 135 -->
<table class="code-lines"><tr><td>

```js
135
```

</td><td>

```js
    vscode.commands.registerCommand('claudeAccounts.switch', (p) => onSwitch(provider, p)),
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 135 | `claudeAccounts.switch` が呼ばれたら、渡されたアカウント `p` で `onSwitch` を動かす |

- **なぜ:** 5-1 で決めたコマンドは名前だけなので、動きを結びつける。
- **効果:** 一覧の名前を押すと、そのアカウントに切り替わる。

### 5-8. コマンドパレットに出さない

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **15行目** `"menus": {` の下、`"view/title": [` の上に、3行足します（16〜18行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 16 -->
<table class="code-lines"><tr><td>

```json
16
17
18
```

</td><td>

```json
      "commandPalette": [
        { "command": "claudeAccounts.switch", "when": "false" }
      ],
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 16〜18 | `Ctrl+Shift+P`（コマンドパレット）の一覧に、切り替えのコマンドを出さない |

- **なぜ:** コマンドは、何もしないとコマンドパレットにも出る。切り替えは一覧の行から呼ばれる（どのアカウントかを渡す）ので、パレットから呼ばれると、渡すものがなくて失敗する。
- **効果:** コマンドパレットに、押しても動かない「このアカウントに切り替える」が出ない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `commandPalette` | コマンドパレットの出し方を決めるため | ここで `"when": "false"` にしたコマンドは、コマンドパレットに出ない | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.menus) |

**確かめ方:**

1. 2つ目のアカウントを保存する: Claude の会話で `/logout` のあと `/login` し、ブラウザで2つ目のアカウントを選ぶ。一覧の「+」で保存すると、一覧に2つ並び、2つ目に「（使用中）」が付く。
2. 1つ目の名前を押すと「〇〇 に切り替えました。」と出て、「（使用中）」が1つ目に移る。
3. 「再読み込み」を押し、Claude の会話で `/status` を打つと、1つ目のアカウントのメールアドレスが出る。ブラウザは開かない。

---

## 手順6: 保存を消す

一覧の行にマウスを乗せるとゴミ箱のボタンが出て、押すとそのアカウントの保存ファイルを消します。Claude からログアウトはしません。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 追加（6-1 でコマンド、6-5 で行のボタン、6-6 でコマンドパレットに出さない設定） |
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 追加（6-2〜6-4） |

### 6-1. 消すコマンドを決める

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **10行目** `"commands": [` の下、`{ "command": "claudeAccounts.switch", "title": "このアカウントに切り替える" },` の上に、1行足します（11行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 11 -->
<table class="code-lines"><tr><td>

```json
11
```

</td><td>

```json
      { "command": "claudeAccounts.remove", "title": "保存を消す", "icon": "$(trash)" },
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 11 | 保存を消すコマンド。ボタンの絵はゴミ箱 |

- **なぜ:** 行のボタン（6-5）は、コマンドを指して作るため。
- **効果:** `claudeAccounts.remove` の名前で、6-4・6-5 から使える。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `$(trash)` | ボタンの絵にするため | ゴミ箱の絵（Codicon） | VSCode の拡張機能の決まり |

### 6-2. 消す関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **127行目** `}` の下、（空の行） の上に、8行足します（128〜135行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 128 -->
<table class="code-lines"><tr><td>

```js
128
129
130
131
132
133
134
135
```

</td><td>

```js

async function onRemove(provider, item) {
  const answer = await vscode.window.showWarningMessage(
    `${item.profile.name} の保存を消しますか？（ログアウトはしません）`, { modal: true }, '消す');
  if (answer !== '消す') return;
  fs.unlinkSync(item.profile.file);
  provider.refresh();
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 129 | 一覧のクラスと、ボタンを押した行を受け取る |
| 130〜132 | 画面の中央で「消しますか？」と聞き、「消す」を押さなければやめる |
| 133 | その行のアカウントの保存ファイルを消す |
| 134 | 一覧を描き直す |

- **なぜ:** 消したら戻せない（そのアカウントは、もう一度ログインして保存し直すことになる）ので、必ず聞く。ログアウトはしないので、使用中のアカウントを消しても Claude はそのまま使える。
- **効果:** 要らなくなったアカウントを一覧から外せる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `onRemove` | ゴミ箱のボタンの動き | 確かめてから、保存ファイルを消す | この手順で作る |
| 引数 | `item` | 押した行を受け取るため | 行のボタンを押すと、VSCode がその行（`TreeItem`）を渡す | VSCode の拡張機能の API |
| 関数 | `fs.unlinkSync` | 保存ファイルを消すため | ファイルを消す | Node.js の標準ライブラリ（fs） |

### 6-3. 行に、どのアカウントかを持たせる

ゴミ箱のボタンからは、行（`TreeItem`）しか渡されません。行に、どのアカウントの行かを持たせておきます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **86行目** `item.command = { command: 'claudeAccounts.switch', title: '切り替える', arguments: [p] };` の下、`return item;` の上に、1行足します（87行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 87 -->
<table class="code-lines"><tr><td>

```js
87
```

</td><td>

```js
      item.profile = p;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 87 | 行に、そのアカウント（ファイルのパスも入っている）を持たせる |

- **なぜ:** `TreeItem` には決まったメンバー変数しかないので、消すファイルを知るために、自分で `profile` を足す。
- **効果:** 6-2 で、`item.profile.file` から消すファイルがわかる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `profile` | 行がどのアカウントかを知るため | その行の、保存したアカウント | この手順で作る |

### 6-4. 消すコマンドに動きを結びつける

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **144行目** `vscode.commands.registerCommand('claudeAccounts.switch', (p) => onSwitch(provider, p)),` の下、`);` の上に、1行足します（145行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 145 -->
<table class="code-lines"><tr><td>

```js
145
```

</td><td>

```js
    vscode.commands.registerCommand('claudeAccounts.remove', (item) => onRemove(provider, item)),
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 145 | `claudeAccounts.remove` が呼ばれたら、渡された行で `onRemove` を動かす |

- **なぜ:** 6-1 で決めたコマンドは名前だけなので、動きを結びつける。
- **効果:** 6-5 のボタンを押すと、消す動きが動く。

### 6-5. 行にゴミ箱のボタンを出す

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **16行目** `"menus": {` の下、`"commandPalette": [` の上に、3行足します（17〜19行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 17 -->
<table class="code-lines"><tr><td>

```json
17
18
19
```

</td><td>

```json
      "view/item/context": [
        { "command": "claudeAccounts.remove", "when": "view == claudeAccounts.list", "group": "inline" }
      ],
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 17〜19 | 一覧 `claudeAccounts.list` の各行の右に、ゴミ箱のボタンを出す |

- **なぜ:** `inline` の組に入れると、右クリックのメニューの中ではなく、マウスを乗せたときに行の右に絵のボタンで出る。
- **効果:** 行にマウスを乗せると、ゴミ箱のボタンが出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `view/item/context` | 一覧の行にボタンを出すため | ビューの各行の所 | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.menus) |
| 値 | `inline` | 行の右にボタンを出すため | `group` を `inline` にすると、マウスを乗せたときに行の右に絵で出る | 同上 |

### 6-6. 消すコマンドもコマンドパレットに出さない

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **20行目** `"commandPalette": [` の下、`{ "command": "claudeAccounts.switch", "when": "false" }` の上に、1行足します（21行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 21 -->
<table class="code-lines"><tr><td>

```json
21
```

</td><td>

```json
        { "command": "claudeAccounts.remove", "when": "false" },
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 21 | `Ctrl+Shift+P` の一覧に、消すコマンドを出さない |

- **なぜ:** 5-8 と同じで、行から呼ばれるコマンドなので、パレットから呼ぶと渡すものがない。
- **効果:** コマンドパレットに、押しても動かない「保存を消す」が出ない。

### 出来上がった package.json

<!-- full: .claude/extensions/claude-accounts/package.json -->
<table class="code-lines"><tr><td>

```json
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
```

</td><td>

```json
{
  "name": "claude-accounts",
  "displayName": "Claude アカウント",
  "description": "保存した Claude のログイン情報を入れ替えて、アカウントを切り替える",
  "version": "0.0.1",
  "publisher": "local",
  "engines": { "vscode": "^1.80.0" },
  "main": "./extension.js",
  "contributes": {
    "commands": [
      { "command": "claudeAccounts.remove", "title": "保存を消す", "icon": "$(trash)" },
      { "command": "claudeAccounts.switch", "title": "このアカウントに切り替える" },
      { "command": "claudeAccounts.save", "title": "今のアカウントを保存", "icon": "$(add)" },
      { "command": "claudeAccounts.refresh", "title": "一覧を読み直す", "icon": "$(refresh)" }
    ],
    "menus": {
      "view/item/context": [
        { "command": "claudeAccounts.remove", "when": "view == claudeAccounts.list", "group": "inline" }
      ],
      "commandPalette": [
        { "command": "claudeAccounts.remove", "when": "false" },
        { "command": "claudeAccounts.switch", "when": "false" }
      ],
      "view/title": [
        { "command": "claudeAccounts.save", "when": "view == claudeAccounts.list", "group": "navigation" },
        { "command": "claudeAccounts.refresh", "when": "view == claudeAccounts.list", "group": "navigation" }
      ]
    },
    "viewsWelcome": [
      {
        "view": "claudeAccounts.list",
        "contents": "保存したアカウントがありません。\nClaude で /login してから、今のアカウントを保存してください。\n[今のアカウントを保存](command:claudeAccounts.save)"
      }
    ],
    "viewsContainers": {
      "activitybar": [
        { "id": "claudeAccounts", "title": "Claude アカウント", "icon": "media/accounts.svg" }
      ]
    },
    "views": {
      "claudeAccounts": [
        { "id": "claudeAccounts.list", "name": "アカウント" }
      ]
    }
  }
}
```

</td></tr></table>

### 出来上がった extension.js

<!-- full: .claude/extensions/claude-accounts/extension.js -->
<table class="code-lines"><tr><td>

```js
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
47
48
49
50
51
52
53
54
55
56
57
58
59
60
61
62
63
64
65
66
67
68
69
70
71
72
73
74
75
76
77
78
79
80
81
82
83
84
85
86
87
88
89
90
91
92
93
94
95
96
97
98
99
100
101
102
103
104
105
106
107
108
109
110
111
112
113
114
115
116
117
118
119
120
121
122
123
124
125
126
127
128
129
130
131
132
133
134
135
136
137
138
139
140
141
142
143
144
145
146
147
148
149
150
151
```

</td><td>

```js
// Claude アカウントの切り替え。
// ログイン情報（~/.claude/.credentials.json）と、だれでログインしているか（~/.claude.json の oauthAccount）を
// アカウントごとに ~/.claude/account-profiles/ へ保存しておき、押されたら入れ替える。
const vscode = require('vscode');
const fs = require('fs');
const os = require('os');
const path = require('path');

const HOME = os.homedir();
const CREDENTIALS_PATH = path.join(HOME, '.claude', '.credentials.json');
const CONFIG_PATH = path.join(HOME, '.claude.json');
const PROFILES_DIR = path.join(HOME, '.claude', 'account-profiles');

function readJson(file) {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

// 今ログインしているアカウント（~/.claude.json の oauthAccount）。ログインしていなければ null。
function currentAccount() {
  try {
    return readJson(CONFIG_PATH).oauthAccount || null;
  } catch {
    return null;
  }
}

// 保存したアカウントの一覧。1つのファイルに { name, oauthAccount, credentials } が入っている。
function loadProfiles() {
  if (!fs.existsSync(PROFILES_DIR)) return [];
  return fs.readdirSync(PROFILES_DIR)
    .filter((f) => f.endsWith('.json'))
    .map((f) => ({ file: path.join(PROFILES_DIR, f), ...readJson(path.join(PROFILES_DIR, f)) }))
    .sort((a, b) => a.name.localeCompare(b.name));
}

// 書きかけのファイルを Claude に読まれないよう、別の名前に書いてから置き換える。
function writeJson(file, value) {
  const temp = file + '.tmp';
  fs.writeFileSync(temp, JSON.stringify(value, null, 2), 'utf8');
  fs.renameSync(temp, file);
}

function findProfile(accountUuid) {
  return loadProfiles().find((p) => p.oauthAccount.accountUuid === accountUuid);
}

// 今のログイン情報を、そのアカウントの保存ファイルに書く（ないときは作る）。
function saveCurrent(name) {
  const account = currentAccount();
  const saved = findProfile(account.accountUuid);
  const file = saved ? saved.file : path.join(PROFILES_DIR, account.accountUuid + '.json');
  fs.mkdirSync(PROFILES_DIR, { recursive: true });
  writeJson(file, { name, oauthAccount: account, credentials: readJson(CREDENTIALS_PATH) });
}

// 保存したアカウントのログイン情報を、Claude が読む2つのファイルに書き戻す。
function applyProfile(profile) {
  writeJson(CREDENTIALS_PATH, profile.credentials);
  const config = readJson(CONFIG_PATH);
  config.oauthAccount = profile.oauthAccount;
  writeJson(CONFIG_PATH, config);
}

// アクティビティバーに出す一覧。今のアカウントには印を付ける。
class AccountsProvider {
  constructor() {
    this.changed = new vscode.EventEmitter();
    this.onDidChangeTreeData = this.changed.event;
  }

  refresh() {
    this.changed.fire();
  }

  getTreeItem(item) {
    return item;
  }

  getChildren() {
    const current = currentAccount();
    return loadProfiles().map((p) => {
      const isCurrent = current && p.oauthAccount.accountUuid === current.accountUuid;
      const item = new vscode.TreeItem(p.name);
      item.description = p.oauthAccount.emailAddress + (isCurrent ? '（使用中）' : '');
      item.iconPath = new vscode.ThemeIcon(isCurrent ? 'pass-filled' : 'account');
      item.command = { command: 'claudeAccounts.switch', title: '切り替える', arguments: [p] };
      item.profile = p;
      return item;
    });
  }
}

async function onSave(provider) {
  const account = currentAccount();
  if (!account || !fs.existsSync(CREDENTIALS_PATH)) {
    vscode.window.showWarningMessage('Claude にログインしていません。Claude で /login してから保存してください。');
    return;
  }
  const saved = findProfile(account.accountUuid);
  const name = await vscode.window.showInputBox({
    prompt: `${account.emailAddress} を一覧に出す名前`,
    value: saved ? saved.name : account.emailAddress,
  });
  if (!name) return;
  saveCurrent(name);
  provider.refresh();
}

async function onSwitch(provider, profile) {
  const current = currentAccount();
  if (current && current.accountUuid === profile.oauthAccount.accountUuid) return;
  // 今のアカウントの最新のログイン情報を先に取っておく（使っている間に Claude が更新しているため）。
  const saved = current && findProfile(current.accountUuid);
  if (saved) {
    saveCurrent(saved.name);
  } else if (current) {
    const answer = await vscode.window.showWarningMessage(
      `${current.emailAddress} は保存されていません。切り替えると、戻すときにもう一度ログインが要ります。`,
      { modal: true }, '切り替える');
    if (answer !== '切り替える') return;
  }
  applyProfile(profile);
  provider.refresh();
  const answer = await vscode.window.showInformationMessage(
    `${profile.name} に切り替えました。開いている Claude に効かせるには、ウィンドウを再読み込みしてください。`,
    '再読み込み');
  if (answer === '再読み込み') vscode.commands.executeCommand('workbench.action.reloadWindow');
}

async function onRemove(provider, item) {
  const answer = await vscode.window.showWarningMessage(
    `${item.profile.name} の保存を消しますか？（ログアウトはしません）`, { modal: true }, '消す');
  if (answer !== '消す') return;
  fs.unlinkSync(item.profile.file);
  provider.refresh();
}

function activate(context) {
  const provider = new AccountsProvider();
  context.subscriptions.push(
    vscode.window.registerTreeDataProvider('claudeAccounts.list', provider),
    vscode.commands.registerCommand('claudeAccounts.save', () => onSave(provider)),
    vscode.commands.registerCommand('claudeAccounts.refresh', () => provider.refresh()),
    vscode.commands.registerCommand('claudeAccounts.switch', (p) => onSwitch(provider, p)),
    vscode.commands.registerCommand('claudeAccounts.remove', (item) => onRemove(provider, item)),
  );
}

function deactivate() {}

module.exports = { activate, deactivate };
```

</td></tr></table>

### 出来上がった install-claude-accounts.ps1

<!-- full: .claude/scripts/install-claude-accounts.ps1 -->
<table class="code-lines"><tr><td>

```powershell
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
```

</td><td>

```powershell
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
```

</td></tr></table>

**確かめ方（これで出来上がり）:**

1. ウィンドウを再読み込みし、一覧の行にマウスを乗せると、右にゴミ箱が出る。
2. 押して「消す」を押すと、一覧から消え、`account-profiles` のファイルも消える。Claude はログインしたまま使える。
3. `Ctrl+Shift+P` で「切り替える」「保存を消す」と打っても、この拡張機能のコマンドは出ない。
