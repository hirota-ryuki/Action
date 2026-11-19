# Claudeアカウントの切り替えの手順

設計: [Claudeアカウントの切り替えの設計](Claudeアカウントの切り替えの設計.md)
経緯: [Claudeアカウントの切り替えの経緯](Claudeアカウントの切り替えの経緯.md)

VSCode のアクティビティバーに「Claude アカウント」のアイコンを足し、保存しておいたログイン情報を入れ替えて Claude のアカウントを切り替える拡張機能を、一から作る手順です。

**出来上がるもの:** アクティビティバーのアイコンを押すと、保存したアカウントが1つずつ枠（カード）で並びます。今のアカウントを「+」で保存し、枠の「このアカウントに切り替える」を押すと、そのアカウントに切り替わります。ブラウザでのログインは、アカウントごとに最初の1回だけです。枠は↑↓で並べ替えられます。各枠には、5時間の枠と週の枠の使用量がバーで出て、残り何%か・何時に復活するかもわかります。使用中のアカウントの使用量は、Claude の会話が進むたびに（30秒に1回まで）新しくなります。

**前提:**

- VSCode 1.82 以降。拡張機能は JavaScript で書き、VSCode がそのまま読むので、ビルドや Node.js のインストールは要らない。
- Claude Code（VSCode の拡張機能）で、切り替えたいアカウントにそれぞれ1回ログインできる。
- 新しく作る `.ps1` は UTF-8（BOM 付き）、`.json`・`.js`・`.css`・`.svg` は UTF-8（BOM なし）で保存する。Windows の PowerShell 5.1 は、BOM のない `.ps1` の日本語を読み違えるため。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | アクティビティバーのアイコンと、空の欄（`package.json`・`accounts.svg`） | 手順2で入れてから見る |
| 2 | このPCの VSCode に入れるスクリプト | アイコンが出る |
| 3 | 保存したアカウントを枠で並べる画面（`extension.js`・`view.js`・`view.css`） | エラーなく欄が開く |
| 4 | 今のアカウントを保存する | 保存したアカウントの枠が出る |
| 5 | アカウントを切り替える | 押したアカウントに替わる |
| 6 | 保存を消す | 枠が消える |
| 7 | 並べ替える | ↑↓で枠が入れ替わる |
| 8 | 使用量（5時間・週）のバーと、復活する時刻を出す | 枠にバーが出て、会話が進むと新しくなる |

**読み方:**

- 新しいファイルは、小さく始めて少しずつ足します。
- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる名前を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、この手順で作るもの・VSCode の拡張機能の決まり・Node.js の標準ライブラリ・JavaScript の書き方・ブラウザ（HTML・CSS）・PowerShell のどれかがわかります。
- [前提知識](../../前提知識.md) は C++ が中心で、JavaScript・CSS・PowerShell・VSCode の拡張機能は載っていないので、知らない前提で書きます。C++ と同じ意味のもの（`if`・`return`・`class`・`!`・`&&`・`? :` など）は載せません。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 新しく作る（あとの手順で書き足す） | 1・3・4・8 |
| [accounts.svg](../../../../.claude/extensions/claude-accounts/media/accounts.svg) | 新しく作る | 1 |
| [install-claude-accounts.ps1](../../../../.claude/scripts/install-claude-accounts.ps1) | 新しく作る | 2 |
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 新しく作る（あとの手順で書き足す）。ファイルを読み書きし、画面に中身を送る | 3〜8 |
| [view.js](../../../../.claude/extensions/claude-accounts/media/view.js) | 新しく作る（あとの手順で書き足す）。画面（Webview）の中で動き、枠を描く | 3〜8 |
| [view.css](../../../../.claude/extensions/claude-accounts/media/view.css) | 新しく作る（あとの手順で書き足す）。枠・ボタン・バーの見た目 | 3・4・6・8 |

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
  "engines": { "vscode": "^1.82.0" },
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
| 7 | 動く VSCode の版。`^1.82.0` は「1.82.0 以降の 1.x」。手順8で使う `fetch` が、この版から使える |
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
        { "type": "webview", "id": "claudeAccounts.list", "name": "アカウント" }
      ]
    }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 9〜13 | アクティビティバーに入れ物を1つ足す。`id` は入れ物の名前、`title` はアイコンにマウスを乗せたときの名前、`icon` は 1-4 で作る絵 |
| 14〜18 | 入れ物 `claudeAccounts` の中に、欄を1つ置く。`"type": "webview"` で、中身を Web ページ（Webview）で描く欄にする。`claudeAccounts.list` の名前で、手順3 から中身を渡す |

- **なぜ:** アクティビティバーのアイコンは「入れ物（`viewsContainers`）」、その中の欄は「ビュー（`views`）」として別々に書く決まりのため。VSCode の決まった形の一覧（木の形）では、バーや色付きの文字を描けないので、Web ページで描く欄にする。
- **効果:** アクティビティバーにアイコンが出て、押すと「アカウント」の欄が開く。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `viewsContainers` | アクティビティバーにアイコンを足すため | 入れ物（アイコンと、押すと開くサイドバー）を書く | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.viewsContainers) |
| 値 | `activitybar` | 入れ物を置く場所を決めるため | アクティビティバー（VSCode の左端のアイコンの列）に置く | 同上 |
| 値 | `claudeAccounts` | 入れ物を見分けるため | アクティビティバーに足す入れ物の名前。14行目でこの名前を使う | この手順で作る |
| 値 | `views` | 入れ物の中に一覧を置くため | 入れ物の名前ごとに、置くビュー（一覧）を書く | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.views) |
| 値 | `"type": "webview"` | Web ページで描く欄にするため | 書かないと、VSCode の決まった形の一覧（木の形）になる | 同上 |
| 値 | `claudeAccounts.list` | 欄を見分けるため | アカウントの欄のビューの名前。`extension.js` からこの名前で中身を渡す | この手順で作る |

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
3. VSCode で `Ctrl+Shift+P` →「Developer: Reload Window」（ウィンドウの再読み込み）を選ぶと、アクティビティバーに人と矢印のアイコンが出る。押すと「アカウント」の欄が開く（中身を渡していないので、空のまま）。

---

## 手順3: 保存したアカウントを枠で並べる画面を作る

`extension.js` を作り、`~/.claude/account-profiles/` に保存したアカウントを読んで、1-3 の欄に送ります。欄の中は Web ページ（Webview）で、`view.js` が届いた中身からアカウントごとの枠を描き、`view.css` で見た目を付けます。保存はまだできないので（手順4）、この手順では欄は空のままです。

```
extension.js（ファイルを読む） ──中身を送る（postMessage）──▶ view.js（枠を描く）
                               ◀──ボタンの知らせ（postMessage）──
```

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 追加（3-1 で、動かすファイルの名前） |
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 新しく作る（3-2〜3-12） |
| [view.js](../../../../.claude/extensions/claude-accounts/media/view.js) | 新しく作る（3-13〜3-16） |
| [view.css](../../../../.claude/extensions/claude-accounts/media/view.css) | 新しく作る（3-17〜3-22） |

### 3-1. 動かすファイルを書く

VSCode に、拡張機能の動き（JavaScript）がどのファイルにあるかを知らせます。

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **7行目** `"engines": { "vscode": "^1.82.0" },` の下、`"contributes": {` の上に、1行足します（8行目になります）。

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
- **効果:** あとの手順で、今のアカウントに印を付けたり（3-5）、保存したり（手順4）できる。

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

### 3-5. 画面に渡す中身を作る関数を足す

保存したアカウントを、画面に送る形（番号・名前・メールアドレス・使用中か）に作り替えます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **34行目** `}` の下に、14行足します（35〜48行目になります）。

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
43
44
45
46
47
48
```

</td><td>

```js

// 画面に渡す中身。アカウントを並び順に並べ、使用量を添える。
function viewData() {
  const current = currentAccount();
  const profiles = loadProfiles();
  const accounts = profiles.map((p) => ({
    id: p.oauthAccount.accountUuid,
    name: p.name,
    email: p.oauthAccount.emailAddress,
    isCurrent: !!current && p.oauthAccount.accountUuid === current.accountUuid,
  }));
  const data = { accounts };
  return data;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 38 | 今のアカウントを取る（3-3） |
| 39 | 保存したアカウントを読む（3-4） |
| 40 | 1つずつ、画面に送る形に作り替える |
| 41〜43 | 番号・名前・メールアドレス |
| 44 | 番号（`accountUuid`）が今のアカウントと同じなら、使用中 |
| 46 | 画面に送るものをまとめる（手順4で、保存していないアカウントのことも足す） |

- **なぜ:** 画面（Webview）の中ではファイルを読めないので、`extension.js` で読んで、要るものだけを送る。ログイン情報（鍵）は画面に送らない。アカウントは番号で見分ける。メールアドレスや名前は変わることがあるが、番号は変わらない。
- **効果:** 3-9 で、この中身を画面に送れる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `viewData` | 画面に渡す中身を作るため | `{ accounts }` を返す（手順4で `unsaved` も足す） | この手順で作る |
| 変数 | `current` | 今のアカウントを覚えておくため | `currentAccount()` の結果 | この手順で作る |
| 変数 | `profiles`・`accounts` | アカウントを持っておくため | 読んだアカウントと、画面に送る形にしたもの | この手順で作る |
| 値 | `id`・`email`・`isCurrent` | 画面に送るため | 番号・メールアドレス・使用中なら `true` | この手順で作る |
| 値 | `accountUuid` | アカウントを見分けるため | アカウントごとに決まった、変わらない番号 | Claude Code が書く（`oauthAccount` の中） |
| 値 | `emailAddress` | どのアカウントかを人に見せるため | アカウントのメールアドレス | Claude Code が書く（`oauthAccount` の中） |
| C++ の書き方 | `!!x` | `true`・`false` にそろえるため | `x` が `null` なども `false` にする | JavaScript の書き方 |
| C++ の書き方 | `===` | 同じかを比べるため | 型も中身も同じなら `true`（C++ の `==` に当たる） | JavaScript の書き方 |
| C++ の書き方 | `(p) => ({ ... })` | オブジェクトを返す小さな関数のため | `{ }` を `( )` で囲むと、関数の中身でなくオブジェクトを返す | JavaScript の書き方 |
| 変数 | `data` | 画面に送るものをまとめるため | `{ accounts }`（`accounts: accounts` の略） | この手順で作る |

### 3-6. 画面を受け持つクラスを作る

アクティビティバーの欄に出す画面は、Webview（VSCode の中で動く小さな Web ページ）で作ります。まず、そのクラスの入れ物を作ります。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **48行目** `}` の下に、8行足します（49〜56行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 49 -->
<table class="code-lines"><tr><td>

```js
49
50
51
52
53
54
55
56
```

</td><td>

```js

// アクティビティバーに出す画面（Webview）。描くのは media/view.js で、ここは中身を渡すだけ。
class AccountsView {
  constructor(extensionUri) {
    this.extensionUri = extensionUri;
    this.view = null;
  }
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 51 | 画面を受け持つクラス |
| 52〜55 | 拡張機能のフォルダの場所を覚え、画面はまだないので `null` にしておく |

- **なぜ:** `view.js`・`view.css` を画面に読ませるには、拡張機能のフォルダの場所が要る。画面は、欄を初めて開いたときに VSCode が作って渡してくる（3-7）ので、それまでは `null`。
- **効果:** 3-7 で、VSCode から画面を受け取れる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `AccountsView` | 画面を受け持つため | 画面を作り、中身を送り、画面からの知らせを受け取る | この手順で作る |
| 関数 | `constructor` | 作ったときに用意するため | クラスのコンストラクタ。`new` で作ったときに呼ばれる | JavaScript の書き方 |
| 引数・メンバー変数 | `extensionUri` | 拡張機能のフォルダを指すため | 拡張機能のフォルダの場所（`vscode.Uri`） | VSCode の拡張機能の API（`ExtensionContext`）。3-12 で渡す |
| C++ の書き方 | `this.名前 = ...` | メンバー変数を作るため | JavaScript のクラスでは、宣言せずに `this.名前` に入れるとメンバー変数ができる | JavaScript の書き方 |
| メンバー変数 | `view` | 受け取った画面を持っておくため | VSCode から渡された欄（`WebviewView`）。まだなければ `null` | この手順で作る |

### 3-7. 画面を受け取り、ファイルを読ませる設定をする

欄を開くと、VSCode が `resolveWebviewView` を呼んで画面を渡してきます。画面で JavaScript を動かすことと、`media` フォルダのファイルを読ませることを決めます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **55行目** `}` の下、`}` の上に、7行足します（56〜62行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 56 -->
<table class="code-lines"><tr><td>

```js
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

  resolveWebviewView(webviewView) {
    this.view = webviewView;
    const webview = webviewView.webview;
    const media = vscode.Uri.joinPath(this.extensionUri, 'media');
    webview.options = { enableScripts: true, localResourceRoots: [media] };
  }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 57 | 欄を初めて開いたときに、VSCode が呼ぶ |
| 58 | 受け取った欄を覚えておく（3-9 で中身を送るため） |
| 59 | 欄の中の Web ページ |
| 60 | 拡張機能の `media` フォルダの場所 |
| 61 | ページで JavaScript を動かせるようにし、読めるファイルを `media` の中だけにする |

- **なぜ:** Webview は、何も決めないと JavaScript が動かず、PC のファイルも読めない。読める場所を `media` だけにしておけば、ページから鍵のファイルなどは読めない。
- **効果:** 3-8 で、`media` の `view.js`・`view.css` を読ませられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `resolveWebviewView` | 画面を受け取るため | 欄を開いたときに VSCode が呼び、欄（`WebviewView`）を渡す | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#WebviewViewProvider)（`WebviewViewProvider` の決まり） |
| 引数 | `webviewView` | 欄を受け取るため | 欄。中の Web ページ（`webview`）と、見えているか（`visible`）などを持つ | VSCode の拡張機能の API（`WebviewView`） |
| 変数 | `webview` | ページを動かすため | 欄の中の Web ページ。中身（`html`）を入れ、知らせを送り合う | VSCode の拡張機能の API（`Webview`） |
| 関数 | `vscode.Uri.joinPath` | フォルダの中の場所を作るため | 場所（`Uri`）に、名前をつないだ場所を返す | VSCode の拡張機能の API（`Uri`） |
| 変数 | `media` | `media` フォルダを指すため | 拡張機能の `media` フォルダの場所 | この手順で作る |
| メンバー変数 | `options`・`enableScripts`・`localResourceRoots` | ページでできることを決めるため | JavaScript を動かすか、読めるフォルダはどこか | VSCode の拡張機能の API（`WebviewOptions`） |

### 3-8. ページの中身（HTML）を作る

ページには、枠を描く場所（`root`）を1つ置き、`view.css`（見た目）と `view.js`（描く動き）を読ませます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **61行目** `webview.options = { enableScripts: true, localResourceRoots: [media] };` の下、`}` の上に、7行足します（62〜68行目になります）。

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
```

</td><td>

```js
    const nonce = String(Math.random()).slice(2);
    const script = webview.asWebviewUri(vscode.Uri.joinPath(media, 'view.js'));
    const style = webview.asWebviewUri(vscode.Uri.joinPath(media, 'view.css'));
    webview.html = `<!DOCTYPE html><html lang="ja"><head><meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src ${webview.cspSource}; script-src 'nonce-${nonce}';">
<link rel="stylesheet" href="${style}"></head>
<body><div id="root"></div><script nonce="${nonce}" src="${script}"></script></body></html>`;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 62 | 読ませてよい JavaScript の印にする、毎回違う数字の文字 |
| 63〜64 | `view.js`・`view.css` を、ページから読める形の場所にする |
| 65〜68 | ページの中身。読んでよいもの（CSS は拡張機能のもの、JavaScript は印の付いたものだけ）を決め、CSS を読み、`root` を置き、`view.js` を読む |

- **なぜ:** ページで動かしてよい JavaScript を、印（`nonce`）の付いた `view.js` だけにする（VSCode の決まりで勧められている形）。描く動きを別のファイルにすると、`extension.js` の中に長い HTML を書かなくて済む。
- **効果:** 欄を開くと、`view.js` が動き出す（3-13 から書く）。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `nonce` | 読ませてよい JavaScript に印を付けるため | 毎回違う数字の文字 | この手順で作る |
| 関数 | `Math.random`・`String`・`slice` | 毎回違う文字を作るため | 0〜1 の数を作る・文字にする・`0.` を取り除く | JavaScript の標準 |
| 関数 | `asWebviewUri` | ページから読める場所にするため | PC の中のファイルの場所を、ページが読める URL にする | VSCode の拡張機能の API（`Webview`） |
| 変数 | `script`・`style` | 読ませるファイルを指すため | `view.js`・`view.css` の URL | この手順で作る |
| メンバー変数 | `html` | ページの中身を入れるため | 入れた HTML がページに出る | VSCode の拡張機能の API（`Webview`） |
| C++ の書き方 | `` `…${x}…` `` | 文字に値を埋め込むため | `` ` `` で囲んだ文字の中の `${ }` に、値が入る。改行もそのまま入る | JavaScript の書き方 |
| 値 | `Content-Security-Policy` | ページで読んでよいものを決めるため | `default-src 'none'` で全部止め、CSS と印の付いた JavaScript だけを許す | ブラウザ（HTML） |
| メンバー変数 | `cspSource` | 拡張機能のファイルを許すため | 拡張機能のファイルの場所を表す文字 | VSCode の拡張機能の API（`Webview`） |
| 値 | `root` | 枠を描く場所 | `<div id="root">`。`view.js` がこの中に枠を入れる | この手順で作る |

### 3-9. ページに中身を送る関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **69行目** `}` の下、`}` の上に、4行足します（70〜73行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 70 -->
<table class="code-lines"><tr><td>

```js
70
71
72
73
```

</td><td>

```js

  render() {
    if (this.view) this.view.webview.postMessage(viewData());
  }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 71〜73 | 欄があれば、3-5 の中身をページに送る |

- **なぜ:** ページ（`view.js`）と `extension.js` は別々に動いていて、関数を直接は呼べない。知らせ（メッセージ）を送り合う。
- **効果:** 保存や切り替えのあとに `render()` を呼べば、ページが描き直される（ページ側は 3-16）。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `render` | ページを描き直させるため | `viewData()` をページに送る | この手順で作る |
| 関数 | `postMessage` | ページに知らせを送るため | 渡したもの（JSON にできるもの）をページに届ける | VSCode の拡張機能の API（`Webview`） |

### 3-10. ページから届いた知らせを受け取る関数を作る

ページは、動き出すと「準備できた（`ready`）」と知らせてきます（3-16）。そのときに中身を送ります。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **74行目** `}` の下に、8行足します（75〜82行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 75 -->
<table class="code-lines"><tr><td>

```js
75
76
77
78
79
80
81
82
```

</td><td>

```js

// 画面から届いた知らせ。{ type: 'ready' | 'save' | 'switch' | 'up' | 'down' | 'remove', id }
function onMessage(view, message) {
  if (message.type === 'ready') {
    view.render();
    return;
  }
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 77 | 画面のクラスと、届いた知らせを受け取る |
| 78〜80 | `ready` なら、中身を送って終わる |

- **なぜ:** ページが動き出す前に送った知らせは、受け取る相手がいなくて消える。ページから「準備できた」と言ってもらってから送れば、取りこぼさない。
- **効果:** 欄を開くと、すぐ枠が描かれる。手順4〜7 で、ボタンの知らせもここに足す。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `onMessage` | ページからの知らせに応えるため | 知らせの `type` ごとに動きを分ける | この手順で作る |
| 引数 | `message` | 知らせを受け取るため | `{ type, id }`。`type` は何をしてほしいか、`id` はどのアカウントか | この手順で作る（送るのは `view.js`） |

### 3-11. ページからの知らせを、3-10 の関数につなぐ

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **68行目** `` <body><div id="root"></div><script nonce="${nonce}" src="${script}"></script></body></html>`; `` の下、`}` の上に、1行足します（69行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 69 -->
<table class="code-lines"><tr><td>

```js
69
```

</td><td>

```js
    webview.onDidReceiveMessage((message) => onMessage(this, message));
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 69 | ページから知らせが届いたら、`onMessage` に渡す |

- **なぜ:** 受け取る口（`onDidReceiveMessage`）に関数を登録しないと、知らせは届かない。
- **効果:** ページの `ready` で、中身が送られる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `onDidReceiveMessage` | ページからの知らせを受け取るため | 知らせが届いたときに呼ぶ関数を登録する | VSCode の拡張機能の API（`Webview`） |

### 3-12. 拡張機能が動き出したときに、画面のクラスを登録する

VSCode は、欄を開いたときに `extension.js` の `activate` を呼びます。そこで、画面のクラスを欄 `claudeAccounts.list` に登録します。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **83行目** `}` の下に、11行足します（84〜94行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 84 -->
<table class="code-lines"><tr><td>

```js
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
```

</td><td>

```js

function activate(context) {
  const view = new AccountsView(context.extensionUri);
  context.subscriptions.push(
    vscode.window.registerWebviewViewProvider('claudeAccounts.list', view, { webviewOptions: { retainContextWhenHidden: true } }),
  );
}

function deactivate() {}

module.exports = { activate, deactivate };
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 85 | 拡張機能が動き出したときに VSCode が呼ぶ |
| 86 | 画面のクラスを1つ作る |
| 87〜89 | 欄 `claudeAccounts.list` の画面を、このクラスが受け持つように登録する。見えなくなっても、ページを残しておく |
| 92 | 拡張機能が止まるときに VSCode が呼ぶ。片付けは `subscriptions` に任せるので、中身はない |
| 94 | `activate` と `deactivate` を、VSCode から呼べるようにする |

- **なぜ:** 拡張機能の入り口は `activate` と決まっている。登録を `subscriptions` に入れておけば、止めるときの片付けを自分で書かなくてよい。`retainContextWhenHidden` を付けないと、ほかの欄に切り替えるたびにページが作り直され、`view.js` が初めから動き直す。
- **効果:** 欄を開くと、`view.js` が動き出す。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `activate` | 拡張機能を動き出させるため | 拡張機能が動き出したときに、VSCode が1回呼ぶ | VSCode の拡張機能の決まり |
| 引数 | `context` | 拡張機能の情報を受け取るため | 片付けを任せる `subscriptions`、フォルダの場所 `extensionUri` などを持つ | VSCode の拡張機能の API（`ExtensionContext`） |
| C++ の書き方 | `new` | クラスを作るため | C++ と同じくクラスを作る。消すのは JavaScript が自動でする | JavaScript の書き方 |
| メンバー変数 | `subscriptions` | 片付けを任せるため | ここに入れたものを、拡張機能が止まるときに VSCode が片付ける | VSCode の拡張機能の API（`ExtensionContext`） |
| 関数 | `push` | 配列に足すため | 配列の最後に足す（`std::vector` の `push_back` に当たる） | JavaScript の標準（配列） |
| 関数 | `vscode.window.registerWebviewViewProvider` | 画面のクラスを登録するため | 欄の名前と、`resolveWebviewView` を持つクラスを結びつける | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#window.registerWebviewViewProvider) |
| 値 | `retainContextWhenHidden` | ページを残しておくため | `true` なら、欄が見えなくなってもページを消さない | VSCode の拡張機能の API（`WebviewPanelOptions`） |
| 関数 | `deactivate` | 拡張機能を止めるため | 拡張機能が止まるときに、VSCode が呼ぶ | VSCode の拡張機能の決まり |
| 変数 | `module.exports` | 関数をファイルの外から呼べるようにするため | ここに入れたものを、`require` した側（VSCode）が使える | Node.js |

### 3-13. ページの JavaScript を作り、要素を作る関数を足す

ここからは `media\view.js`（ページの中で動く JavaScript）です。`extension.js` と違い、ファイルは読めず、HTML の要素を作って並べることだけをします。

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) に、次の12行を書きます（1〜12行目）。

<!-- new: .claude/extensions/claude-accounts/media/view.js -->
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
// アカウントの一覧を描く（Webview の中で動く）。中身は extension.js から postMessage で届く。
const vscode = acquireVsCodeApi();
const root = document.getElementById('root');
let data = { accounts: [] };

// 要素を作る。children は文字か要素の配列（文字は textContent なので、名前に < などがあっても崩れない）。
function el(tag, className, children = []) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  for (const child of children) node.append(child);
  return node;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 2 | `extension.js` と知らせを送り合う道具を受け取る |
| 3 | 3-8 で置いた `root` |
| 4 | `extension.js` から届いた中身。届くまでは、アカウントなし |
| 7〜12 | 要素を作り、見た目の名前（`className`）を付け、中に文字や要素を入れて返す |

- **なぜ:** 文字は `append` で入れると、そのまま文字として出る。HTML の文字をつないで入れる形にすると、アカウントの名前に `<` などがあったときに崩れる。
- **効果:** `el('div', 'name', [名前])` のように、短く要素を作れる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `acquireVsCodeApi` | `extension.js` と知らせを送り合うため | `postMessage` を持つものを返す。ページで1回だけ呼べる | VSCode の Webview（ページの中だけにある） |
| 変数 | `vscode`（`view.js`） | `extension.js` に知らせを送るため | `acquireVsCodeApi()` の結果。`extension.js` の `vscode` とは別もの | この手順で作る |
| 関数 | `document.getElementById` | `root` を取るため | `id` が同じ要素を返す | ブラウザ（DOM） |
| C++ の書き方 | `let` | あとで値を入れる名前を作るため | 名前を作る。`const` と違い、あとで別の値を入れられる | JavaScript の書き方 |
| 変数 | `data`（`view.js`） | 届いた中身を覚えておくため | 3-5 の `viewData()` の結果 | この手順で作る |
| 関数 | `el` | 要素を作るため | 要素の種類・見た目の名前・中身を受け取り、要素を返す | この手順で作る |
| C++ の書き方 | `children = []` | 引数を省けるようにするため | 渡されなければ空の配列にする（C++ の引数の初期値と同じ） | JavaScript の書き方 |
| 関数 | `document.createElement` | 要素を作るため | `div`・`span`・`button` などの要素を作る | ブラウザ（DOM） |
| メンバー変数 | `className` | 見た目を決めるため | 要素の `class`。`view.css` の `.名前` と同じなら、その見た目になる | ブラウザ（DOM） |
| C++ の書き方 | `for (const x of 配列)` | 1つずつ取り出すため | 配列の中身を1つずつ `x` に入れて回す（C++ の範囲 for と同じ） | JavaScript の書き方 |
| 関数 | `append` | 中身を入れるため | 要素の最後に、文字か要素を足す。いくつでも渡せる | ブラウザ（DOM） |

### 3-14. アカウント1つ分の枠を作る関数を足す

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **12行目** `}` の下に、10行足します（13〜22行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 13 -->
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
```

</td><td>

```js

function card(account) {
  const head = el('div', 'card-head', [
    el('span', `dot ${account.isCurrent ? 'on' : ''}`, [account.isCurrent ? '●' : '○']),
    el('div', 'who', [el('div', 'name', [account.name]), el('div', 'dim', [account.email])]),
  ]);
  const node = el('div', `card ${account.isCurrent ? 'is-current' : ''}`, [head]);
  if (account.isCurrent) node.append(el('div', 'current', ['使用中']));
  return node;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 15〜18 | 枠の頭の段。使用中なら `●`、それ以外は `○`。その右に、名前と、薄い字のメールアドレス |
| 19 | 枠を作り、頭の段を入れる。使用中なら枠の線の色を変える（3-18） |
| 20 | 使用中なら、下に「使用中」と出す |

- **なぜ:** 頭の段を配列で書いておくと、手順6・7 で↑↓✕のボタンを、行を足すだけで並べられる。枠に入れるものを `append` で足していく形にすると、手順5・8 で切り替えのボタンや使用量を、行を足すだけで入れられる。
- **効果:** アカウント1つ分の枠ができる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `card` | アカウント1つ分の枠を作るため | アカウントを受け取り、枠の要素を返す | この手順で作る |
| 引数 | `account` | 枠に出すアカウントを受け取るため | 3-5 で作った `{ id, name, email, isCurrent }` | 3-5 で作る |
| 変数 | `head`・`node` | 枠を組み立てるため | 枠の頭の段と、枠 | この手順で作る |

### 3-15. 全部の枠を描く関数を足す

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **22行目** `}` の下に、5行足します（23〜27行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 23 -->
<table class="code-lines"><tr><td>

```js
23
24
25
26
27
```

</td><td>

```js

function render() {
  const children = data.accounts.map(card);
  root.replaceChildren(...children);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 25 | アカウントを1つずつ枠にする |
| 26 | `root` の中身を、作った枠に入れ替える |

- **なぜ:** 毎回全部作り直す。アカウントは数個なので、差分を探すより簡単で速さも変わらない。
- **効果:** `render()` を呼ぶと、今の `data` で描き直される。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `render`（`view.js`） | 枠を描き直すため | `data` から全部の枠を作り、`root` に入れる | この手順で作る |
| 変数 | `children` | 描く枠を持っておくため | 枠の配列 | この手順で作る |
| 関数 | `replaceChildren` | 中身を入れ替えるため | 要素の中身を全部消し、渡したものを入れる | ブラウザ（DOM） |
| C++ の書き方 | `f(...配列)` | 配列を引数に並べるため | 配列の中身を、1つずつの引数にして渡す | JavaScript の書き方 |

### 3-16. 中身が届いたら描き、準備できたと知らせる

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **27行目** `}` の下に、6行足します（28〜33行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 28 -->
<table class="code-lines"><tr><td>

```js
28
29
30
31
32
33
```

</td><td>

```js

window.addEventListener('message', (event) => {
  data = event.data;
  render();
});
vscode.postMessage({ type: 'ready' });
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 29〜32 | `extension.js` から中身が届いたら、覚えて描き直す |
| 33 | `extension.js` に「準備できた」と知らせる（3-10） |

- **なぜ:** 受け取る関数を登録してから `ready` を送る。逆にすると、`ready` への返事が、登録の前に届いて消えることがある。
- **効果:** 欄を開くと、保存したアカウントの枠が並ぶ。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `window.addEventListener` | 知らせを受け取るため | `'message'` を渡すと、`extension.js` の `postMessage` が届いたときに関数を呼ぶ | ブラウザ（DOM） |
| 値 | `event.data` | 届いた中身を取るため | `postMessage` で送られたもの | ブラウザ（DOM） |
| 関数 | `vscode.postMessage`（`view.js`） | `extension.js` に知らせるため | 渡したものを `extension.js` の `onDidReceiveMessage` に届ける | VSCode の Webview |

### 3-17. 見た目のファイルを作り、文字の色と大きさを VSCode に合わせる

ここからは `media\view.css`（ページの見た目）です。

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) に、次の7行を書きます（1〜7行目）。

<!-- new: .claude/extensions/claude-accounts/media/view.css -->
<table class="code-lines"><tr><td>

```css
1
2
3
4
5
6
7
```

</td><td>

```css
/* 色は VSCode のテーマの色（--vscode-〇〇）を使い、明るいテーマでも暗いテーマでも読めるようにする。 */
body {
  padding: 8px 12px;
  color: var(--vscode-foreground);
  font-family: var(--vscode-font-family);
  font-size: var(--vscode-font-size);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 2〜7 | ページ全体の余白と、文字の色・書体・大きさを、VSCode と同じにする |

- **なぜ:** 色を決め打ちにすると、明るいテーマ・暗いテーマのどちらかで読めなくなる。VSCode は、テーマの色を `--vscode-〇〇` の名前でページに渡してくれる。
- **効果:** ほかの欄と同じ文字で出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `body { … }` | 見た目を決めるため | `選ぶもの { 名前: 値; }` の形で、選んだ要素の見た目を決める。`body` はページ全体 | ブラウザ（CSS） |
| 値 | `padding`・`color`・`font-family`・`font-size` | 見た目を決めるため | 内側の余白・文字の色・書体・文字の大きさ | ブラウザ（CSS） |
| 値 | `var(--vscode-〇〇)` | テーマの色を使うため | VSCode が渡す、今のテーマの色や書体。`var(名前, 代わり)` で、ないときの値も書ける | VSCode の Webview（[テーマの色の名前](https://code.visualstudio.com/api/references/theme-color)） |

### 3-18. 枠の見た目を足す

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **7行目** `}` の下に、11行足します（8〜18行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 8 -->
<table class="code-lines"><tr><td>

```css
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
```

</td><td>

```css

.card {
  border: 1px solid var(--vscode-widget-border, var(--vscode-panel-border));
  border-radius: 6px;
  padding: 10px;
  margin-bottom: 10px;
}

.card.is-current {
  border-color: var(--vscode-focusBorder);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 9〜14 | 枠（`.card`）に、細い線・角の丸み・内側の余白・下のすき間を付ける |
| 16〜18 | 使用中の枠は、線を目立つ色にする |

- **なぜ:** 1つずつ線で囲むと、アカウントの区切りがわかりやすい。
- **効果:** アカウントが枠で並び、使用中がひと目でわかる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `.card`・`.card.is-current` | 見た目を当てる要素を選ぶため | `.名前` は `className` にその名前を持つ要素。続けて書くと、両方を持つ要素 | ブラウザ（CSS） |
| 値 | `border`・`border-radius`・`margin-bottom`・`border-color` | 枠の線を引くため | 線・角の丸み・下のすき間・線の色 | ブラウザ（CSS） |

### 3-19. 枠の頭の段を横に並べる

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **18行目** `}` の下に、7行足します（19〜25行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 19 -->
<table class="code-lines"><tr><td>

```css
19
20
21
22
23
24
25
```

</td><td>

```css

.card-head {
  display: flex;
  align-items: center;
  gap: 4px;
  margin-bottom: 8px;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 20〜25 | 頭の段の中身（印・名前・ボタン）を、横1列に、縦の真ん中でそろえて並べる |

- **なぜ:** 要素は何もしないと縦に積まれる。
- **効果:** 印・名前が横に並ぶ（手順6・7 でボタンも右に並ぶ）。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `display: flex`・`align-items`・`gap` | 横に並べるため | 中身を横1列に並べる・縦のそろえ方・すき間 | ブラウザ（CSS） |

### 3-20. 名前とメールアドレスを、残りの幅いっぱいに置く

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **25行目** `}` の下に、12行足します（26〜37行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 26 -->
<table class="code-lines"><tr><td>

```css
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
```

</td><td>

```css

.who {
  flex: 1;
  min-width: 0;
}

.name {
  font-weight: 600;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 27〜30 | 名前の欄を、残りの幅いっぱいに広げる（ボタンを右端に寄せる） |
| 32〜37 | 名前を太字にし、入りきらないときは `…` で切る |

- **なぜ:** 欄の幅は人が変えられるので、狭いときに名前が折り返して崩れないようにする。
- **効果:** 欄を狭くしても、1行に収まる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `flex: 1`・`min-width: 0` | 残りの幅に広げるため | 横に並んだ中で、余った幅を全部取る。`min-width: 0` で、中身が長くても縮められる | ブラウザ（CSS） |
| 値 | `font-weight`・`overflow`・`text-overflow`・`white-space` | 名前を1行に収めるため | 太さ・はみ出した分を隠す・隠した所に `…`・折り返さない | ブラウザ（CSS） |

### 3-21. 使用中の印の色を付ける

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **37行目** `}` の下に、9行足します（38〜46行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 38 -->
<table class="code-lines"><tr><td>

```css
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

```css

.dot {
  color: var(--vscode-descriptionForeground);
  margin-right: 4px;
}

.dot.on {
  color: var(--vscode-testing-iconPassed, var(--vscode-charts-green));
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 39〜42 | 印（`○`）は薄い色にする |
| 44〜46 | 使用中の印（`●`）は緑にする |

- **なぜ:** 使用中かどうかを、枠の線（3-18）と印の色の両方で見せる。
- **効果:** 使用中のアカウントに緑の `●` が付く。

### 3-22. 薄い字と「使用中」の見た目を足す

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **46行目** `}` の下に、11行足します（47〜57行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 47 -->
<table class="code-lines"><tr><td>

```css
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
```

</td><td>

```css

.dim {
  color: var(--vscode-descriptionForeground);
  font-size: 0.92em;
}

.current {
  margin-top: 6px;
  text-align: center;
  color: var(--vscode-descriptionForeground);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 48〜51 | メールアドレスなどの説明の字を、薄く少し小さくする |
| 53〜57 | 「使用中」を、枠の下に真ん中寄せで薄く出す |

- **なぜ:** 名前より目立たせない情報は、VSCode のほかの欄と同じ「説明の色」にする。
- **効果:** 名前が目立ち、メールアドレスは控えめに出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `text-align`・`margin-top` | 「使用中」の置き方を決めるため | 文字の寄せ方・上のすき間 | ブラウザ（CSS） |

**確かめ方:** VSCode のウィンドウを再読み込みし（2-6）、アイコンを押すと、欄が開いて何も出ない（まだ保存していないので）。`Ctrl+Shift+U`（出力）で「拡張機能ホスト」を選び、`claude-accounts` のエラーが出ていない。

---

## 手順4: 今のアカウントを保存する

欄の上に「+」のボタンを出し、押すと今ログインしているアカウントのログイン情報を、名前を付けて `account-profiles` に保存します。今のアカウントが保存されていないときは、欄の一番上にも保存のボタンを出します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 追加（4-1 でコマンド、4-10 でボタン） |
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 追加（4-2〜4-9、4-11） |
| [view.js](../../../../.claude/extensions/claude-accounts/media/view.js) | 追加（4-12〜4-13） |
| [view.css](../../../../.claude/extensions/claude-accounts/media/view.css) | 追加（4-14〜4-15） |

### 4-1. 保存のコマンドを決める

ボタンから呼ぶものは、先に「コマンド」として名前と絵を決めます。

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **9行目** `"contributes": {` の下、`"viewsContainers": {` の上に、3行足します（10〜12行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 10 -->
<table class="code-lines"><tr><td>

```json
10
11
12
```

</td><td>

```json
    "commands": [
      { "command": "claudeAccounts.save", "title": "今のアカウントを保存", "icon": "$(add)" }
    ],
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 10 | コマンドの一覧 |
| 11 | 保存のコマンド。ボタンの絵は `+` |
| 12 | 一覧の終わり。次の `"viewsContainers"` があるので `,` を付ける |

- **なぜ:** VSCode の見出しのボタンは、コマンドの名前を指して作る。コマンドを先に決めておくと、ボタン（4-10）から同じ名前で呼べる。
- **効果:** `claudeAccounts.save` の名前で、4-9 で動きを結びつけられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `commands` | コマンドを決めるため | コマンドの名前・見出し・絵を並べる | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.commands) |
| 値 | `command`・`title`・`icon` | コマンドを決めるため | コマンドの名前・マウスを乗せたときの見出し・ボタンの絵 | 同上 |
| 値 | `$(add)` | ボタンの絵にするため | `$(名前)` で、VSCode に入っている絵（Codicon）を使う。`add` は `+` | 同上 |
| 値 | `claudeAccounts.save` | コマンドを見分けるため | 保存のコマンドの名前 | この手順で作る |

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

### 4-5. 保存などのあとに描き直す関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **94行目** `}` の下、`}` の上に、5行足します（95〜99行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 95 -->
<table class="code-lines"><tr><td>

```js
95
96
97
98
99
```

</td><td>

```js

  // 保存・切り替え・消したあと。すぐ描き直し、使用量もすぐ聞き直す。
  refresh() {
    this.render();
  }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 97〜99 | ページを描き直す（手順8で、使用量もすぐ聞き直すようにする） |

- **なぜ:** 保存・切り替え・消したあとに呼ぶものを1つにしておけば、手順8で「そのあと使用量も聞き直す」を1行足すだけで済む。
- **効果:** 4-7 などで `view.refresh()` を呼べば、枠が新しくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `refresh` | 保存などのあとに描き直すため | `render()` を呼ぶ（手順8で使用量の聞き直しを足す） | この手順で作る |

### 4-6. 保存のボタンを押したときの関数を作る

まず、ログインしていないときは知らせて終わるところまでを書きます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **100行目** `}` の下、（空の行） の上に、8行足します（101〜108行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 101 -->
<table class="code-lines"><tr><td>

```js
101
102
103
104
105
106
107
108
```

</td><td>

```js

async function onSave(view) {
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
| 102 | 保存のボタンを押したときに呼ぶ。描き直しのために画面のクラスを受け取る |
| 104〜107 | ログインしていない（`oauthAccount` かログイン情報のファイルがない）ときは、黄色の知らせを出して終わる |

- **なぜ:** ログインしていないのに保存すると、中身のないアカウントが一覧に出てしまう。
- **効果:** ログインしていないときは「/login してから」と知らせる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `async` | 中で `await`（4-7）を使うため | 関数を「途中で人の入力を待てる関数」にする | JavaScript の書き方 |
| 関数 | `onSave` | 保存のボタンの動き | 名前を聞いて、今のアカウントを保存する | この手順で作る |
| 関数 | `vscode.window.showWarningMessage` | 注意を知らせるため | 右下に黄色の知らせを出す | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#window.showWarningMessage) |

### 4-7. 名前を聞いて保存する

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **107行目** `}` の下、`}` の上に、8行足します（108〜115行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 108 -->
<table class="code-lines"><tr><td>

```js
108
109
110
111
112
113
114
115
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
  view.refresh();
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 108 | 保存済みかを探す |
| 109〜112 | 画面の上に入力欄を出し、一覧に出す名前を聞く。初めの値は、保存済みならその名前、まだならメールアドレス |
| 113 | `Esc` で閉じた・空にしたときは保存しない |
| 114〜115 | 保存して、描き直す |

- **なぜ:** メールアドレスだけだと、どのアカウントか（仕事用・自分用など）がわかりにくいので、名前を付けられるようにする。保存済みのアカウントでもう一度押せば、名前を付け直せる。
- **効果:** 押すと名前を聞かれ、`account-profiles` にファイルができて枠が出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `name` | 聞いた名前を覚えておくため | 入力欄に打った名前。閉じたら `undefined` | この手順で作る |
| C++ の書き方 | `await` | 人が入力し終わるのを待つため | 右の処理が終わるまで待ち、その結果を受け取る。待つ間も VSCode は止まらない | JavaScript の書き方 |
| 関数 | `vscode.window.showInputBox` | 名前を聞くため | 画面の上に入力欄を出し、打った文字を返す | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#window.showInputBox) |
| 値 | `prompt`・`value` | 入力欄に出すため | 入力欄の下の説明と、初めから入っている文字 | VSCode の拡張機能の API（`InputBoxOptions`） |

### 4-8. ページの「保存」の知らせで保存する

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **123行目** `}` の下、`}` の上に、1行足します（124行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 124 -->
<table class="code-lines"><tr><td>

```js
124
```

</td><td>

```js
  if (message.type === 'save') return onSave(view);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 124 | ページから `save` が届いたら、`onSave` を動かす（ページのボタンは 4-13） |

- **なぜ:** 欄の一番上に出す保存のボタン（4-13）は、ページの中にあるので、知らせで頼んでくる。
- **効果:** ページのボタンからも保存できる。

### 4-9. コマンドに動きを結びつける

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **130行目** `vscode.window.registerWebviewViewProvider('claudeAccounts.list', view, { webviewOptions: { retainContextWhenHidden: true } }),` の下、`);` の上に、1行足します（131行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 131 -->
<table class="code-lines"><tr><td>

```js
131
```

</td><td>

```js
    vscode.commands.registerCommand('claudeAccounts.save', () => onSave(view)),
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 131 | `claudeAccounts.save` が呼ばれたら、`onSave` を動かす |

- **なぜ:** `package.json` のコマンドは名前だけなので、動きは `registerCommand` で結びつける。
- **効果:** コマンドが呼べるようになる（ボタンは 4-10 で出す）。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `vscode.commands.registerCommand` | コマンドに動きを結びつけるため | コマンドの名前と、呼ばれたときに動かす関数を結びつける | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#commands.registerCommand) |
| C++ の書き方 | `() => f()` | その場で小さな関数を作るため | 引数のない関数。呼ばれたら `f()` を動かす | JavaScript の書き方 |

### 4-10. 欄の見出しにボタンを出す

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **12行目** `],` の下、`"viewsContainers": {` の上に、5行足します（13〜17行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 13 -->
<table class="code-lines"><tr><td>

```json
13
14
15
16
17
```

</td><td>

```json
    "menus": {
      "view/title": [
        { "command": "claudeAccounts.save", "when": "view == claudeAccounts.list", "group": "navigation@1" }
      ]
    },
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13 | ボタン（メニュー）の一覧 |
| 14〜16 | 欄 `claudeAccounts.list` の見出しの右に、保存のボタンを出す |
| 17 | 次の `"viewsContainers"` があるので `,` を付ける |

- **なぜ:** `navigation` の組に入れると、`…` のメニューの中ではなく、見出しの右に絵のボタンとして出る。`@1` は組の中での順番で、手順8で足す聞き直しのボタン（`@2`）より左に出すため。
- **効果:** 欄の見出しの右に「+」が出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `menus` | ボタンを出すため | どこに、どのコマンドのボタン・メニューを出すかを書く | [VSCode の拡張機能の決まり（Contribution Points）](https://code.visualstudio.com/api/references/contribution-points#contributes.menus) |
| 値 | `view/title` | 欄の見出しにボタンを出すため | ビューの見出しの所 | 同上 |
| 値 | `when` | どのビューに出すかを決めるため | 条件を書く。`view == claudeAccounts.list` は「この欄のとき」 | 同上 |
| 値 | `group` | ボタンの出し方を決めるため | `navigation` にすると、見出しの右に絵で出る。`@` の後ろの数が小さいほど左 | 同上 |

### 4-11. 今のアカウントが保存されていなければ、ページに知らせる

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **66行目** `const data = { accounts };` の下、`return data;` の上に、2行足します（67〜68行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 67 -->
<table class="code-lines"><tr><td>

```js
67
68
```

</td><td>

```js
  // 今のアカウントが保存されていなければ、保存を勧める
  if (current && !accounts.some((a) => a.isCurrent)) data.unsaved = current.emailAddress;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 68 | ログインしていて、そのアカウントがどの枠でも使用中でない（保存されていない）なら、メールアドレスを `unsaved` に入れて送る |

- **なぜ:** 保存していないアカウントで使っていると、切り替えたあとに戻れない。気づけるように、欄に出す（4-13）。
- **効果:** ページで、保存していないアカウントがわかる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `some` | 1つでも当てはまるかを調べるため | 配列の中に、関数が `true` を返すものが1つでもあれば `true` | JavaScript の標準（配列） |
| 値 | `unsaved` | 保存していないアカウントを知らせるため | 今のアカウントのメールアドレス。保存済みなら入れない | この手順で作る |

### 4-12. 横いっぱいのボタンを作る関数を足す（view.js）

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **12行目** `}` の下、（空の行） の上に、7行足します（13〜19行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 13 -->
<table class="code-lines"><tr><td>

```js
13
14
15
16
17
18
19
```

</td><td>

```js

// 横いっぱいのボタン（切り替える・保存）。押したら extension.js に { type, id } を送る。
function action(text, type, id) {
  const node = el('button', 'action', [text]);
  node.addEventListener('click', () => vscode.postMessage({ type, id }));
  return node;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 15 | ボタンの文字、知らせの種類、どのアカウントかを受け取る |
| 16 | 見た目の名前 `action` のボタンを作る |
| 17 | 押したら `extension.js` に `{ type, id }` を送る |

- **なぜ:** 保存（4-13）と切り替え（手順5）で同じ形のボタンを使うので、関数にまとめる。
- **効果:** `action('文字', '種類', 番号)` で、押すと知らせを送るボタンが作れる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `action` | 横いっぱいのボタンを作るため | 押すと `{ type, id }` を送るボタンを返す | この手順で作る |
| 引数 | `type`・`id` | 何をしてほしいか・どのアカウントかを送るため | `'save'`・`'switch'` などと、アカウントの番号 | 使う側が決める |
| 関数 | `addEventListener` | 押したときの動きを決めるため | `'click'` を渡すと、押されたときに関数を呼ぶ | ブラウザ（DOM） |

### 4-13. 保存していないとき・1つもないときの案内を出す

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **32行目** `const children = data.accounts.map(card);` の下、`root.replaceChildren(...children);` の上に、3行足します（33〜35行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 33 -->
<table class="code-lines"><tr><td>

```js
33
34
35
```

</td><td>

```js
  // 今のアカウントが保存されていなければ、一番上に保存のボタンを出す
  if (data.unsaved) children.unshift(el('div', 'card', [el('div', 'dim', [`${data.unsaved} は保存されていません。`]), action('今のアカウントを保存', 'save')]));
  if (children.length === 0) children.push(el('div', 'dim', ['Claude で /login してから、上の「+」で今のアカウントを保存してください。']));
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 33〜34 | 今のアカウントが保存されていなければ、一番上に「〇〇 は保存されていません。」と保存のボタンの枠を出す |
| 35 | 枠が1つもなければ、保存のしかたを出す |

- **なぜ:** 初めて開いたときに何も出ないと、何をすればよいかがわからない。
- **効果:** 保存していないアカウントで使っていると、すぐ保存できる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `unshift` | 配列の先頭に足すため | 配列の一番前に足す | JavaScript の標準（配列） |
| メンバー変数 | `length` | 枠がいくつあるかを知るため | 配列の長さ | JavaScript の標準（配列） |

### 4-14. 横いっぱいのボタンの見た目を足す

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **57行目** `}` の下に、11行足します（58〜68行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 58 -->
<table class="code-lines"><tr><td>

```css
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
```

</td><td>

```css

button.action {
  width: 100%;
  margin-top: 6px;
  padding: 4px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  color: var(--vscode-button-foreground);
  background: var(--vscode-button-background);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 59〜68 | 枠の幅いっぱいに広げ、VSCode のボタンと同じ色にし、マウスを乗せると指の形にする |

- **なぜ:** VSCode のほかのボタン（拡張機能の「インストール」など）と同じ見た目にすると、押せるものだとすぐわかる。
- **効果:** 保存・切り替えのボタンが、VSCode の青いボタンの形で出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `button.action` | 見た目を当てる要素を選ぶため | `button` の要素のうち、`className` が `action` のもの | ブラウザ（CSS） |
| 値 | `width`・`border: none`・`cursor`・`background` | ボタンの見た目を決めるため | 幅・線なし・マウスの形・塗りの色 | ブラウザ（CSS） |

### 4-15. マウスを乗せたときの色を足す

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **68行目** `}` の下に、4行足します（69〜72行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 69 -->
<table class="code-lines"><tr><td>

```css
69
70
71
72
```

</td><td>

```css

button.action:hover {
  background: var(--vscode-button-hoverBackground);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 70〜72 | マウスを乗せている間だけ、少し違う色にする |

- **なぜ:** 押せる所だとわかるようにする。
- **効果:** マウスを乗せると、ボタンの色が変わる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `:hover` | マウスを乗せている間の見た目のため | 選んだ要素にマウスが乗っている間だけ当たる | ブラウザ（CSS） |

**確かめ方:**

1. ウィンドウを再読み込みし、アイコンを押すと、「〇〇 は保存されていません。」と「今のアカウントを保存」のボタンが出る。
2. ボタン（または見出しの「+」）を押し、名前を入れて `Enter` を押すと、その名前とメールアドレス、緑の `●`、「使用中」の枠が出る。
3. `%USERPROFILE%\.claude\account-profiles\` に、`番号.json` が1つできている。

---

## 手順5: アカウントを切り替える

枠の「このアカウントに切り替える」を押すと、そのアカウントの保存したログイン情報を、Claude Code が読む2つのファイルに書き戻します。書き戻す前に、今のアカウントの最新のログイン情報を保存し直します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 追加（5-1〜5-5） |
| [view.js](../../../../.claude/extensions/claude-accounts/media/view.js) | 追加（5-6） |

### 5-1. 保存したログイン情報を書き戻す関数を足す

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

### 5-2. 切り替えのボタンを押したときの関数を作る

まず、押したのが今のアカウントなら、何もしないところまでを書きます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **126行目** `}` の下、（空の行） の上に、5行足します（127〜131行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 127 -->
<table class="code-lines"><tr><td>

```js
127
128
129
130
131
```

</td><td>

```js

async function onSwitch(view, profile) {
  const current = currentAccount();
  if (current && current.accountUuid === profile.oauthAccount.accountUuid) return;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 128 | 画面のクラスと、切り替える先のアカウントを受け取る |
| 129 | 今のアカウントを取る |
| 130 | 押したのが今のアカウントなら、何もしない |

- **なぜ:** 同じアカウントに切り替えても意味がなく、再読み込みを勧める知らせが出るだけになるため。
- **効果:** 使用中のアカウントを頼まれても何も起きない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `onSwitch` | 切り替えのボタンの動き | 今のアカウントを保存し直してから、押したアカウントに切り替える | この手順で作る |
| 引数 | `profile` | 切り替える先を受け取るため | 保存したアカウント（`loadProfiles` の1つ） | 5-5 で `findProfile` から渡す |

### 5-3. 切り替える前に、今のアカウントを保存し直す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **130行目** `if (current && current.accountUuid === profile.oauthAccount.accountUuid) return;` の下、`}` の上に、10行足します（131〜140行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 131 -->
<table class="code-lines"><tr><td>

```js
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
| 131 | なぜ保存し直すかの説明 |
| 132 | 今のアカウントが保存済みかを探す |
| 133〜134 | 保存済みなら、今のログイン情報で保存し直す（名前はそのまま） |
| 135〜140 | 保存していないアカウントなら、「戻すときにもう一度ログインが要る」と画面の中央で聞き、「切り替える」を押さなければやめる |

- **なぜ:** Claude Code は、使っている間にログイン情報（鍵）を新しいものに取り替え、古い鍵は使えなくなる。保存したときの古い鍵のままにしておくと、戻したときにログインし直しになる。
- **効果:** 何度行き来しても、どのアカウントもログインし直さずに戻れる。保存していないアカウントを、うっかり失わない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `{ modal: true }` | 必ず答えてもらうため | 知らせを右下ではなく画面の中央に出し、答えるまでほかを触れなくする | VSCode の拡張機能の API（`MessageOptions`） |
| 変数 | `answer` | 押したボタンを知るため | 押したボタンの文字（`'切り替える'`）。閉じたら `undefined` | この手順で作る |

### 5-4. 書き戻して、再読み込みを勧める

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **140行目** `}` の下、`}` の上に、6行足します（141〜146行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 141 -->
<table class="code-lines"><tr><td>

```js
141
142
143
144
145
146
```

</td><td>

```js
  applyProfile(profile);
  view.refresh();
  const answer = await vscode.window.showInformationMessage(
    `${profile.name} に切り替えました。開いている Claude に効かせるには、ウィンドウを再読み込みしてください。`,
    '再読み込み');
  if (answer === '再読み込み') vscode.commands.executeCommand('workbench.action.reloadWindow');
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 141 | 押したアカウントのログイン情報を書き戻す |
| 142 | 描き直す（「使用中」が移る） |
| 143〜145 | 切り替えたことと、再読み込みが要ることを、「再読み込み」のボタン付きで知らせる |
| 146 | 「再読み込み」を押したら、VSCode のウィンドウを読み込み直す |

- **なぜ:** 開いている Claude の会話は、動き出したときに読んだログイン情報を持ったままで、ファイルを替えても気づかない。読み込み直すと、新しいファイルを読み直す。
- **効果:** ボタンを1つ押すだけで、開いている Claude も切り替わる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `vscode.window.showInformationMessage` | 終わったことを知らせるため | 右下に青の知らせを出す。ボタンの文字を渡すと、押したボタンの文字を返す | [VSCode の拡張機能の API](https://code.visualstudio.com/api/references/vscode-api#window.showInformationMessage) |
| 関数 | `vscode.commands.executeCommand` | VSCode のコマンドを呼ぶため | 名前でコマンドを呼ぶ | VSCode の拡張機能の API |
| 値 | `'workbench.action.reloadWindow'` | ウィンドウを読み込み直すため | `Ctrl+Shift+P` の「Developer: Reload Window」と同じコマンド | VSCode に入っているコマンド |

### 5-5. ページの「切り替え」の知らせで切り替える

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **155行目** `if (message.type === 'save') return onSave(view);` の下、`}` の上に、3行足します（156〜158行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 156 -->
<table class="code-lines"><tr><td>

```js
156
157
158
```

</td><td>

```js
  const profile = findProfile(message.id);
  if (!profile) return;
  if (message.type === 'switch') return onSwitch(view, profile);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 156 | 知らせの番号（`id`）で、保存したアカウントを探す |
| 157 | 見つからなければ（ほかの窓で消したなど）、何もしない |
| 158 | `switch` なら、そのアカウントに切り替える |

- **なぜ:** ページには番号しか送っていない（3-5。ログイン情報はページに渡さない）ので、ここで保存ファイルを読み直す。
- **効果:** 手順6・7 の知らせも、見つけたアカウントを使える。

### 5-6. 枠に切り替えのボタンを出す（view.js）

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **27行目** `if (account.isCurrent) node.append(el('div', 'current', ['使用中']));` の下、`return node;` の上に、1行足します（28行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 28 -->
<table class="code-lines"><tr><td>

```js
28
```

</td><td>

```js
  if (!account.isCurrent) node.append(action('このアカウントに切り替える', 'switch', account.id));
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 28 | 使用中でない枠の下に、「このアカウントに切り替える」のボタンを出す（4-12 の `action`） |

- **なぜ:** 枠そのものを押して切り替える形にすると、↑↓などを押すつもりで切り替えてしまう。ボタンを分ける。
- **効果:** ボタンを押すと、そのアカウントに切り替わる。

**確かめ方:**

1. 2つ目のアカウントを保存する: Claude の会話で `/login` し、ブラウザで2つ目のアカウントを選ぶ（**`/logout` はしない**。`/logout` すると、1つ目の保存した鍵がサーバーで使えなくなる）。欄の「+」で保存すると、枠が2つ並び、2つ目が「使用中」になる。
2. 1つ目の「このアカウントに切り替える」を押すと「〇〇 に切り替えました。」と出て、「使用中」が1つ目に移る。
3. 「再読み込み」を押し、Claude の会話で `/status` を打つと、1つ目のアカウントのメールアドレスが出る。ブラウザは開かない。

---

## 手順6: 保存を消す

枠の右上の ✕ を押すと、そのアカウントの保存ファイルを消します。Claude からログアウトはしません。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 追加（6-1〜6-2） |
| [view.js](../../../../.claude/extensions/claude-accounts/media/view.js) | 追加（6-3〜6-4） |
| [view.css](../../../../.claude/extensions/claude-accounts/media/view.css) | 追加（6-5） |

### 6-1. 消す関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **147行目** `}` の下、（空の行） の上に、8行足します（148〜155行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 148 -->
<table class="code-lines"><tr><td>

```js
148
149
150
151
152
153
154
155
```

</td><td>

```js

async function onRemove(view, profile) {
  const answer = await vscode.window.showWarningMessage(
    `${profile.name} の保存を消しますか？（ログアウトはしません）`, { modal: true }, '消す');
  if (answer !== '消す') return;
  fs.unlinkSync(profile.file);
  view.refresh();
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 149 | 画面のクラスと、消すアカウントを受け取る |
| 150〜152 | 画面の中央で「消しますか？」と聞き、「消す」を押さなければやめる |
| 153 | そのアカウントの保存ファイルを消す |
| 154 | 描き直す |

- **なぜ:** 消したら戻せない（そのアカウントは、もう一度ログインして保存し直すことになる）ので、必ず聞く。ログアウトはしないので、使用中のアカウントを消しても Claude はそのまま使える。
- **効果:** 要らなくなったアカウントを外せる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `onRemove` | ✕ のボタンの動き | 確かめてから、保存ファイルを消す | この手順で作る |
| 関数 | `fs.unlinkSync` | 保存ファイルを消すため | ファイルを消す | Node.js の標準ライブラリ（fs） |

### 6-2. ページの「消す」の知らせで消す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **166行目** `if (message.type === 'switch') return onSwitch(view, profile);` の下、`}` の上に、1行足します（167行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 167 -->
<table class="code-lines"><tr><td>

```js
167
```

</td><td>

```js
  if (message.type === 'remove') return onRemove(view, profile);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 167 | `remove` なら、そのアカウントの保存を消す |

- **なぜ:** 5-5 で探したアカウントを、そのまま使う。
- **効果:** ✕ の知らせ（6-4）で消せる。

### 6-3. 小さなボタンを作る関数を足す（view.js）

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **19行目** `}` の下、（空の行） の上に、8行足します（20〜27行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 20 -->
<table class="code-lines"><tr><td>

```js
20
21
22
23
24
25
26
27
```

</td><td>

```js

// 小さなボタン（↑・↓・✕）。title はマウスを乗せたときに出る説明。
function button(text, title, type, id) {
  const node = el('button', 'icon', [text]);
  node.title = title;
  node.addEventListener('click', () => vscode.postMessage({ type, id }));
  return node;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 22 | ボタンの文字、マウスを乗せたときの説明、知らせの種類、どのアカウントかを受け取る |
| 23〜24 | 見た目の名前 `icon` のボタンを作り、説明を付ける |
| 25 | 押したら `extension.js` に `{ type, id }` を送る |

- **なぜ:** ✕（手順6）と↑↓（手順7）で同じ形のボタンを使うので、関数にまとめる。文字だけのボタンなので、何をするかを `title` で出す。
- **効果:** `button('✕', '説明', 'remove', 番号)` で、小さなボタンが作れる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `button` | 小さなボタンを作るため | 押すと `{ type, id }` を送る、文字1つのボタンを返す | この手順で作る |
| 引数・メンバー変数 | `title` | 何のボタンかを見せるため | マウスを乗せたときに出る説明 | ブラウザ（DOM） |

### 6-4. 枠の頭の段に ✕ を出す

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **32行目** `el('div', 'who', [el('div', 'name', [account.name]), el('div', 'dim', [account.email])]),` の下、`]);` の上に、1行足します（33行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 33 -->
<table class="code-lines"><tr><td>

```js
33
```

</td><td>

```js
    button('✕', '保存を消す（ログアウトはしない）', 'remove', account.id),
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 33 | 名前の右に ✕ を出す。押すと `remove` を送る |

- **なぜ:** 3-14 で頭の段を配列にしたので、1行足すだけで右に並ぶ。
- **効果:** 枠の右上に ✕ が出る。

### 6-5. 小さなボタンの見た目を足す

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **57行目** `}` の下、（空の行） の上に、13行足します（58〜70行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 58 -->
<table class="code-lines"><tr><td>

```css
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
```

</td><td>

```css

button.icon {
  background: none;
  border: none;
  color: var(--vscode-icon-foreground, var(--vscode-foreground));
  cursor: pointer;
  padding: 2px 5px;
  border-radius: 4px;
}

button.icon:hover {
  background: var(--vscode-toolbar-hoverBackground);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 59〜66 | 塗り・線なしで、絵の色の文字にする |
| 68〜70 | マウスを乗せている間だけ、薄く塗る |

- **なぜ:** VSCode の見出しのボタンと同じ見た目にすると、目立ちすぎない。
- **効果:** ✕ が、VSCode の小さなボタンの形で出る。

**確かめ方:**

1. ウィンドウを再読み込みすると、枠の右上に ✕ が出る。
2. 押して「消す」を押すと、枠が消え、`account-profiles` のファイルも消える。Claude はログインしたまま使える。

---

## 手順7: 並べ替える

枠の右上の↑↓を押すと、1つ上・1つ下の枠と入れ替わります。並び順は、VSCode がこのPCに覚えておく場所（`globalState`）に、アカウントの番号を上から並べた配列で置きます。保存ファイルには書きません。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 追加（7-1〜7-5） |
| [view.js](../../../../.claude/extensions/claude-accounts/media/view.js) | 追加（7-6） |

### 7-1. 覚えておく場所と、並び順のとおりに並べる関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **62行目** `}` の下、（空の行） の上に、13行足します（63〜75行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 63 -->
<table class="code-lines"><tr><td>

```js
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
```

</td><td>

```js

// 並び順と使用量は、VSCode がこのPCに覚えておく（context.globalState）。
let state;

// 並び順（accountUuid を上から並べた配列）のとおりに並べ替える。並び順にないものは後ろ（名前の順のまま）。
function sortByOrder(profiles) {
  const order = state.get('order', []);
  const rank = (p) => {
    const i = order.indexOf(p.oauthAccount.accountUuid);
    return i < 0 ? order.length : i;
  };
  return profiles.sort((a, b) => rank(a) - rank(b));
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 65 | VSCode がこのPCに覚えておく場所。7-2 で入れる（手順8の使用量も、ここに置く） |
| 68 | アカウントの配列を受け取る |
| 69 | 覚えておいた並び順（番号の配列）を取る。まだなければ空の配列 |
| 70〜73 | アカウントが並び順の何番目かを返す。並び順にないもの（新しく保存したもの）は、一番後ろ |
| 74 | 何番目かの小さい順に並べ替える |

- **なぜ:** 並び順にないアカウントを後ろにまとめれば、新しく保存したアカウントは一番下に出る。JavaScript の `sort` は、同じ順位どうしの前後を変えないので、並び順にないものは 3-4 の名前の順のまま並ぶ。
- **効果:** 保存したアカウントを、覚えておいた順に並べられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `state` | 並び順と使用量を覚えておくため | `context.globalState`（7-2） | この手順で作る |
| 関数 | `sortByOrder` | 並び順のとおりに並べるため | 渡した配列を並べ替え、その配列を返す | この手順で作る |
| 関数 | `get` | 覚えておいた値を取るため | 名前で値を取る。なければ2つ目の値を返す | VSCode の拡張機能の API（`Memento`） |
| 関数 | `rank` | 並び順の何番目かを知るため | アカウントを受け取り、並び順の何番目か（なければ一番後ろ）を返す | この手順で作る |
| 関数 | `indexOf` | 番号が並び順の何番目かを探すため | 配列の中で、同じものが何番目にあるかを返す。なければ -1 | JavaScript の標準（配列） |

### 7-2. 拡張機能が動き出したときに、覚えておく場所を受け取る

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **183行目** `function activate(context) {` の下、`const view = new AccountsView(context.extensionUri);` の上に、1行足します（184行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 184 -->
<table class="code-lines"><tr><td>

```js
184
```

</td><td>

```js
  state = context.globalState;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 184 | VSCode がこのPCに覚えておく場所を、`state` に入れる |

- **なぜ:** 描く（`viewData`）前に入れておかないと、`sortByOrder` が `state` を読めない。だから `activate` の一番初めに置く。
- **効果:** 並び順が、VSCode を閉じても残る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `globalState` | このPCに値を覚えておくため | 名前と値の組を覚えておく。VSCode を閉じても残り、どのフォルダを開いても同じ | VSCode の拡張機能の API（`ExtensionContext`） |

### 7-3. 画面に送る前に、並び順のとおりに並べる

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **80行目** `const profiles = loadProfiles();` の下、`const accounts = profiles.map((p) => ({` の上に、1行足します（81行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 81 -->
<table class="code-lines"><tr><td>

```js
81
```

</td><td>

```js
  sortByOrder(profiles);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 81 | 読んだアカウントを、並び順のとおりに並べ替える |

- **なぜ:** 3-5 で読んだ `profiles` を、画面に送る形に作り替える前に並べ替える。
- **効果:** 枠が、覚えておいた順に並ぶ。

### 7-4. 上へ・下へを押したときの関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **169行目** `}` の下、（空の行） の上に、11行足します（170〜180行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 170 -->
<table class="code-lines"><tr><td>

```js
170
171
172
173
174
175
176
177
178
179
180
```

</td><td>

```js

// 押したアカウントを、1つ上（step = -1）か1つ下（step = 1）と入れ替える。
async function onMove(view, profile, step) {
  const ids = sortByOrder(loadProfiles()).map((p) => p.oauthAccount.accountUuid);
  const from = ids.indexOf(profile.oauthAccount.accountUuid);
  const to = from + step;
  if (to < 0 || to >= ids.length) return;
  [ids[from], ids[to]] = [ids[to], ids[from]];
  await state.update('order', ids);
  view.render();
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 172 | 押したアカウントと、動かす向き（上は -1、下は 1）を受け取る |
| 173 | 今の並び（画面に出ている順）の、番号の配列を作る |
| 174〜175 | 押したアカウントが何番目か、どこへ動かすか |
| 176 | 一番上で「上へ」、一番下で「下へ」を押したときは何もしない |
| 177 | 2つの番号を入れ替える |
| 178 | 新しい並び順を覚えておく |
| 179 | 描き直す |

- **なぜ:** 画面に出ている順をそのまま並び順として覚え直すので、まだ並び順にないアカウントも、一度動かせば並び順に入る。描き直しに `refresh()` でなく `render()` を使うのは、手順8で `refresh()` が使用量を聞き直すようになるため（並べ替えでは聞き直さなくてよい）。
- **効果:** ↑↓を押すたびに、枠が1つずつ動く。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `onMove` | ↑↓のボタンの動き | 押したアカウントを、上か下のアカウントと入れ替える | この手順で作る |
| 引数 | `step` | 動かす向きを受け取るため | 上は -1、下は 1 | 使う側が決める（7-5） |
| 変数 | `ids`・`from`・`to` | 入れ替えるため | 番号の配列、押したアカウントの位置、動かす先の位置 | この手順で作る |
| C++ の書き方 | `[a, b] = [b, a]` | 2つを入れ替えるため | 右に並べた値を、左の2つに入れる（`std::swap` と同じことができる） | JavaScript の書き方 |
| 関数 | `update` | 値を覚えておくため | 名前と値を覚えておく。終わるまで `await` で待てる | VSCode の拡張機能の API（`Memento`） |

### 7-5. ページの「上へ」「下へ」の知らせで動かす

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **192行目** `if (message.type === 'remove') return onRemove(view, profile);` の下、`}` の上に、2行足します（193〜194行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 193 -->
<table class="code-lines"><tr><td>

```js
193
194
```

</td><td>

```js
  if (message.type === 'up') return onMove(view, profile, -1);
  if (message.type === 'down') return onMove(view, profile, 1);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 193〜194 | `up` なら1つ上へ、`down` なら1つ下へ動かす |

- **なぜ:** 動かす向きを `step` で渡せば、関数は1つで済む。
- **効果:** ↑↓の知らせ（7-6）で動かせる。

### 7-6. 枠の頭の段に↑↓を出す（view.js）

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **32行目** `el('div', 'who', [el('div', 'name', [account.name]), el('div', 'dim', [account.email])]),` の下、`button('✕', '保存を消す（ログアウトはしない）', 'remove', account.id),` の上に、2行足します（33〜34行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 33 -->
<table class="code-lines"><tr><td>

```js
33
34
```

</td><td>

```js
    button('↑', '上へ', 'up', account.id),
    button('↓', '下へ', 'down', account.id),
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 33〜34 | 名前の右（✕ の左）に↑↓を出す。押すと `up`・`down` を送る |

- **なぜ:** 6-4 と同じく、頭の段の配列に足すだけで並ぶ。
- **効果:** 枠の右上に ↑ ↓ ✕ が並ぶ。

**確かめ方:**

1. ウィンドウを再読み込みすると、枠の右上に ↑ ↓ ✕ が出る。
2. 2つ目の枠の ↑ を押すと、1つ目と入れ替わる。一番上の ↑・一番下の ↓ は何も起きない。
3. VSCode を閉じて開き直しても、並び順が残っている。

---

## 手順8: 使用量のバーと、復活する時刻を出す

各枠に、5時間の枠と週の枠の使用量をバーで出し、「残り何%か」「何時何分に復活するか（あと何分か）」も出します。使用量は、Claude Code の `/usage` と同じ API から、各アカウントの鍵で聞きます。使用中のアカウントは、Claude の会話が進むたびに（30秒に1回まで）、それ以外は5分に1回聞き直します。

| 出るもの（例） | 何のことか |
|---|---|
| `5時間の枠　残り 83%` とバー | バーは使った分（17%）を塗る（Claude Code の `/usage` と同じ向き）。残りが 30% 以下で黄色、10% 以下で赤 |
| `17% 使用・17:10 に復活（あと 41分）` | 17:10 に 0% に戻る |
| `93% 使用・11/20(金) 07:00 に復活（あと 14時間31分）` | 週の枠は日付も出す |
| `16:28:19 に取った値` | いつ聞いた値か |
| `保存した鍵が無効（ログインし直して、保存し直す）` | 401 が返った（鍵がサーバーで無効にされている。`/logout` したときなど） |
| `混んでいて取れなかった（2分後に聞き直す）` | 429 が返った（聞く回数が多すぎる） |
| `鍵の期限切れで聞けない（切り替えると取れる）` | 保存した鍵の期限が切れている |

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [package.json](../../../../.claude/extensions/claude-accounts/package.json) | 追加（8-20 でコマンド、8-21 でボタン） |
| [extension.js](../../../../.claude/extensions/claude-accounts/extension.js) | 追加（8-1〜8-19、8-22） |
| [view.js](../../../../.claude/extensions/claude-accounts/media/view.js) | 追加（8-23〜8-30） |
| [view.css](../../../../.claude/extensions/claude-accounts/media/view.css) | 追加（8-31〜8-35） |

### 8-1. API の場所と、聞き直す間隔を決める

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **75行目** `}` の下、（空の行） の上に、9行足します（76〜84行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 76 -->
<table class="code-lines"><tr><td>

```js
76
77
78
79
80
81
82
83
84
```

</td><td>

```js

const USAGE_URL = 'https://api.anthropic.com/api/oauth/usage';
const CURRENT_GAP_MS = 30 * 1000;      // 使用中のアカウントを、続けて聞かない間（Claude の会話が進むたびに聞く）
const OTHER_GAP_MS = 5 * 60 * 1000;    // ほかのアカウントを、続けて聞かない間
const BUSY_WAIT_MS = 2 * 60 * 1000;    // 429（混んでいる）のあと、聞かずに待つ間
const lastAsked = {};                  // アカウントの番号 → 最後に聞いた時刻
const busyUntil = {};                  // アカウントの番号 → この時刻までは聞かない
let updating = false;                  // 聞いている最中か
let again = null;                      // 聞いている最中に頼まれたら、終わったあとにもう一度聞く（null・false・true は force）
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 77 | 使用量を返す API の URL |
| 78 | 使用中のアカウントは、前に聞いてから30秒たつまで聞かない |
| 79 | ほかのアカウントは、5分たつまで聞かない |
| 80 | 429（混んでいる）が返ったら、2分は聞かない |
| 81〜82 | アカウントごとの、最後に聞いた時刻と、聞かずに待つ時刻 |
| 83〜84 | 聞いている最中か。最中に頼まれたかを覚えておく（8-7・8-8） |

- **なぜ:** 使用中のアカウントは会話のたびに変わるので短く、ほかのアカウントはこのPCでは使っていないので長くする。聞きすぎると 429 で断られるので、そのときは待つ。
- **効果:** 8-4〜8-8 で、アカウントごとに聞くかどうかを決められる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `USAGE_URL` | 使用量を聞くため | `https://api.anthropic.com/api/oauth/usage`。Claude Code が `/usage` で使う API | この手順で作る（URL は Claude のサーバー） |
| 変数 | `CURRENT_GAP_MS`・`OTHER_GAP_MS`・`BUSY_WAIT_MS` | 聞く間隔を決めるため | 30秒・5分・2分をミリ秒にしたもの | この手順で作る |
| 変数 | `lastAsked`・`busyUntil` | アカウントごとの時刻を覚えるため | アカウントの番号から時刻（ミリ秒）を引ける | この手順で作る |
| 変数 | `updating`・`again` | 聞いている最中に重ねて聞かないため | 最中なら `true`。最中に頼まれたら `again` に `force`（8-4）を入れる | この手順で作る |

### 8-2. 1つのアカウントの使用量を聞く関数を作る

鍵（ログイン情報）を受け取り、API に聞きます。まず、聞くところまでを書きます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **84行目** `let again = null;                      // 聞いている最中に頼まれたら、終わったあとにもう一度聞く（null・false・true は force）` の下、（空の行） の上に、9行足します（85〜93行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 85 -->
<table class="code-lines"><tr><td>

```js
85
86
87
88
89
90
91
92
93
```

</td><td>

```js

// 1つのアカウントの使用量を聞く。返すのは { fiveHour, sevenDay } か、取れなかった理由（status）。
async function fetchUsage(credentials) {
  const oauth = credentials.claudeAiOauth;
  if (!oauth || oauth.expiresAt < Date.now()) return { status: 'expired' };
  const response = await fetch(USAGE_URL, {
    headers: { Authorization: `Bearer ${oauth.accessToken}`, 'anthropic-beta': 'oauth-2025-04-20' },
  });
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 87 | ログイン情報（`.credentials.json` の中身の形）を受け取る |
| 88 | その中の鍵の組 |
| 89 | 鍵がない・期限が切れているときは、聞かずに `expired` を返す |
| 90〜92 | 鍵を付けて API に聞き、返事を待つ |

- **なぜ:** 期限の切れた鍵で聞いても断られるだけなので、先に期限を見る。鍵を新しくする（リフレッシュ）のは、使っていない鍵を勝手に取り替えることになるのでしない。
- **効果:** 期限内の鍵なら、そのアカウントの使用量を聞ける。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `fetchUsage` | 1つのアカウントの使用量を聞くため | `{ status, fiveHour, sevenDay, fetchedAt }` を返す。取れなければ `status` だけ | この手順で作る |
| 値 | `claudeAiOauth` | 鍵の組を取り出すため | `.credentials.json` の中の、`accessToken`（鍵）・`expiresAt`（期限。ミリ秒）など | Claude Code が書く |
| 関数 | `Date.now` | 今の時刻を知るため | 今の時刻を、1970年からのミリ秒で返す | JavaScript の標準 |
| 値 | `status` | 取れたか・取れなかった理由を知らせるため | `'ok'`・`'expired'`（期限切れ）・`'invalid'`（401）・`'busy'`（429）・`'error'`（そのほか） | この手順で作る |
| 関数 | `fetch` | API に聞くため | URL に問い合わせ、返事（`Response`）を返す。終わるまで `await` で待つ | JavaScript の標準（Node.js 18 から） |
| 値 | `Authorization`・`anthropic-beta` | 鍵を渡すため | `Bearer 鍵` で鍵を渡す。`anthropic-beta: oauth-2025-04-20` は、ログインの鍵で使う API だと知らせる | Claude のサーバーの決まり |
| 変数 | `response` | 返事を受け取るため | `fetch` の返事 | この手順で作る |

### 8-3. 返事から、5時間の枠と週の枠を取り出す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **92行目** `});` の下、`}` の上に、6行足します（93〜98行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 93 -->
<table class="code-lines"><tr><td>

```js
93
94
95
96
97
98
```

</td><td>

```js
  // 401 は、鍵がサーバーで無効にされている（/logout したときなど）。429 は、聞く回数が多すぎる
  if (response.status === 401) return { status: 'invalid' };
  if (response.status === 429) return { status: 'busy' };
  if (!response.ok) return { status: 'error' };
  const body = await response.json();
  return { status: 'ok', fiveHour: body.five_hour, sevenDay: body.seven_day, fetchedAt: Date.now() };
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 94 | 401（認証できない）なら、鍵が無効になっている |
| 95 | 429（回数が多すぎる）なら、混んでいる |
| 96 | ほかの失敗 |
| 97 | 返事の JSON を読む |
| 98 | 5時間の枠（`five_hour`）・週の枠（`seven_day`）と、聞いた時刻を返す |

- **なぜ:** 取れなかった理由で、することが違う（401 はログインし直す、429 は待つ、期限切れは切り替えたときに取れる）。理由ごとに分けて画面に出す。
- **効果:** 取れれば使用量、取れなければ理由を返せる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `status`（`Response`）・`ok` | 返事が成功かを知るため | `status` は返事の番号（200 は成功、401 は認証できない、429 は回数が多すぎる）。`ok` は成功なら `true` | JavaScript の標準（`Response`） |
| 関数 | `json` | 返事を読むため | 返事の中身を JSON として読み、オブジェクトにする | JavaScript の標準（`Response`） |
| 値 | `five_hour`・`seven_day` | 2つの枠を取り出すため | それぞれ `utilization`（使った割合。0〜100）と `resets_at`（0% に戻る時刻） | Claude のサーバーの返事 |
| 値 | `fiveHour`・`sevenDay`・`fetchedAt` | 覚えておく形にするため | 5時間の枠、週の枠、聞いた時刻（ミリ秒） | この手順で作る |

### 8-4. 間があいたアカウントの使用量を聞き直す関数を作る

まず、聞いた値を覚えて描き直すところまでを書きます。

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **99行目** `}` の下、（空の行） の上に、13行足します（100〜112行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 100 -->
<table class="code-lines"><tr><td>

```js
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
```

</td><td>

```js

// 間があいたアカウントの使用量を聞き直す。force なら間を待たない（429 のあとは待つ）。
async function updateUsage(view, force) {
  updating = true;
  try {
    const current = currentAccount();
    const usage = state.get('usage', {});
    await state.update('usage', usage);
  } finally {
    updating = false;
  }
  view.render();
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 102 | 画面のクラス（描き直すため）と、間を待たずに聞くか（`force`）を受け取る |
| 103 | 聞いている最中にする |
| 105〜106 | 今のアカウントと、前に聞いた使用量（アカウントの番号ごと） |
| 107 | 聞いた使用量を覚えておく |
| 108〜110 | 途中で失敗しても、最中を終わりにする |
| 111 | 描き直す |

- **なぜ:** `finally` で最中を終わりにしないと、どこかで失敗したときに、ずっと「最中」のままになって二度と聞かなくなる。
- **効果:** 8-5 で聞いた値が、VSCode を閉じても残る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `updateUsage` | 使用量を聞き直すため | 間があいたアカウントだけ聞き、覚えて描き直す | この手順で作る |
| 引数 | `force` | 間を待たずに聞くため | `true` なら、30秒・5分を待たない（429 のあとの2分は待つ） | 使う側が決める |
| C++ の書き方 | `try { } finally { }` | 失敗しても必ず後始末するため | `try` の中で何があっても、終わるときに `finally` の中を動かす | JavaScript の書き方 |
| 変数 | `usage` | 使用量を持っておくため | アカウントの番号から、`fetchUsage` の結果を引ける | この手順で作る |

### 8-5. アカウントを1つずつ、聞くかどうか決める

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **106行目** `const usage = state.get('usage', {});` の下、`await state.update('usage', usage);` の上に、8行足します（107〜114行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 107 -->
<table class="code-lines"><tr><td>

```js
107
108
109
110
111
112
113
114
```

</td><td>

```js
    for (const p of loadProfiles()) {
      const id = p.oauthAccount.accountUuid;
      const isCurrent = current && id === current.accountUuid;
      const gap = isCurrent ? CURRENT_GAP_MS : OTHER_GAP_MS;
      if (Date.now() < (busyUntil[id] || 0)) continue;
      if (!force && Date.now() - (lastAsked[id] || 0) < gap) continue;
      lastAsked[id] = Date.now();
    }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 107 | 保存したアカウントを1つずつ |
| 108〜110 | 番号、今のアカウントか、待つ間（今なら30秒、ほかは5分） |
| 111 | 429 のあとの待ち時間なら、聞かない |
| 112 | `force` でなく、前に聞いてから間がたっていなければ、聞かない |
| 113 | 聞く時刻を覚える |

- **なぜ:** 一覧を開くたび・会話が進むたびに全部のアカウントを聞くと、聞く回数が多すぎて 429 で断られる。
- **効果:** 使用中は30秒、ほかは5分より短い間では聞かない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `id`・`isCurrent`・`gap` | 聞くかどうか決めるため | 番号、今のアカウントか、待つ間 | この手順で作る |
| C++ の書き方 | `continue` | このアカウントを飛ばすため | C++ と同じく、次の1つへ進む | JavaScript の書き方 |
| C++ の書き方 | `a || 0` | まだ覚えていないときに 0 にするため | 左がない（`undefined`）なら右を使う | JavaScript の書き方 |

### 8-6. 聞いて、結果を覚える

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **113行目** `lastAsked[id] = Date.now();` の下、`}` の上に、9行足します（114〜122行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 114 -->
<table class="code-lines"><tr><td>

```js
114
115
116
117
118
119
120
121
122
```

</td><td>

```js
      let result;
      try {
        result = await fetchUsage(isCurrent ? readJson(CREDENTIALS_PATH) : p.credentials);
      } catch {
        result = { status: 'error' };   // 通信できない
      }
      if (result.status === 'busy') busyUntil[id] = Date.now() + BUSY_WAIT_MS;
      // 取れなかったときは、前に取れた値を残して理由だけ替える（理由は毎回替わるので、前の理由は残らない）
      usage[id] = result.status === 'ok' ? result : { ...usage[id], status: result.status };
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 116 | 今のアカウントは `.credentials.json` の鍵、ほかは保存した鍵で聞く |
| 117〜119 | 通信できないときは `error` にする |
| 120 | 429 なら、2分は聞かない |
| 122 | 取れたら値を入れ替える。取れなければ、前に取れた値を残して、理由（`status`）だけ入れ替える |

- **なぜ:** 今のアカウントの鍵は、Claude Code が使っている間に新しくしているので、保存した鍵より `.credentials.json` の方が新しい。取れなかったときも、理由は毎回入れ替えるので、前の理由（たとえば「無効」）が残り続けない。
- **効果:** 枠に、最後に取れた値と、今取れない理由が出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `result` | 聞いた結果を受け取るため | `fetchUsage` の結果 | この手順で作る |

### 8-7. 聞いている最中に頼まれたら、覚えておく

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **102行目** `async function updateUsage(view, force) {` の下、`updating = true;` の上に、4行足します（103〜106行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 103 -->
<table class="code-lines"><tr><td>

```js
103
104
105
106
```

</td><td>

```js
  if (updating) {
    again = again || force;
    return;
  }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 103〜106 | 聞いている最中なら、頼まれたことを覚えて終わる（`force` が1回でもあれば `true`） |

- **なぜ:** 聞くのには時間がかかり、その間に会話が進んだり保存したりする。重ねて聞くと、同じアカウントを2回聞き、覚える値も取り合いになる。
- **効果:** 一度に動く聞き直しは1つだけになる。

### 8-8. 終わったあとに、頼まれていた分を聞く

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **132行目** `view.render();` の下、`}` の上に、5行足します（133〜137行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 133 -->
<table class="code-lines"><tr><td>

```js
133
134
135
136
137
```

</td><td>

```js
  if (again !== null) {
    const nextForce = again;
    again = null;
    updateUsage(view, nextForce);
  }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 133〜137 | 最中に頼まれていたら、もう一度聞き直す |

- **なぜ:** 8-7 で捨てた頼みの中に「保存し直したのですぐ聞いて（`force`）」があると、それが消えてしまう。
- **効果:** 保存の直後に、新しい鍵の使用量が出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `nextForce` | もう一度聞くときの `force` | `again` の値 | この手順で作る |

### 8-9. 前に聞いた使用量を消す関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **138行目** `}` の下、（空の行） の上に、9行足します（139〜147行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 139 -->
<table class="code-lines"><tr><td>

```js
139
140
141
142
143
144
145
146
147
```

</td><td>

```js

// 保存し直したアカウントの、前に取った使用量を消す（新しい鍵で取り直すため）。
async function forgetUsage(accountUuid) {
  const usage = state.get('usage', {});
  delete usage[accountUuid];
  await state.update('usage', usage);
  delete lastAsked[accountUuid];
  delete busyUntil[accountUuid];
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 141 | アカウントの番号を受け取る |
| 142〜144 | 覚えておいた使用量から、そのアカウントの分を消す |
| 145〜146 | 聞いた時刻と、待つ時刻も消す（すぐ聞けるように） |

- **なぜ:** 保存し直すと、鍵が新しくなる。前の鍵で聞いた「無効」などが残っていると、新しい鍵で聞くまで古い表示のままになる。
- **効果:** 8-15 で、保存し直したアカウントをすぐ聞き直せる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `forgetUsage` | 保存し直したアカウントの、前の使用量を消すため | 使用量・聞いた時刻・待つ時刻から、そのアカウントの分を消す | この手順で作る |
| JavaScript の書き方 | `delete` | オブジェクトから1つ消すため | `delete usage[番号]` で、その番号の値をなくす（C++ の `delete` とは別もの。メモリは開放しない） | JavaScript の書き方 |

### 8-10. 画面に送る中身に、使用量を足す（1）

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **151行目** `const current = currentAccount();` の下、`const profiles = loadProfiles();` の上に、1行足します（152行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 152 -->
<table class="code-lines"><tr><td>

```js
152
```

</td><td>

```js
  const usage = state.get('usage', {});
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 152 | 覚えておいた使用量を取る |

### 8-11. 画面に送る中身に、使用量を足す（2）

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **159行目** `isCurrent: !!current && p.oauthAccount.accountUuid === current.accountUuid,` の下、`}));` の上に、1行足します（160行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 160 -->
<table class="code-lines"><tr><td>

```js
160
```

</td><td>

```js
    usage: usage[p.oauthAccount.accountUuid] || null,
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 160 | アカウントごとに、使用量（まだなければ `null`）を添える |

- **なぜ:** 聞くのは `extension.js`（8-4）で、描くのはページ（8-23〜）なので、聞いた値を中身に入れて送る。
- **効果:** ページで、アカウントごとの使用量が使える。

### 8-12. 欄が見えるようになったら聞き直す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **187行目** `webview.onDidReceiveMessage((message) => onMessage(this, message));` の下、`}` の上に、3行足します（188〜190行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 188 -->
<table class="code-lines"><tr><td>

```js
188
189
190
```

</td><td>

```js
    webviewView.onDidChangeVisibility(() => {
      if (webviewView.visible) updateUsage(this, false);
    });
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 188〜190 | 欄が見えるようになったら、間があいたアカウントを聞き直す |

- **なぜ:** ほかの欄を見ている間は聞かない（8-17）ので、戻ってきたときに古い値のことがある。
- **効果:** 欄を開くと、間があいていれば新しい値になる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `onDidChangeVisibility` | 見える・見えないが変わったときに動くため | 変わったときに呼ぶ関数を登録する | VSCode の拡張機能の API（`WebviewView`） |
| メンバー変数 | `visible` | 欄が見えているかを知るため | 見えていれば `true` | VSCode の拡張機能の API（`WebviewView`） |

### 8-13. ページの準備ができたら聞く

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **262行目** `view.render();` の下、`return;` の上に、1行足します（263行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 263 -->
<table class="code-lines"><tr><td>

```js
263
```

</td><td>

```js
    updateUsage(view, false);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 263 | 枠を描いたあと、間があいたアカウントを聞き直す |

- **なぜ:** 聞くのには時間がかかるので、先に前の値で描いてから聞く。
- **効果:** 欄を開くとすぐ枠が出て、少しあとで使用量が新しくなる。

### 8-14. 保存などのあとは、すぐ聞き直す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **199行目** `this.render();` の下、`}` の上に、1行足します（200行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 200 -->
<table class="code-lines"><tr><td>

```js
200
```

</td><td>

```js
    updateUsage(this, true);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 200 | 描き直したあと、間を待たずに全部のアカウントを聞き直す |

- **なぜ:** 保存・切り替えで、どのアカウントの鍵を使うかが変わる。
- **効果:** 切り替えると、すぐ使用量が新しくなる。聞き直しのボタン（8-20〜8-22）も、これを呼ぶ。

### 8-15. 保存したら、前の使用量を消す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **216行目** `saveCurrent(name);` の下、`view.refresh();` の上に、1行足します（217行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 217 -->
<table class="code-lines"><tr><td>

```js
217
```

</td><td>

```js
  await forgetUsage(account.accountUuid);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 217 | 保存したアカウントの、前に聞いた使用量を消す。すぐ下の `refresh()` で聞き直す |

- **なぜ:** 「無効」と出ているアカウントにログインし直して保存したとき、すぐに新しい鍵の値に変わるようにするため。
- **効果:** 保存のボタンを押すと、そのアカウントの使用量がすぐ聞き直される。

### 8-16. Claude の会話が進んだことに気づく関数を足す

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **275行目** `}` の下、（空の行） の上に、12行足します（276〜287行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 276 -->
<table class="code-lines"><tr><td>

```js
276
277
278
279
280
281
282
283
284
285
286
287
```

</td><td>

```js

// Claude の会話のファイル（~/.claude/projects の下）が書かれたら、少し待って onChange を呼ぶ。
function watchClaude(onChange) {
  const dir = path.join(HOME, '.claude', 'projects');
  if (!fs.existsSync(dir)) return { dispose() {} };
  let timer;
  const watcher = fs.watch(dir, { recursive: true }, () => {
    clearTimeout(timer);
    timer = setTimeout(onChange, 5000);   // 書き終わるのを待つ
  });
  return { dispose: () => { clearTimeout(timer); watcher.close(); } };
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 278 | Claude が会話を書くフォルダ |
| 279 | フォルダがなければ、何も見張らない（片付けるものもない） |
| 281〜284 | フォルダの中のどこかが書かれたら、5秒待ってから `onChange` を呼ぶ。待つ間にまた書かれたら、待ち直す |
| 285 | 止めるときに、待ちと見張りを止めるものを返す |

- **なぜ:** Claude Code は、会話が1つ進むたびに `~/.claude/projects` の下のファイルに書く。そこを見張れば、使用量が増えたかもしれないときがわかる。会話の間は何度も続けて書かれるので、静かになってから1回だけ呼ぶ。
- **効果:** 会話が進むたびに、使用中のアカウントを聞き直せる（30秒に1回まで）。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `watchClaude` | 会話が進んだことに気づくため | フォルダを見張り、書かれたら `onChange` を呼ぶ。止めるための `{ dispose }` を返す | この手順で作る |
| 引数 | `onChange` | 気づいたときに動かすため | 呼ぶ関数 | 使う側が決める（8-18） |
| 変数 | `dir`・`timer`・`watcher` | 見張るため | 見張るフォルダ、待ちの印、見張り | この手順で作る |
| 関数 | `fs.watch` | フォルダを見張るため | 中のファイルが変わったら関数を呼ぶ。`{ recursive: true }` で下のフォルダも見る | Node.js の標準ライブラリ（fs） |
| 関数 | `setTimeout`・`clearTimeout` | 少し待ってから動かすため | 渡したミリ秒のあとに1回呼ぶ・それをやめる | JavaScript の標準 |
| 関数 | `dispose`・`close` | 止めるときに片付けるため | `subscriptions` に `dispose` を持つものを入れると、止めるときに VSCode が呼ぶ。`close` は見張りを止める | VSCode の拡張機能の決まり・Node.js の標準ライブラリ（fs） |

### 8-17. 欄が見えているときだけ聞く関数を作る

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **291行目** `const view = new AccountsView(context.extensionUri);` の下、`context.subscriptions.push(` の上に、4行足します（292〜295行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 292 -->
<table class="code-lines"><tr><td>

```js
292
293
294
295
```

</td><td>

```js
  // 画面が見えているときだけ、間があいたアカウントを聞き直す
  const update = () => {
    if (view.view && view.view.visible) updateUsage(view, false);
  };
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 293〜295 | 欄があって見えているときだけ、間があいたアカウントを聞き直す |

- **なぜ:** 見ていない間に聞いても、だれも見ない。聞く回数も減る。
- **効果:** 8-18・8-19 から、この関数を呼ぶ。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `update` | 見えているときだけ聞くため | 引数のない関数 | この手順で作る |

### 8-18. 会話が進んだら聞く

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **298行目** `vscode.commands.registerCommand('claudeAccounts.save', () => onSave(view)),` の下、`);` の上に、1行足します（299行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 299 -->
<table class="code-lines"><tr><td>

```js
299
```

</td><td>

```js
    watchClaude(update),
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 299 | 会話のフォルダを見張り、書かれたら `update` を呼ぶ。止めるときの片付けは `subscriptions` に任せる |

- **なぜ:** 「逐一」新しくするため。
- **効果:** Claude の返事が終わると、5秒ほどで使用中のアカウントのバーが動く。

### 8-19. 会話がなくても1分ごとに聞く

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **300行目** `);` の下、`}` の上に、3行足します（301〜303行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 301 -->
<table class="code-lines"><tr><td>

```js
301
302
303
```

</td><td>

```js
  // 会話が進んでいなくても、1分ごとに聞き直す（間があいたアカウントだけ）
  const timer = setInterval(update, 60 * 1000);
  context.subscriptions.push({ dispose: () => clearInterval(timer) });
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 302 | 1分ごとに `update` を呼ぶ |
| 303 | 拡張機能が止まるときに、1分ごとの呼び出しを止める |

- **なぜ:** ほかのアカウント（5分ごと）や、別のPC・claude.ai で使った分は、このPCの会話の見張りでは気づけない。
- **効果:** 使用中は1分ごと、ほかは5分ごとに新しくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `setInterval`・`clearInterval` | 決まった間隔で動かすため | 関数を、渡したミリ秒ごとに呼ぶ・それをやめる | JavaScript の標準 |

### 8-20. 聞き直しのコマンドを決める

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **10行目** `"commands": [` の下、`{ "command": "claudeAccounts.save", "title": "今のアカウントを保存", "icon": "$(add)" }` の上に、1行足します（11行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 11 -->
<table class="code-lines"><tr><td>

```json
11
```

</td><td>

```json
      { "command": "claudeAccounts.refresh", "title": "使用量を聞き直す", "icon": "$(refresh)" },
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 11 | 聞き直しのコマンド。ボタンの絵は回る矢印 |

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `$(refresh)`・`claudeAccounts.refresh` | 聞き直しのボタンにするため | 回る矢印の絵（Codicon）と、聞き直しのコマンドの名前 | VSCode の拡張機能の決まり・この手順で作る |

### 8-21. 見出しに聞き直しのボタンを出す

[package.json](../../../../.claude/extensions/claude-accounts/package.json) の **15行目** `"view/title": [` の下、`{ "command": "claudeAccounts.save", "when": "view == claudeAccounts.list", "group": "navigation@1" }` の上に、1行足します（16行目になります）。

<!-- add: .claude/extensions/claude-accounts/package.json 16 -->
<table class="code-lines"><tr><td>

```json
16
```

</td><td>

```json
        { "command": "claudeAccounts.refresh", "when": "view == claudeAccounts.list", "group": "navigation@2" },
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 16 | 欄の見出しの、保存の右に、聞き直しのボタンを出す |

### 8-22. 聞き直しのコマンドに動きを結びつける

[extension.js](../../../../.claude/extensions/claude-accounts/extension.js) の **298行目** `vscode.commands.registerCommand('claudeAccounts.save', () => onSave(view)),` の下、`watchClaude(update),` の上に、1行足します（299行目になります）。

<!-- add: .claude/extensions/claude-accounts/extension.js 299 -->
<table class="code-lines"><tr><td>

```js
299
```

</td><td>

```js
    vscode.commands.registerCommand('claudeAccounts.refresh', () => view.refresh()),
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 299 | `claudeAccounts.refresh` が呼ばれたら、描き直して、間を待たずに聞き直す（8-14） |

- **なぜ:** 別のPCで使ったあとなど、すぐ新しい値を見たいときのため。
- **効果:** 見出しの回る矢印を押すと、すぐ聞き直す。

### 8-23. 「〇時〇分に復活（あと 〇分）」の文を作る関数を足す（view.js）

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **27行目** `}` の下、（空の行） の上に、12行足します（28〜39行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 28 -->
<table class="code-lines"><tr><td>

```js
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
```

</td><td>

```js

// 「17:10 に復活（あと 47分）」。週の枠は日付も付ける。
function resetText(resetsAt, withDate) {
  // 16:59:59.68 のような時刻を、次の分（17:00）に切り上げる
  const reset = new Date(Math.ceil(Date.parse(resetsAt) / 60000) * 60000);
  const minutes = Math.max(1, Math.floor((reset - Date.now()) / 60000));
  const left = minutes >= 1440 ? `${Math.floor(minutes / 1440)}日${Math.floor((minutes % 1440) / 60)}時間`
    : minutes >= 60 ? `${Math.floor(minutes / 60)}時間${minutes % 60}分` : `${minutes}分`;
  const day = withDate ? reset.toLocaleDateString('ja-JP', { month: 'numeric', day: 'numeric', weekday: 'short' }) + ' ' : '';
  const time = reset.toLocaleTimeString('ja-JP', { hour: '2-digit', minute: '2-digit' });
  return `${day}${time} に復活（あと ${left}）`;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 30 | 復活する時刻（文字）と、日付も出すかを受け取る |
| 32 | 復活の時刻（16:59:59.68 のような値）を、次の分（17:00）に切り上げる |
| 33 | 復活まであと何分か（少なくとも1分） |
| 34〜35 | 1日以上なら「〇日〇時間」、1時間以上なら「〇時間〇分」、それより短ければ「〇分」 |
| 36 | 週の枠なら、「11/20(金)」のような日付も付ける |
| 37 | 「17:00」のような時刻 |
| 38 | 「11/20(金) 07:00 に復活（あと 14時間31分）」のような文にする |

- **なぜ:** API の時刻は 16:59:59.68 のように1秒前を返すので、そのまま切り捨てると「16:59」になり、実際に戻る 17:00 とずれる。「あと〇分」は今の時刻で決まるので、ページで描くたびに作る。
- **効果:** 復活する時刻と、あと何分かが1つの文になる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `resetText` | 復活の文を作るため | 「〇時〇分 に復活（あと 〇）」を返す | この手順で作る |
| 引数 | `resetsAt`・`withDate` | 文を作るため | 復活する時刻（`2026-11-19T08:10:00+00:00` のような文字）と、日付も出すか | 使う側が決める（8-25） |
| 型 | `Date` | 時刻を扱うため | `new Date(ミリ秒)` で時刻を作る。引き算するとミリ秒の差になる | JavaScript の標準 |
| 関数 | `Date.parse` | 時刻の文字を数にするため | 時刻の文字を、ミリ秒にする | JavaScript の標準 |
| 関数 | `Math.ceil`・`Math.floor`・`Math.round`・`Math.max` | 数を丸める・大きい方を取るため | 切り上げ・切り捨て・四捨五入・大きい方 | JavaScript の標準 |
| 変数 | `reset`・`minutes`・`left`・`day`・`time` | 文を組み立てるため | 復活の時刻、あと何分、「あと」の文、日付の文、時刻の文 | この手順で作る |
| 関数 | `toLocaleDateString`・`toLocaleTimeString` | 日本の書き方で日付・時刻を出すため | `'ja-JP'` と、出すもの（月・日・曜日、時・分）を渡すと、「11/20(金)」「07:00」のような文を返す | JavaScript の標準（`Date`） |

### 8-24. 1つの枠を描く関数を作る

まず、値がないときと、何%使ったかを決めるところまでを書きます。

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **39行目** `}` の下、（空の行） の上に、11行足します（40〜50行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 40 -->
<table class="code-lines"><tr><td>

```js
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
```

</td><td>

```js

// 1つの枠（5時間・週）を、見出し・バー・復活の時刻の3段にする。
function limitBlock(label, limit, withDate) {
  if (!limit || !limit.resets_at) return el('div', 'limit', [el('div', 'limit-head', [el('span', '', [label]), el('span', 'dim', ['使用量なし'])])]);
  const reset = Date.parse(limit.resets_at) <= Date.now();
  const used = reset ? 0 : Math.round(limit.utilization);
  const left = 100 - used;
  const level = left <= 10 ? 'danger' : left <= 30 ? 'warn' : '';
  const fill = el('div', `fill ${level}`);
  fill.style.width = `${used}%`;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 42 | 見出し（`5時間の枠`・`週の枠`）、枠の値、日付も出すかを受け取る |
| 43 | 枠の値がなければ、見出しと「使用量なし」だけ |
| 44〜45 | 復活の時刻を過ぎていれば 0%、まだなら使った割合（四捨五入） |
| 46 | 残り |
| 47 | 残りが 10% 以下なら赤、30% 以下なら黄色 |
| 48〜49 | バーの塗り。幅を使った割合にする |

- **なぜ:** 前に聞いた値を出しているとき（鍵の期限切れなど）でも、復活の時刻を過ぎていれば、その枠は 0% に戻っているとわかる。
- **効果:** 8-25 で、バーと文を組み立てられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `limitBlock` | 1つの枠を描くため | 見出しと残り・バー・使用と復活の文の3段を返す | この手順で作る |
| 引数 | `label`・`limit` | 描く枠を受け取るため | 見出しと、枠の値（`fiveHour` か `sevenDay`） | 使う側が決める（8-27） |
| 値 | `utilization`・`resets_at` | 使った割合と復活の時刻 | 8-3 の返事の中身 | Claude のサーバーの返事 |
| 変数 | `used`・`level`・`fill` | バーを描くため | 使った割合、色の名前（`danger`・`warn`・なし）、バーの塗りの要素 | この手順で作る |
| メンバー変数 | `style.width` | バーの長さを決めるため | 要素の幅。`'17%'` なら、外の枠の 17% | ブラウザ（DOM） |

### 8-25. 見出し・バー・文の3段にする

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **49行目** `` fill.style.width = `${used}%`; `` の下、`}` の上に、5行足します（50〜54行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 50 -->
<table class="code-lines"><tr><td>

```js
50
51
52
53
54
```

</td><td>

```js
  return el('div', 'limit', [
    el('div', 'limit-head', [el('span', '', [label]), el('span', `left ${level}`, [`残り ${left}%`])]),
    el('div', 'bar', [fill]),
    el('div', 'dim', [reset ? '復活済み' : `${used}% 使用・${resetText(limit.resets_at, withDate)}`]),
  ]);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 51 | 1段目: 左に見出し、右に「残り 〇%」（色付き） |
| 52 | 2段目: バー（8-33・8-34 の見た目） |
| 53 | 3段目: 「〇% 使用・〇〇 に復活（あと 〇）」。過ぎていれば「復活済み」 |

- **なぜ:** Claude Code の `/usage` と同じく、バーは使った分を塗る。残り（どれだけ使えるか）は数字で大きく出す。
- **効果:** 1つの枠が、見出し・バー・文の3段で描ける。

### 8-26. 取れなかった理由の文を決める

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **55行目** `}` の下、（空の行） の上に、7行足します（56〜62行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 56 -->
<table class="code-lines"><tr><td>

```js
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

const NOTES = {
  invalid: '保存した鍵が無効（ログインし直して、保存し直す）',
  busy: '混んでいて取れなかった（2分後に聞き直す）',
  expired: '鍵の期限切れで聞けない（切り替えると取れる）',
  error: '取れなかった（通信できない など）',
};
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 57〜62 | 8-3 の `status` ごとに、枠に出す文 |

- **なぜ:** 理由ごとに、何をすればよいか（ログインし直す・待つ・切り替える）が違う。
- **効果:** 8-27 で、`NOTES[status]` で文が引ける。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `NOTES` | 理由の文を引くため | `status` の名前から文を引ける | この手順で作る |

### 8-27. アカウントの使用量を描く関数を作る

まず、2つの枠と、取れなかった理由を描くところまでを書きます。

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **62行目** `};` の下、（空の行） の上に、10行足します（63〜72行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 63 -->
<table class="code-lines"><tr><td>

```js
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

function usageBlock(usage) {
  if (!usage) return [el('div', 'dim', ['使用量: まだ取れていない'])];
  const blocks = [];
  if (usage.fiveHour || usage.sevenDay) {
    blocks.push(limitBlock('5時間の枠', usage.fiveHour, false), limitBlock('週の枠', usage.sevenDay, true));
  }
  if (NOTES[usage.status]) blocks.push(el('div', `note ${usage.status === 'invalid' ? 'danger' : ''}`, [NOTES[usage.status]]));
  return blocks;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 64 | アカウントの使用量（8-11 で送ったもの）を受け取る |
| 65 | まだ1回も聞いていなければ、そう書いた1行だけ |
| 67〜69 | 前に取れた値があれば、5時間の枠（日付なし）と週の枠（日付あり）を描く |
| 70 | 取れなかったなら、理由の文を出す。無効なら赤 |
| 71 | 描いたものの配列を返す |

- **なぜ:** 取れなかったときも、前に取れた値はそのまま出す。5時間の枠は今日のうちに戻るので時刻だけ、週の枠は何日も先なので日付も出す。
- **効果:** アカウントの使用量の段ができる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `usageBlock` | アカウントの使用量を描くため | 枠・理由・聞いた時刻の要素の配列を返す | この手順で作る |
| 変数 | `blocks` | 描いたものを持っておくため | 要素の配列 | この手順で作る |

### 8-28. いつ聞いた値かを書き足す

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **70行目** `` if (NOTES[usage.status]) blocks.push(el('div', `note ${usage.status === 'invalid' ? 'danger' : ''}`, [NOTES[usage.status]])); `` の下、`return blocks;` の上に、4行足します（71〜74行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 71 -->
<table class="code-lines"><tr><td>

```js
71
72
73
74
```

</td><td>

```js
  if (usage.fetchedAt) {
    const at = new Date(usage.fetchedAt).toLocaleTimeString('ja-JP', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    blocks.push(el('div', 'fetched', [`${at} に取った値`]));
  }
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 71〜74 | 聞いた時刻を、「16:28:19 に取った値」のように右下に出す |

- **なぜ:** 取れなかったときは前の値のまま出るので、いつの値かがわからないと、今の残りと思い違える。秒まで出すと、聞き直したことがわかる。
- **効果:** 出ている値の新しさがわかる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `at` | 聞いた時刻を出すため | 「16:28:19」のような文 | この手順で作る |

### 8-29. 枠に使用量を入れる

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **86行目** `` const node = el('div', `card ${account.isCurrent ? 'is-current' : ''}`, [head]); `` の下、`if (account.isCurrent) node.append(el('div', 'current', ['使用中']));` の上に、1行足します（87行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 87 -->
<table class="code-lines"><tr><td>

```js
87
```

</td><td>

```js
  node.append(...usageBlock(account.usage));
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 87 | 頭の段の下に、使用量の段を入れる |

- **なぜ:** 3-14 で枠に `append` で足していく形にしたので、1行足すだけで入る。
- **効果:** 枠に、5時間と週のバーが出る。

### 8-30. 「あと〇分」を進める

[view.js](../../../../.claude/extensions/claude-accounts/media/view.js) の **104行目** `});` の下、`vscode.postMessage({ type: 'ready' });` の上に、2行足します（105〜106行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.js 105 -->
<table class="code-lines"><tr><td>

```js
105
106
```

</td><td>

```js
// 「あと〇分」を進めるため、30秒ごとに描き直す（値は extension.js が聞き直して送ってくる）
setInterval(render, 30 * 1000);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 105〜106 | 30秒ごとに描き直す |

- **なぜ:** 「あと〇分」は時間がたつと変わるので、描き直さないと進まない。値を聞き直すのは `extension.js`（8-17〜8-19）で、ページは描き直すだけ。
- **効果:** 「あと〇分」が1分ごとに減っていく。

### 8-31. いつ聞いた値かの見た目を足す（view.css）

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **57行目** `}` の下、（空の行） の上に、7行足します（58〜64行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 58 -->
<table class="code-lines"><tr><td>

```css
58
59
60
61
62
63
64
```

</td><td>

```css

.fetched {
  color: var(--vscode-descriptionForeground);
  font-size: 0.92em;
  text-align: right;
  margin-top: 4px;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 59〜64 | 薄く小さい字で、右寄せにする |

### 8-32. 枠の見出しの段の見た目を足す

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **64行目** `}` の下、（空の行） の上に、15行足します（65〜79行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 65 -->
<table class="code-lines"><tr><td>

```css
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
```

</td><td>

```css

.limit {
  margin-bottom: 8px;
}

.limit-head {
  display: flex;
  justify-content: space-between;
  margin-bottom: 3px;
}

.left {
  font-weight: 600;
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 66〜68 | 5時間の枠と週の枠の間をあける |
| 70〜74 | 見出しを左、「残り 〇%」を右の端に置く |
| 76〜78 | 「残り 〇%」を太字にする |

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `justify-content: space-between` | 両端に置くため | 横に並べた中身を、左端と右端に分ける | ブラウザ（CSS） |

### 8-33. バーの見た目を足す

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **79行目** （空の行） の下、（空の行） の上に、13行足します（80〜92行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 80 -->
<table class="code-lines"><tr><td>

```css
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
```

</td><td>

```css
/* バーは使った分を塗る（Claude Code の /usage と同じ向き） */
.bar {
  height: 6px;
  border-radius: 3px;
  background: var(--vscode-input-background, rgba(128, 128, 128, 0.2));
  overflow: hidden;
  margin-bottom: 3px;
}

.fill {
  height: 100%;
  background: var(--vscode-progressBar-background);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 81〜87 | バーの外の枠。細く、角を丸く、薄い色で塗る。はみ出した塗りは隠す |
| 89〜92 | バーの塗り。高さいっぱいに、VSCode の進み具合のバーと同じ色で塗る（幅は 8-24 で決める） |

- **なぜ:** VSCode の進み具合のバー（読み込み中の線など）と同じ色にすると、見慣れた形になる。
- **効果:** 使った分が青いバーで出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `height` | バーの太さを決めるため | 要素の高さ | ブラウザ（CSS） |

### 8-34. 残りが少ないときのバーの色を足す

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **92行目** `}` の下、（空の行） の上に、8行足します（93〜100行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 93 -->
<table class="code-lines"><tr><td>

```css
93
94
95
96
97
98
99
100
```

</td><td>

```css

.fill.warn {
  background: var(--vscode-editorWarning-foreground);
}

.fill.danger {
  background: var(--vscode-errorForeground);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 94〜96 | 残りが 30% 以下なら、黄色（注意の色） |
| 98〜100 | 残りが 10% 以下なら、赤（エラーの色） |

- **なぜ:** 数字を読まなくても、もうすぐ使えなくなる枠がわかるようにする。
- **効果:** 残りが少ない枠のバーが、黄色・赤になる。

### 8-35. 残りが少ないときの文字の色と、理由の文の見た目を足す

[view.css](../../../../.claude/extensions/claude-accounts/media/view.css) の **100行目** `}` の下、（空の行） の上に、13行足します（101〜113行目になります）。

<!-- add: .claude/extensions/claude-accounts/media/view.css 101 -->
<table class="code-lines"><tr><td>

```css
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
```

</td><td>

```css

.left.warn {
  color: var(--vscode-editorWarning-foreground);
}

.left.danger,
.note.danger {
  color: var(--vscode-errorForeground);
}

.note {
  margin: 4px 0;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 102〜104 | 残りが 30% 以下なら、「残り 〇%」を黄色にする |
| 106〜109 | 残りが 10% 以下の「残り 〇%」と、「保存した鍵が無効」を赤にする |
| 111〜113 | 理由の文の上下をあける |

**確かめ方:**

1. ウィンドウを再読み込みすると、各枠に「5時間の枠」「週の枠」のバーと「残り 〇%」、「〇% 使用・〇〇 に復活（あと 〇）」が出る。
2. 使用中のアカウントの値が、Claude の会話で `/usage` を打ったときの値と合う（`/usage` の「〇% used」が「〇% 使用」、残り = 100 − 使用）。
3. Claude に1つ頼んで返事が終わると、5秒ほどで使用中の枠の「〇〇 に取った値」の時刻が新しくなる（前に聞いてから30秒たっていれば）。
4. 1分たつと「あと〇分」が1つ減る。

---

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
```

</td><td>

```json
{
  "name": "claude-accounts",
  "displayName": "Claude アカウント",
  "description": "保存した Claude のログイン情報を入れ替えて、アカウントを切り替える",
  "version": "0.0.1",
  "publisher": "local",
  "engines": { "vscode": "^1.82.0" },
  "main": "./extension.js",
  "contributes": {
    "commands": [
      { "command": "claudeAccounts.refresh", "title": "使用量を聞き直す", "icon": "$(refresh)" },
      { "command": "claudeAccounts.save", "title": "今のアカウントを保存", "icon": "$(add)" }
    ],
    "menus": {
      "view/title": [
        { "command": "claudeAccounts.refresh", "when": "view == claudeAccounts.list", "group": "navigation@2" },
        { "command": "claudeAccounts.save", "when": "view == claudeAccounts.list", "group": "navigation@1" }
      ]
    },
    "viewsContainers": {
      "activitybar": [
        { "id": "claudeAccounts", "title": "Claude アカウント", "icon": "media/accounts.svg" }
      ]
    },
    "views": {
      "claudeAccounts": [
        { "type": "webview", "id": "claudeAccounts.list", "name": "アカウント" }
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
152
153
154
155
156
157
158
159
160
161
162
163
164
165
166
167
168
169
170
171
172
173
174
175
176
177
178
179
180
181
182
183
184
185
186
187
188
189
190
191
192
193
194
195
196
197
198
199
200
201
202
203
204
205
206
207
208
209
210
211
212
213
214
215
216
217
218
219
220
221
222
223
224
225
226
227
228
229
230
231
232
233
234
235
236
237
238
239
240
241
242
243
244
245
246
247
248
249
250
251
252
253
254
255
256
257
258
259
260
261
262
263
264
265
266
267
268
269
270
271
272
273
274
275
276
277
278
279
280
281
282
283
284
285
286
287
288
289
290
291
292
293
294
295
296
297
298
299
300
301
302
303
304
305
306
307
308
309
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

// 並び順と使用量は、VSCode がこのPCに覚えておく（context.globalState）。
let state;

// 並び順（accountUuid を上から並べた配列）のとおりに並べ替える。並び順にないものは後ろ（名前の順のまま）。
function sortByOrder(profiles) {
  const order = state.get('order', []);
  const rank = (p) => {
    const i = order.indexOf(p.oauthAccount.accountUuid);
    return i < 0 ? order.length : i;
  };
  return profiles.sort((a, b) => rank(a) - rank(b));
}

const USAGE_URL = 'https://api.anthropic.com/api/oauth/usage';
const CURRENT_GAP_MS = 30 * 1000;      // 使用中のアカウントを、続けて聞かない間（Claude の会話が進むたびに聞く）
const OTHER_GAP_MS = 5 * 60 * 1000;    // ほかのアカウントを、続けて聞かない間
const BUSY_WAIT_MS = 2 * 60 * 1000;    // 429（混んでいる）のあと、聞かずに待つ間
const lastAsked = {};                  // アカウントの番号 → 最後に聞いた時刻
const busyUntil = {};                  // アカウントの番号 → この時刻までは聞かない
let updating = false;                  // 聞いている最中か
let again = null;                      // 聞いている最中に頼まれたら、終わったあとにもう一度聞く（null・false・true は force）

// 1つのアカウントの使用量を聞く。返すのは { fiveHour, sevenDay } か、取れなかった理由（status）。
async function fetchUsage(credentials) {
  const oauth = credentials.claudeAiOauth;
  if (!oauth || oauth.expiresAt < Date.now()) return { status: 'expired' };
  const response = await fetch(USAGE_URL, {
    headers: { Authorization: `Bearer ${oauth.accessToken}`, 'anthropic-beta': 'oauth-2025-04-20' },
  });
  // 401 は、鍵がサーバーで無効にされている（/logout したときなど）。429 は、聞く回数が多すぎる
  if (response.status === 401) return { status: 'invalid' };
  if (response.status === 429) return { status: 'busy' };
  if (!response.ok) return { status: 'error' };
  const body = await response.json();
  return { status: 'ok', fiveHour: body.five_hour, sevenDay: body.seven_day, fetchedAt: Date.now() };
}

// 間があいたアカウントの使用量を聞き直す。force なら間を待たない（429 のあとは待つ）。
async function updateUsage(view, force) {
  if (updating) {
    again = again || force;
    return;
  }
  updating = true;
  try {
    const current = currentAccount();
    const usage = state.get('usage', {});
    for (const p of loadProfiles()) {
      const id = p.oauthAccount.accountUuid;
      const isCurrent = current && id === current.accountUuid;
      const gap = isCurrent ? CURRENT_GAP_MS : OTHER_GAP_MS;
      if (Date.now() < (busyUntil[id] || 0)) continue;
      if (!force && Date.now() - (lastAsked[id] || 0) < gap) continue;
      lastAsked[id] = Date.now();
      let result;
      try {
        result = await fetchUsage(isCurrent ? readJson(CREDENTIALS_PATH) : p.credentials);
      } catch {
        result = { status: 'error' };   // 通信できない
      }
      if (result.status === 'busy') busyUntil[id] = Date.now() + BUSY_WAIT_MS;
      // 取れなかったときは、前に取れた値を残して理由だけ替える（理由は毎回替わるので、前の理由は残らない）
      usage[id] = result.status === 'ok' ? result : { ...usage[id], status: result.status };
    }
    await state.update('usage', usage);
  } finally {
    updating = false;
  }
  view.render();
  if (again !== null) {
    const nextForce = again;
    again = null;
    updateUsage(view, nextForce);
  }
}

// 保存し直したアカウントの、前に取った使用量を消す（新しい鍵で取り直すため）。
async function forgetUsage(accountUuid) {
  const usage = state.get('usage', {});
  delete usage[accountUuid];
  await state.update('usage', usage);
  delete lastAsked[accountUuid];
  delete busyUntil[accountUuid];
}

// 画面に渡す中身。アカウントを並び順に並べ、使用量を添える。
function viewData() {
  const current = currentAccount();
  const usage = state.get('usage', {});
  const profiles = loadProfiles();
  sortByOrder(profiles);
  const accounts = profiles.map((p) => ({
    id: p.oauthAccount.accountUuid,
    name: p.name,
    email: p.oauthAccount.emailAddress,
    isCurrent: !!current && p.oauthAccount.accountUuid === current.accountUuid,
    usage: usage[p.oauthAccount.accountUuid] || null,
  }));
  const data = { accounts };
  // 今のアカウントが保存されていなければ、保存を勧める
  if (current && !accounts.some((a) => a.isCurrent)) data.unsaved = current.emailAddress;
  return data;
}

// アクティビティバーに出す画面（Webview）。描くのは media/view.js で、ここは中身を渡すだけ。
class AccountsView {
  constructor(extensionUri) {
    this.extensionUri = extensionUri;
    this.view = null;
  }

  resolveWebviewView(webviewView) {
    this.view = webviewView;
    const webview = webviewView.webview;
    const media = vscode.Uri.joinPath(this.extensionUri, 'media');
    webview.options = { enableScripts: true, localResourceRoots: [media] };
    const nonce = String(Math.random()).slice(2);
    const script = webview.asWebviewUri(vscode.Uri.joinPath(media, 'view.js'));
    const style = webview.asWebviewUri(vscode.Uri.joinPath(media, 'view.css'));
    webview.html = `<!DOCTYPE html><html lang="ja"><head><meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src ${webview.cspSource}; script-src 'nonce-${nonce}';">
<link rel="stylesheet" href="${style}"></head>
<body><div id="root"></div><script nonce="${nonce}" src="${script}"></script></body></html>`;
    webview.onDidReceiveMessage((message) => onMessage(this, message));
    webviewView.onDidChangeVisibility(() => {
      if (webviewView.visible) updateUsage(this, false);
    });
  }

  render() {
    if (this.view) this.view.webview.postMessage(viewData());
  }

  // 保存・切り替え・消したあと。すぐ描き直し、使用量もすぐ聞き直す。
  refresh() {
    this.render();
    updateUsage(this, true);
  }
}

async function onSave(view) {
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
  await forgetUsage(account.accountUuid);
  view.refresh();
}

async function onSwitch(view, profile) {
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
  view.refresh();
  const answer = await vscode.window.showInformationMessage(
    `${profile.name} に切り替えました。開いている Claude に効かせるには、ウィンドウを再読み込みしてください。`,
    '再読み込み');
  if (answer === '再読み込み') vscode.commands.executeCommand('workbench.action.reloadWindow');
}

async function onRemove(view, profile) {
  const answer = await vscode.window.showWarningMessage(
    `${profile.name} の保存を消しますか？（ログアウトはしません）`, { modal: true }, '消す');
  if (answer !== '消す') return;
  fs.unlinkSync(profile.file);
  view.refresh();
}

// 押したアカウントを、1つ上（step = -1）か1つ下（step = 1）と入れ替える。
async function onMove(view, profile, step) {
  const ids = sortByOrder(loadProfiles()).map((p) => p.oauthAccount.accountUuid);
  const from = ids.indexOf(profile.oauthAccount.accountUuid);
  const to = from + step;
  if (to < 0 || to >= ids.length) return;
  [ids[from], ids[to]] = [ids[to], ids[from]];
  await state.update('order', ids);
  view.render();
}

// 画面から届いた知らせ。{ type: 'ready' | 'save' | 'switch' | 'up' | 'down' | 'remove', id }
function onMessage(view, message) {
  if (message.type === 'ready') {
    view.render();
    updateUsage(view, false);
    return;
  }
  if (message.type === 'save') return onSave(view);
  const profile = findProfile(message.id);
  if (!profile) return;
  if (message.type === 'switch') return onSwitch(view, profile);
  if (message.type === 'remove') return onRemove(view, profile);
  if (message.type === 'up') return onMove(view, profile, -1);
  if (message.type === 'down') return onMove(view, profile, 1);
}

// Claude の会話のファイル（~/.claude/projects の下）が書かれたら、少し待って onChange を呼ぶ。
function watchClaude(onChange) {
  const dir = path.join(HOME, '.claude', 'projects');
  if (!fs.existsSync(dir)) return { dispose() {} };
  let timer;
  const watcher = fs.watch(dir, { recursive: true }, () => {
    clearTimeout(timer);
    timer = setTimeout(onChange, 5000);   // 書き終わるのを待つ
  });
  return { dispose: () => { clearTimeout(timer); watcher.close(); } };
}

function activate(context) {
  state = context.globalState;
  const view = new AccountsView(context.extensionUri);
  // 画面が見えているときだけ、間があいたアカウントを聞き直す
  const update = () => {
    if (view.view && view.view.visible) updateUsage(view, false);
  };
  context.subscriptions.push(
    vscode.window.registerWebviewViewProvider('claudeAccounts.list', view, { webviewOptions: { retainContextWhenHidden: true } }),
    vscode.commands.registerCommand('claudeAccounts.save', () => onSave(view)),
    vscode.commands.registerCommand('claudeAccounts.refresh', () => view.refresh()),
    watchClaude(update),
  );
  // 会話が進んでいなくても、1分ごとに聞き直す（間があいたアカウントだけ）
  const timer = setInterval(update, 60 * 1000);
  context.subscriptions.push({ dispose: () => clearInterval(timer) });
}

function deactivate() {}

module.exports = { activate, deactivate };
```

</td></tr></table>

### 出来上がった view.js

<!-- full: .claude/extensions/claude-accounts/media/view.js -->
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
```

</td><td>

```js
// アカウントの一覧を描く（Webview の中で動く）。中身は extension.js から postMessage で届く。
const vscode = acquireVsCodeApi();
const root = document.getElementById('root');
let data = { accounts: [] };

// 要素を作る。children は文字か要素の配列（文字は textContent なので、名前に < などがあっても崩れない）。
function el(tag, className, children = []) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  for (const child of children) node.append(child);
  return node;
}

// 横いっぱいのボタン（切り替える・保存）。押したら extension.js に { type, id } を送る。
function action(text, type, id) {
  const node = el('button', 'action', [text]);
  node.addEventListener('click', () => vscode.postMessage({ type, id }));
  return node;
}

// 小さなボタン（↑・↓・✕）。title はマウスを乗せたときに出る説明。
function button(text, title, type, id) {
  const node = el('button', 'icon', [text]);
  node.title = title;
  node.addEventListener('click', () => vscode.postMessage({ type, id }));
  return node;
}

// 「17:10 に復活（あと 47分）」。週の枠は日付も付ける。
function resetText(resetsAt, withDate) {
  // 16:59:59.68 のような時刻を、次の分（17:00）に切り上げる
  const reset = new Date(Math.ceil(Date.parse(resetsAt) / 60000) * 60000);
  const minutes = Math.max(1, Math.floor((reset - Date.now()) / 60000));
  const left = minutes >= 1440 ? `${Math.floor(minutes / 1440)}日${Math.floor((minutes % 1440) / 60)}時間`
    : minutes >= 60 ? `${Math.floor(minutes / 60)}時間${minutes % 60}分` : `${minutes}分`;
  const day = withDate ? reset.toLocaleDateString('ja-JP', { month: 'numeric', day: 'numeric', weekday: 'short' }) + ' ' : '';
  const time = reset.toLocaleTimeString('ja-JP', { hour: '2-digit', minute: '2-digit' });
  return `${day}${time} に復活（あと ${left}）`;
}

// 1つの枠（5時間・週）を、見出し・バー・復活の時刻の3段にする。
function limitBlock(label, limit, withDate) {
  if (!limit || !limit.resets_at) return el('div', 'limit', [el('div', 'limit-head', [el('span', '', [label]), el('span', 'dim', ['使用量なし'])])]);
  const reset = Date.parse(limit.resets_at) <= Date.now();
  const used = reset ? 0 : Math.round(limit.utilization);
  const left = 100 - used;
  const level = left <= 10 ? 'danger' : left <= 30 ? 'warn' : '';
  const fill = el('div', `fill ${level}`);
  fill.style.width = `${used}%`;
  return el('div', 'limit', [
    el('div', 'limit-head', [el('span', '', [label]), el('span', `left ${level}`, [`残り ${left}%`])]),
    el('div', 'bar', [fill]),
    el('div', 'dim', [reset ? '復活済み' : `${used}% 使用・${resetText(limit.resets_at, withDate)}`]),
  ]);
}

const NOTES = {
  invalid: '保存した鍵が無効（ログインし直して、保存し直す）',
  busy: '混んでいて取れなかった（2分後に聞き直す）',
  expired: '鍵の期限切れで聞けない（切り替えると取れる）',
  error: '取れなかった（通信できない など）',
};

function usageBlock(usage) {
  if (!usage) return [el('div', 'dim', ['使用量: まだ取れていない'])];
  const blocks = [];
  if (usage.fiveHour || usage.sevenDay) {
    blocks.push(limitBlock('5時間の枠', usage.fiveHour, false), limitBlock('週の枠', usage.sevenDay, true));
  }
  if (NOTES[usage.status]) blocks.push(el('div', `note ${usage.status === 'invalid' ? 'danger' : ''}`, [NOTES[usage.status]]));
  if (usage.fetchedAt) {
    const at = new Date(usage.fetchedAt).toLocaleTimeString('ja-JP', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    blocks.push(el('div', 'fetched', [`${at} に取った値`]));
  }
  return blocks;
}

function card(account) {
  const head = el('div', 'card-head', [
    el('span', `dot ${account.isCurrent ? 'on' : ''}`, [account.isCurrent ? '●' : '○']),
    el('div', 'who', [el('div', 'name', [account.name]), el('div', 'dim', [account.email])]),
    button('↑', '上へ', 'up', account.id),
    button('↓', '下へ', 'down', account.id),
    button('✕', '保存を消す（ログアウトはしない）', 'remove', account.id),
  ]);
  const node = el('div', `card ${account.isCurrent ? 'is-current' : ''}`, [head]);
  node.append(...usageBlock(account.usage));
  if (account.isCurrent) node.append(el('div', 'current', ['使用中']));
  if (!account.isCurrent) node.append(action('このアカウントに切り替える', 'switch', account.id));
  return node;
}

function render() {
  const children = data.accounts.map(card);
  // 今のアカウントが保存されていなければ、一番上に保存のボタンを出す
  if (data.unsaved) children.unshift(el('div', 'card', [el('div', 'dim', [`${data.unsaved} は保存されていません。`]), action('今のアカウントを保存', 'save')]));
  if (children.length === 0) children.push(el('div', 'dim', ['Claude で /login してから、上の「+」で今のアカウントを保存してください。']));
  root.replaceChildren(...children);
}

window.addEventListener('message', (event) => {
  data = event.data;
  render();
});
// 「あと〇分」を進めるため、30秒ごとに描き直す（値は extension.js が聞き直して送ってくる）
setInterval(render, 30 * 1000);
vscode.postMessage({ type: 'ready' });
```

</td></tr></table>

### 出来上がった view.css

<!-- full: .claude/extensions/claude-accounts/media/view.css -->
<table class="code-lines"><tr><td>

```css
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
```

</td><td>

```css
/* 色は VSCode のテーマの色（--vscode-〇〇）を使い、明るいテーマでも暗いテーマでも読めるようにする。 */
body {
  padding: 8px 12px;
  color: var(--vscode-foreground);
  font-family: var(--vscode-font-family);
  font-size: var(--vscode-font-size);
}

.card {
  border: 1px solid var(--vscode-widget-border, var(--vscode-panel-border));
  border-radius: 6px;
  padding: 10px;
  margin-bottom: 10px;
}

.card.is-current {
  border-color: var(--vscode-focusBorder);
}

.card-head {
  display: flex;
  align-items: center;
  gap: 4px;
  margin-bottom: 8px;
}

.who {
  flex: 1;
  min-width: 0;
}

.name {
  font-weight: 600;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.dot {
  color: var(--vscode-descriptionForeground);
  margin-right: 4px;
}

.dot.on {
  color: var(--vscode-testing-iconPassed, var(--vscode-charts-green));
}

.dim {
  color: var(--vscode-descriptionForeground);
  font-size: 0.92em;
}

.current {
  margin-top: 6px;
  text-align: center;
  color: var(--vscode-descriptionForeground);
}

.fetched {
  color: var(--vscode-descriptionForeground);
  font-size: 0.92em;
  text-align: right;
  margin-top: 4px;
}

.limit {
  margin-bottom: 8px;
}

.limit-head {
  display: flex;
  justify-content: space-between;
  margin-bottom: 3px;
}

.left {
  font-weight: 600;
}

/* バーは使った分を塗る（Claude Code の /usage と同じ向き） */
.bar {
  height: 6px;
  border-radius: 3px;
  background: var(--vscode-input-background, rgba(128, 128, 128, 0.2));
  overflow: hidden;
  margin-bottom: 3px;
}

.fill {
  height: 100%;
  background: var(--vscode-progressBar-background);
}

.fill.warn {
  background: var(--vscode-editorWarning-foreground);
}

.fill.danger {
  background: var(--vscode-errorForeground);
}

.left.warn {
  color: var(--vscode-editorWarning-foreground);
}

.left.danger,
.note.danger {
  color: var(--vscode-errorForeground);
}

.note {
  margin: 4px 0;
}

button.icon {
  background: none;
  border: none;
  color: var(--vscode-icon-foreground, var(--vscode-foreground));
  cursor: pointer;
  padding: 2px 5px;
  border-radius: 4px;
}

button.icon:hover {
  background: var(--vscode-toolbar-hoverBackground);
}

button.action {
  width: 100%;
  margin-top: 6px;
  padding: 4px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  color: var(--vscode-button-foreground);
  background: var(--vscode-button-background);
}

button.action:hover {
  background: var(--vscode-button-hoverBackground);
}
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
