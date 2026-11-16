# GPUの選び方の手順

設計: [GPUの選び方の設計](GPUの選び方の設計.md)
経緯: [GPUの選び方の経緯](GPUの選び方の経緯.md)

PC に同じメーカーの GPU が2つ以上あるとき（CPU 内蔵の GPU と専用の GPU など）に、専用のビデオメモリが多い方で描くようにするまでの手順です。上から順にやると、[GPUの選び方の設計](GPUの選び方の設計.md) の形になります。

**出来上がるもの:** CPU 内蔵の AMD の GPU と、AMD Radeon RX 9070 XT がある PC で、RX 9070 XT で描くようになります。NVIDIA → AMD → それ以外、というメーカーの順は変わりません。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 始める前は、エンジンにもとからある形になっている。もとの形では、同じメーカーの GPU を見つけるたびに、あとから見つけた方で置き換える。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | メーカーごとに、選んだ GPU のビデオメモリを覚える変数 | ビルドが通る |
| 2 | 今の GPU より良いかを決める関数（ラムダ） | ビルドが通る |
| 3 | 良いときだけ置き換えるようにする | 専用の GPU で描かれ、FPS が上がる |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- 消すところは、消す行を枠で示します（番号は消す前の番号）。書き換えるところは、「前」と「後」の枠を並べています。
- 小手順の見出しは、足すコードのコメントか関数の名前です。何のためのコードかは、コードのコメントに書いてあります。
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる変数・引数・関数・型・マクロ・C++ の書き方を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、このゲームで作ったもの・エンジン（k2Engine）の機能・C++ の標準ライブラリ・C++ の書き方のどれかがわかります。
- [前提知識](../../../前提知識.md) で「よく知っている」にあるもの（ラムダ・`nullptr`・`auto`・配列・`if` など）は、表に載せません。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) | 追加・書き換え・削除 | 1・2・3 |

---

**もとからあって、変えないもの:** `CreateD3DDevice()` の後半の「使用するアダプターを決める」（NVIDIA → AMD → ビデオメモリが一番多いもの）の `if` は、そのまま使います。

```cpp
		//使用するアダプターを決める。
		if (adapterVender[GPU_VenderNvidia] != nullptr) {
			//NVIDIA製が最優先
			useAdapter = adapterVender[GPU_VenderNvidia];
		}
		else if (adapterVender[GPU_VenderAMD] != nullptr) {
			//次はAMDが優先。
			useAdapter = adapterVender[GPU_VenderAMD];
		}
		else {
			//NVIDIAとAMDのGPUがなければビデオメモリが一番多いやつを使う。
			useAdapter = adapterMaxVideoMemory;
		}
```

---

## 手順1: 選んだ GPU のビデオメモリを覚える

同じメーカーの GPU を比べるために、メーカーごとに「今選んでいる GPU の専用のビデオメモリ」を覚えておく配列を足します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) | 追加 |

### 1-1. `CreateD3DDevice()` の続きを書く

[GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) の **213行目** `SIZE_T videoMemorySize = 0;` の下に、1行足します（214〜214行目になります）。

<!-- add: Action/k2EngineLow/graphics/GraphicsEngine.cpp 214 -->
<table class="code-lines"><tr><td>

```cpp
214
```

</td><td>

```cpp
		SIZE_T venderVideoMemory[Num_GPUVender] = { 0 };				//各ベンダーで選んだアダプターのビデオメモリ。
```

</td></tr></table>

- **なぜ:** 同じメーカーの GPU を比べるには、今選んでいる GPU のビデオメモリを覚えておく場所が要る。メーカーごとに分ければ、NVIDIA と AMD を混ぜずに比べられる。
- **効果:** 次の手順で、見つけた GPU と選んだ GPU のビデオメモリを比べられるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `venderVideoMemory` | メーカーごとに、選んだ GPU のビデオメモリを覚えておくため | メーカーの数だけ並んだ配列。`{ 0 }` で全部 0 にしてから使う | この行で作る |
| 値 | `Num_GPUVender` | 配列の大きさを、メーカーの数と同じにするため | メーカーの種類の数（NVIDIA・AMD・Intel で 3） | k2EngineLow（エンジン）の [GraphicsEngine.h](../../../../k2EngineLow/graphics/GraphicsEngine.h) |
| 型 | `SIZE_T` | ビデオメモリの大きさ（バイト数）を入れるため | 大きな数が入る整数。64ビットのプログラムでは 64ビット | Windows の API |

**確かめ方:** k2EngineLow のビルドが通る。

---

## 手順2: 今の GPU より良いかを決める

見つけた GPU で置き換えてよいかを決める関数を、ループの中にラムダで足します。そのメーカーの GPU をまだ選んでいなければ置き換え、選んでいれば専用のビデオメモリが多いときだけ置き換えます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) | 追加 |

### 2-1. 同じメーカーの GPU が複数あるとき（CPU 内蔵の GPU と、専用の GP…

[GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) の **226行目** `}` の下、`if (wcsstr(desc.Description, L"NVIDIA") != nullptr) {` の上に、8行足します（227〜234行目になります）。

<!-- add: Action/k2EngineLow/graphics/GraphicsEngine.cpp 227 -->
<table class="code-lines"><tr><td>

```cpp
227
228
229
230
231
232
233
234
```

</td><td>

```cpp
			// 同じメーカーの GPU が複数あるとき（CPU 内蔵の GPU と、専用の GPU など）は、専用のビデオメモリが多い方を使う。
			// 先に見つけた方の方が多ければ、あとから見つけた方では置き換えない。
			auto isBetter = [&](int vender) {
				if (adapterVender[vender] == nullptr) {
					return true;
				}
				return desc.DedicatedVideoMemory > venderVideoMemory[vender];
			};
```

</td></tr></table>

- **なぜ:** ループの中で見つけるたびに置き換えると、あとから見つけた方が必ず残る。CPU 内蔵の GPU があとに来ると、専用の GPU が消えてしまう。比べる処理を1つの関数にまとめれば、3つのメーカーで同じ書き方ができる。
- **効果:** 「置き換えてよいか」を `isBetter(メーカー)` の1行で聞けるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `isBetter` | 置き換えてよいかを、3か所で同じように聞くため | ラムダを入れた変数。メーカーの番号を渡すと、`true`（置き換える）か `false`（置き換えない）を返す | この行で作る |
| 引数 | `vender` | どのメーカーのことかを、呼ぶ側から受け取るため | メーカーの番号（`GPU_VenderNvidia` など） | 呼ぶ側が決める |
| 変数 | `adapterVender` | メーカーごとに、選んだ GPU を持っておくため | メーカーの数だけ並んだ GPU（`IDXGIAdapter*`）の配列。まだ選んでいなければ `nullptr` | k2EngineLow（エンジン）の [GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp)（もとからある） |
| 変数 | `desc` | 今見ている GPU の情報を読むため | GPU の名前やビデオメモリの大きさなどを持つ構造体 `DXGI_ADAPTER_DESC` | k2EngineLow（エンジン）の [GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp)（もとからある） |
| メンバー変数 | `DedicatedVideoMemory` | 専用のビデオメモリの大きさで、GPU の良し悪しを決めるため | その GPU 専用のビデオメモリの大きさ（バイト数）。CPU 内蔵の GPU は、メインメモリを借りるので小さい | DirectX 12（DXGI）の `DXGI_ADAPTER_DESC` |

**確かめ方:** k2EngineLow のビルドが通る（まだ `isBetter` を使っていないので、動きは変わらない）。

---

## 手順3: 良いときだけ置き換える

NVIDIA・AMD・Intel の3か所で、見つけるたびに置き換えていた処理を `if (isBetter(...))` で囲み、置き換えたときにビデオメモリを覚えます。3か所とも同じ形です。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) | 書き換え・削除 |

### 3-1. `CreateD3DDevice()` の中を書き換える

[GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) の **247〜248行目**を、次のように書き換えます（247〜253行目になります）。

**前**

<!-- rewrite-before: Action/k2EngineLow/graphics/GraphicsEngine.cpp 247 -->
<table class="code-lines"><tr><td>

```cpp
247
248
```

</td><td>

```cpp
				if (adapterVender[GPU_VenderNvidia]) {
					adapterVender[GPU_VenderNvidia]->Release();
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/k2EngineLow/graphics/GraphicsEngine.cpp 247 -->
<table class="code-lines"><tr><td>

```cpp
247
248
249
250
251
252
253
```

</td><td>

```cpp
				if (isBetter(GPU_VenderNvidia)) {
					if (adapterVender[GPU_VenderNvidia]) {
						adapterVender[GPU_VenderNvidia]->Release();
					}
					adapterVender[GPU_VenderNvidia] = adapterTmp;
					adapterVender[GPU_VenderNvidia]->AddRef();
					venderVideoMemory[GPU_VenderNvidia] = desc.DedicatedVideoMemory;
```

</td></tr></table>

- **なぜ:** 見つけた GPU の方が良いときだけ、前の GPU を手放して入れ替える。`isBetter` が `false` なら何もしない。入れ替えたときは、ビデオメモリも覚え直す。
- **効果:** NVIDIA の GPU が複数あるとき、専用のビデオメモリが多い方が残る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `GPU_VenderNvidia` | メーカーを番号で区別するため | NVIDIA を表す番号（`adapterVender` などの添え字に使う） | k2EngineLow（エンジン）の [GraphicsEngine.h](../../../../k2EngineLow/graphics/GraphicsEngine.h) |
| 変数 | `adapterTmp` | 今見ている GPU を持つため | `for` で1つずつ取り出した GPU（`IDXGIAdapter*`） | k2EngineLow（エンジン）の [GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp)（もとからある） |
| 関数 | `Release` | 使い終わった GPU を手放すため | 参照の数を1つ減らす。0 になると GPU の情報が消える | DirectX 12（COM の関数） |
| 関数 | `AddRef` | `adapterTmp` を、あとでも使えるようにするため | 参照の数を1つ増やす。ループの最後に `adapterTmp` を `Release` しても、この GPU は残る | DirectX 12（COM の関数） |

### 3-2. `CreateD3DDevice()` の中の行を消す

[GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) の **255〜256行目**を消します。

<!-- del: Action/k2EngineLow/graphics/GraphicsEngine.cpp 255 -->
<table class="code-lines"><tr><td>

```cpp
255
256
```

</td><td>

```cpp
				adapterVender[GPU_VenderNvidia] = adapterTmp;
				adapterVender[GPU_VenderNvidia]->AddRef();
```

</td></tr></table>

- **なぜ:** 入れ替えの処理は、3-1 で `if (isBetter(...))` の中に書いた。もとの入れ替えの2行が外に残ると、良くなくても必ず置き換わってしまう。
- **効果:** 置き換えるのが「良いとき」だけになる。

### 3-3. `CreateD3DDevice()` の中を書き換える

[GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) の **258〜259行目**を、次のように書き換えます（258〜264行目になります）。

**前**

<!-- rewrite-before: Action/k2EngineLow/graphics/GraphicsEngine.cpp 258 -->
<table class="code-lines"><tr><td>

```cpp
258
259
```

</td><td>

```cpp
				if (adapterVender[GPU_VenderAMD]) {
					adapterVender[GPU_VenderAMD]->Release();
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/k2EngineLow/graphics/GraphicsEngine.cpp 258 -->
<table class="code-lines"><tr><td>

```cpp
258
259
260
261
262
263
264
```

</td><td>

```cpp
				if (isBetter(GPU_VenderAMD)) {
					if (adapterVender[GPU_VenderAMD]) {
						adapterVender[GPU_VenderAMD]->Release();
					}
					adapterVender[GPU_VenderAMD] = adapterTmp;
					adapterVender[GPU_VenderAMD]->AddRef();
					venderVideoMemory[GPU_VenderAMD] = desc.DedicatedVideoMemory;
```

</td></tr></table>

- **なぜ:** NVIDIA と同じ形を、AMD にも書く。3つのメーカーで同じ形にそろえると、読み間違えにくい。
- **効果:** AMD の GPU が複数あるとき、専用のビデオメモリが多い方が残る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `GPU_VenderAMD` | メーカーを番号で区別するため | AMD を表す番号 | k2EngineLow（エンジン）の [GraphicsEngine.h](../../../../k2EngineLow/graphics/GraphicsEngine.h) |

### 3-4. `CreateD3DDevice()` の中の行を消す

[GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) の **266〜267行目**を消します。

<!-- del: Action/k2EngineLow/graphics/GraphicsEngine.cpp 266 -->
<table class="code-lines"><tr><td>

```cpp
266
267
```

</td><td>

```cpp
				adapterVender[GPU_VenderAMD] = adapterTmp;
				adapterVender[GPU_VenderAMD]->AddRef();
```

</td></tr></table>

- **なぜ:** 3-2 と同じ理由。もとの2行が外に残ると、良くなくても必ず置き換わる。
- **効果:** AMD でも、置き換えるのが「良いとき」だけになる。

### 3-5. `CreateD3DDevice()` の中を書き換える

[GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) の **269〜270行目**を、次のように書き換えます（269〜275行目になります）。

**前**

<!-- rewrite-before: Action/k2EngineLow/graphics/GraphicsEngine.cpp 269 -->
<table class="code-lines"><tr><td>

```cpp
269
270
```

</td><td>

```cpp
				if (adapterVender[GPU_VenderIntel]) {
					adapterVender[GPU_VenderIntel]->Release();
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/k2EngineLow/graphics/GraphicsEngine.cpp 269 -->
<table class="code-lines"><tr><td>

```cpp
269
270
271
272
273
274
275
```

</td><td>

```cpp
				if (isBetter(GPU_VenderIntel)) {
					if (adapterVender[GPU_VenderIntel]) {
						adapterVender[GPU_VenderIntel]->Release();
					}
					adapterVender[GPU_VenderIntel] = adapterTmp;
					adapterVender[GPU_VenderIntel]->AddRef();
					venderVideoMemory[GPU_VenderIntel] = desc.DedicatedVideoMemory;
```

</td></tr></table>

- **なぜ:** NVIDIA・AMD と同じ形を、Intel にも書く。
- **効果:** Intel の GPU が複数あるときも、専用のビデオメモリが多い方が残る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `GPU_VenderIntel` | メーカーを番号で区別するため | Intel を表す番号 | k2EngineLow（エンジン）の [GraphicsEngine.h](../../../../k2EngineLow/graphics/GraphicsEngine.h) |

### 3-6. `CreateD3DDevice()` の中の行を消す

[GraphicsEngine.cpp](../../../../k2EngineLow/graphics/GraphicsEngine.cpp) の **277〜278行目**を消します。

<!-- del: Action/k2EngineLow/graphics/GraphicsEngine.cpp 277 -->
<table class="code-lines"><tr><td>

```cpp
277
278
```

</td><td>

```cpp
				adapterVender[GPU_VenderIntel] = adapterTmp;
				adapterVender[GPU_VenderIntel]->AddRef();
```

</td></tr></table>

- **なぜ:** 3-2 と同じ理由。もとの2行が外に残ると、良くなくても必ず置き換わる。
- **効果:** 3つのメーカーすべてで、置き換えるのが「良いとき」だけになる。

**確かめ方（これで出来上がり）:**

- ソリューションのビルドが通る。
- CPU 内蔵の GPU と専用の GPU がある PC で起動すると、FPS が上がる（この PC では約 30fps から約 470〜490fps。ウィンドウを最小化して測った値）。
- タスクマネージャーの「パフォーマンス」で、専用の GPU（RX 9070 XT など）の使用率が上がっている。
