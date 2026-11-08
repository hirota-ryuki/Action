# GPUの選び方の手順

作成日: 2026-11-08
設計: [GPUの選び方の設計](GPUの選び方の設計.md)
経緯: [GPUの選び方の経緯](GPUの選び方の経緯.md)

PC に同じメーカーの GPU が2つ以上あるとき（CPU 内蔵の GPU と専用の GPU など）に、専用のビデオメモリが多い方で描くようにするまでの手順です。上から順にやると、[GPUの選び方の設計](GPUの選び方の設計.md) の形になります。

**出来上がるもの:** CPU 内蔵の AMD の GPU と、AMD Radeon RX 9070 XT がある PC で、RX 9070 XT で描くようになります。NVIDIA → AMD → それ以外、というメーカーの順は変わりません。

**前提:**

- Visual Studio 2022 で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 始める前は、エンジンにもとからある形になっている。もとの形では、同じメーカーの GPU を見つけるたびに、あとから見つけた方で置き換える。
- 作業を始める前に、main からブランチを切る。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | メーカーごとに、選んだ GPU のビデオメモリを覚える変数 | ビルドが通る |
| 2 | 今の GPU より良いかを決める関数（ラムダ） | ビルドが通る |
| 3 | 良いときだけ置き換えるようにする | 専用の GPU で描かれ、FPS が上がる |

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) | 追加（`CreateD3DDevice()` に変数1つ、ラムダ1つ）・書き換え（NVIDIA・AMD・Intel の置き換え3か所） | 1〜3 |

新しく作るファイル・消すファイル・コメントアウトする行はありません。

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
| [GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) | 追加（`CreateD3DDevice()` の変数1つ） |

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の `GraphicsEngine::CreateD3DDevice()` で、`SIZE_T videoMemorySize = 0;` の下に**追加**します。

```diff
 		IDXGIAdapter* useAdapter = nullptr;							//最終的に使用するアダプタ。
 		SIZE_T videoMemorySize = 0;
+		SIZE_T venderVideoMemory[Num_GPUVender] = { 0 };				//各ベンダーで選んだアダプターのビデオメモリ。
 		for (int i = 0; dxgiFactory->EnumAdapters(i, &adapterTmp) != DXGI_ERROR_NOT_FOUND; i++) {
```

`Num_GPUVender` は、同じクラスの `GPU_Vender` の列挙にもとからある（変えない）。名前の `vender` も、まわりのコード（`adapterVender`）に合わせています。

**確かめ方:** k2EngineLow のビルドが通る。

---

## 手順2: 今の GPU より良いかを決める

見つけた GPU で置き換えてよいかを決める関数を、ループの中にラムダで足します。そのメーカーの GPU をまだ選んでいなければ置き換え、選んでいれば専用のビデオメモリが多いときだけ置き換えます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) | 追加（`for` の中にラムダ `isBetter`） |

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の `CreateD3DDevice()` の `for` の中、ビデオメモリが一番多い GPU を覚える `if` の閉じかっこの下（NVIDIA を見る `if` の上）に**追加**します。ラムダは `desc`（今見ている GPU の情報）を使うので、ループの中に置きます。

```diff
 				videoMemorySize = desc.DedicatedVideoMemory;
 			}
+			// 同じメーカーの GPU が複数あるとき（CPU 内蔵の GPU と、専用の GPU など）は、専用のビデオメモリが多い方を使う。
+			// 先に見つけた方の方が多ければ、あとから見つけた方では置き換えない。
+			auto isBetter = [&](int vender) {
+				if (adapterVender[vender] == nullptr) {
+					return true;
+				}
+				return desc.DedicatedVideoMemory > venderVideoMemory[vender];
+			};
 			if (wcsstr(desc.Description, L"NVIDIA") != nullptr) {
```

**確かめ方:** k2EngineLow のビルドが通る（まだ `isBetter` を使っていないので、動きは変わらない）。

---

## 手順3: 良いときだけ置き換える

NVIDIA・AMD・Intel の3か所で、見つけるたびに置き換えていた処理を `if (isBetter(...))` で囲み、置き換えたときにビデオメモリを覚えます。3か所とも同じ形です。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) | 書き換え（NVIDIA・AMD・Intel の置き換え3か所） |

### 3-1. NVIDIA

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の NVIDIA の `if` の中、`#endif // #ifdef ENABLE_DXR_ON_RTX_ONLY` の下を**書き換え**ます。レイトレーシングが使えるかを決める `#ifdef ENABLE_DXR_ON_RTX_ONLY` の部分は、もとからある（変えない）。

```diff
 #endif // #ifdef ENABLE_DXR_ON_RTX_ONLY
-				if (adapterVender[GPU_VenderNvidia]) {
-					adapterVender[GPU_VenderNvidia]->Release();
+				if (isBetter(GPU_VenderNvidia)) {
+					if (adapterVender[GPU_VenderNvidia]) {
+						adapterVender[GPU_VenderNvidia]->Release();
+					}
+					adapterVender[GPU_VenderNvidia] = adapterTmp;
+					adapterVender[GPU_VenderNvidia]->AddRef();
+					venderVideoMemory[GPU_VenderNvidia] = desc.DedicatedVideoMemory;
 				}
-				adapterVender[GPU_VenderNvidia] = adapterTmp;
-				adapterVender[GPU_VenderNvidia]->AddRef();
 			}
```

### 3-2. AMD

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の `//AMD製` の下を**書き換え**ます。

```diff
 				//AMD製
-				if (adapterVender[GPU_VenderAMD]) {
-					adapterVender[GPU_VenderAMD]->Release();
+				if (isBetter(GPU_VenderAMD)) {
+					if (adapterVender[GPU_VenderAMD]) {
+						adapterVender[GPU_VenderAMD]->Release();
+					}
+					adapterVender[GPU_VenderAMD] = adapterTmp;
+					adapterVender[GPU_VenderAMD]->AddRef();
+					venderVideoMemory[GPU_VenderAMD] = desc.DedicatedVideoMemory;
 				}
-				adapterVender[GPU_VenderAMD] = adapterTmp;
-				adapterVender[GPU_VenderAMD]->AddRef();
 			}
```

### 3-3. Intel

[GraphicsEngine.cpp](../../k2EngineLow/graphics/GraphicsEngine.cpp) の `//Intel製` の下を**書き換え**ます。

```diff
 				//Intel製
-				if (adapterVender[GPU_VenderIntel]) {
-					adapterVender[GPU_VenderIntel]->Release();
+				if (isBetter(GPU_VenderIntel)) {
+					if (adapterVender[GPU_VenderIntel]) {
+						adapterVender[GPU_VenderIntel]->Release();
+					}
+					adapterVender[GPU_VenderIntel] = adapterTmp;
+					adapterVender[GPU_VenderIntel]->AddRef();
+					venderVideoMemory[GPU_VenderIntel] = desc.DedicatedVideoMemory;
 				}
-				adapterVender[GPU_VenderIntel] = adapterTmp;
-				adapterVender[GPU_VenderIntel]->AddRef();
 			}
```

ループの最後の `adapterTmp->Release();` は、もとからある（変えない）。置き換えたときは `AddRef()` で持ち続け、置き換えなかったときはここで手放します。

**確かめ方（これで出来上がり）:**

- ソリューションのビルドが通る。
- CPU 内蔵の GPU と専用の GPU がある PC で起動すると、FPS が上がる（この PC では約 30fps から約 470〜490fps。ウィンドウを最小化して測った値）。
- タスクマネージャーの「パフォーマンス」で、専用の GPU（RX 9070 XT など）の使用率が上がっている。
