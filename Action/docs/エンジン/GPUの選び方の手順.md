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
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) | 追加・書き換え・削除 | 1・2・3 |

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
| [GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) | 追加 |

### 1-1. `CreateD3DDevice()` の続きを書く

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の **213行目** `SIZE_T videoMemorySize = 0;` の下に、1行足します（214〜214行目になります）。

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

**確かめ方:** k2EngineLow のビルドが通る。

---

## 手順2: 今の GPU より良いかを決める

見つけた GPU で置き換えてよいかを決める関数を、ループの中にラムダで足します。そのメーカーの GPU をまだ選んでいなければ置き換え、選んでいれば専用のビデオメモリが多いときだけ置き換えます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) | 追加 |

### 2-1. 同じメーカーの GPU が複数あるとき（CPU 内蔵の GPU と、専用の GP…

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の **226行目** `}` の下、`if (wcsstr(desc.Description, L"NVIDIA") != nullptr) {` の上に、8行足します（227〜234行目になります）。

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

**確かめ方:** k2EngineLow のビルドが通る（まだ `isBetter` を使っていないので、動きは変わらない）。

---

## 手順3: 良いときだけ置き換える

NVIDIA・AMD・Intel の3か所で、見つけるたびに置き換えていた処理を `if (isBetter(...))` で囲み、置き換えたときにビデオメモリを覚えます。3か所とも同じ形です。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) | 書き換え・削除 |

### 3-1. `CreateD3DDevice()` の中を書き換える

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の **247〜248行目**を、次のように書き換えます（247〜253行目になります）。

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

### 3-2. `CreateD3DDevice()` の中の行を消す

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の **255〜256行目**を消します。

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

### 3-3. `CreateD3DDevice()` の中を書き換える

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の **258〜259行目**を、次のように書き換えます（258〜264行目になります）。

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

### 3-4. `CreateD3DDevice()` の中の行を消す

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の **266〜267行目**を消します。

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

### 3-5. `CreateD3DDevice()` の中を書き換える

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の **269〜270行目**を、次のように書き換えます（269〜275行目になります）。

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

### 3-6. `CreateD3DDevice()` の中の行を消す

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の **277〜278行目**を消します。

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

**確かめ方（これで出来上がり）:**

- ソリューションのビルドが通る。
- CPU 内蔵の GPU と専用の GPU がある PC で起動すると、FPS が上がる（この PC では約 30fps から約 470〜490fps。ウィンドウを最小化して測った値）。
- タスクマネージャーの「パフォーマンス」で、専用の GPU（RX 9070 XT など）の使用率が上がっている。
