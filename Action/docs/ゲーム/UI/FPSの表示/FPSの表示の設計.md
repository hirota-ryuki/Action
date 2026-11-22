# FPSの表示の設計

作成日: 2026-11-08
対象: [DebugHud](../../../../Game/Source/UI/Hud/DebugHud.cpp)
経緯: [UIの経緯](../UIの経緯.md)
手順: [FPSの表示の手順](FPSの表示の手順.md)（何もない状態から、どのファイルに何を足すか）

開発中に画面の右上へ出す FPS の表示のページです。UIManager・部品の分け方は [UIの構成の設計](../UIの構成/UIの構成の設計.md) を見てください。

- **FPS はゲームの UI（`DebugHud`）で出す。** エンジン（`K2EngineLow::EndFrame()`）で描いていた処理は、コメントアウトして止めている。
- **開発用の表示は HUD と分ける。** `DebugHud` は PlayerHud・BossHud と並べて UIManager が持ち、いちばん上に描く。
- **Debug・Preview・Release のどのビルドでも出す。** Release で実際の速さを確かめるため。出すかどうかは `DebugHud.cpp` の `SHOW_FPS` 1か所で切り替える。

**用語:**

| 用語 | 何のことか |
|---|---|
| FPS | Frames Per Second。1秒に画面を何回描き直すか |
| Debug・Preview・Release | Visual Studio のビルドの設定の名前。Debug はデバッグ用（遅い）、Release は速くしたもの。Preview はこのプロジェクトの設定で、Debug と同じく `K2_DEBUG` が定義される |

---

## 1. 全体像

```
UIManager（優先度 3）
 ├ PlayerHud
 ├ BossHud
 └ DebugHud  ── Text m_fps … 画面の右上に小さく。「FPS 60.0」
```

| 項目 | 内容 |
|---|---|
| 値 | 平均の FPS。0.5秒ごとに、その間のフレーム数 ÷ 実際にかかった秒数を出す。毎フレームの値は揺れて読みにくいため |
| 時間の測り方 | `std::chrono::steady_clock` で実際の時間を測る。ゲームの経過時間（`GetFrameDeltaTime()`）は 1/30 秒で頭打ちになるので、それで測ると 30 より下が出ない |
| 場所 | 左寄せ。左端は「FPS 0000.0」が右端から 20 に収まる位置に固定。文字の中央が上端から 20 |
| 起動した直後 | 最初の平均が出るまで「FPS --」と出す |

---

## 2. 実装方法

```cpp
void DebugHud::Update(float deltaTime)
{
    if (!SHOW_FPS) { return; }           // FPS を出さない設定なら何もしない
    const Clock::time_point now = Clock::now();   // 実際の時刻（steady_clock）
    if (!m_isStarted) { m_averageStart = now; m_isStarted = true; }
    m_frameCount++;
    const float elapsed = std::chrono::duration<float>(now - m_averageStart).count();
    if (elapsed >= FPS_AVERAGE_SECONDS) {          // 0.5秒たったら、平均に書き換えて数え直す
        wchar_t text[64];                          // 表示する文字を入れる配列
        swprintf_s(text, L"FPS %.1f", m_frameCount / elapsed);
        m_fps.SetText(text);
        m_averageStart = now;
        m_frameCount = 0;
    }
    m_fps.Update(deltaTime);
}
```

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Clock` | 長い時計の型名を短く書くため | `std::chrono::steady_clock` の別名（`using Clock = ...`） | ゲームの [DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) の `DebugHud` クラス |
| 型 | `std::chrono::steady_clock` | 実際の時間を測るため | 巻き戻らない時計。`now()` で今の時刻を返す | C++ の標準ライブラリ（`<chrono>`。[DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) で読み込み） |
| 型 | `Clock::time_point` | 時刻を覚えておくため | `now()` が返す、時計の上の1点 | C++ の標準ライブラリ（`std::chrono::steady_clock` のメンバー） |
| 関数 | `Clock::now()` | 今の時刻を知るため | その時点の時刻（`time_point`）を返す | C++ の標準ライブラリ（`std::chrono::steady_clock` のメンバー） |
| メンバー変数 | `m_isStarted` | 最初の1回だけ、平均を取り始めた時刻を決めるため | 最初の `Update()` で `true` にする | ゲームの [DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) |
| メンバー変数 | `m_averageStart` | 何秒たったかを求めるため | 平均を取り始めた時刻 | ゲームの [DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) |
| メンバー変数 | `m_frameCount` | 平均を求めるため | 平均を取り始めてからの、`Update()` を呼んだ回数 | ゲームの [DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) |
| テンプレートの引数 | `std::chrono::duration<float>` | 時刻の差を、秒の `float` で受け取るため | 時間の長さを表す型。`float` を渡すと、秒を `float` で持つ。`.count()` でその数を取り出す | C++ の標準ライブラリ（`<chrono>`） |
| 関数 | `count()` | 時間の長さを、ただの数にするため | `duration` の中身の数（ここでは秒）を返す | C++ の標準ライブラリ（`std::chrono::duration` のメンバー） |
| 関数 | `swprintf_s` | 数字から表示する文字を作るため | 書式（`%.1f` は小数1桁）に合わせて、文字を配列に書き込む。`wchar_t` の文字用 | C の標準ライブラリ（Microsoft の `_s` 付きの安全な版。`<wchar.h>`） |
| 変数 | `text` | 作った文字を一時的に入れておくため | 64文字ぶんの `wchar_t` の配列。`m_fps.SetText()` に渡す | `Update()` の中のローカル変数 |
| 値 | `FPS_AVERAGE_SECONDS` | 平均を取る間隔を1か所で決めるため | 0.5。この秒数がたったら、平均の FPS に書き換える | ゲームの [DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の名前のない `namespace` |

| 書き方 | 理由 |
|---|---|
| 時間は `GetFrameDeltaTime()` でなく `steady_clock` で測る | ゲームの経過時間は、ロード直後などに物が大きく飛ばないよう 1/30 秒で頭打ちにしている（[フレームレートの設計](../../../最適化/描画/フレームレート/フレームレートの設計.md)）。それを足すと、30 FPS を下回っても 30 と出てしまう |
| 左寄せにし、左端を「FPS 0000.0」が収まる位置に固定する | 右寄せにすると、数字の幅が変わるたびに文字全体が左右に動いて、ガクガクして見える。左端を固定すれば「FPS」の文字は動かない |
| 出すかどうかを `constexpr bool SHOW_FPS` にする | 今はどのビルドでも true。ビルドで分けたくなったら、ここだけを `#ifdef K2_DEBUG` などで切り替えればよい。false なら `Init()` で `SetVisible(false)` にし、`Update()` もすぐ戻る |
| 文字は 0.5秒に1回だけ書き換える | `Text::SetText` は値が変わったときだけ書き換えるので、毎フレームの文字の作り直しも減る |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| マクロ | `K2_DEBUG` | Debug・Preview のときだけ処理を入れるため | `#ifdef` で見分けるための印。Debug と Preview のときだけ定義される | プロジェクトの設定（[Game.vcxproj](../../../../Game/Game.vcxproj) の `PreprocessorDefinitions`） |
| 関数 | `SetVisible()` | FPS を出さない設定のとき、描かないようにするため | `false` を渡すと、`Draw()` で何も描かなくなる | ゲームの [UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) |

### エンジン側で止めたところ

[k2EngineLow.cpp](../../../../k2EngineLow/k2EngineLow.cpp) の `K2EngineLow::EndFrame()` で FPS を描いていた部分を、コメントアウトしている。エンジンだけで動かすとき（ゲームの UI がないとき）は、コメントを外せば右上に出る。文字の幅を測る `Font::MeasureString()` は、ゲームの `Text` が使うので残している。変更前（左）と変更後（右）は次のとおり（「行」は変更前の行番号。ほかのファイルの変更は [FPSの表示の手順](FPSの表示の手順.md) の「もとからあるソースの変更前と変更後」）。

| 行 | 変更前 | 変更後 |
|---|---|---|
| 72 | `{` | `{` |
| 73 | `#ifdef K2_DEBUG` | `// FPS の表示は、ゲームの UI（Game/Source/UI/Hud/DebugHud）で出すので、ここでは描かない。` |
| 74 | `m_fpsFont->Begin(g_graphicsEngine->GetRenderContext());` | `// エンジンだけで動かすとき（ゲームの UI がないとき）は、下のコメントを外せば右上に出る。` |
| 75 | `float time = g_gameTime->GetFrameDeltaTime();` | `//#ifdef K2_DEBUG` |
| 76 | `wchar_t text[256];` | `//		m_fpsFont->Begin(g_graphicsEngine->GetRenderContext());` |
| 77 | `swprintf(text, L"FPS = %0.2f", 1.0f / time);` | `//		float time = g_gameTime->GetFrameDeltaTime();` |
| 78 | `m_fpsFontShadow->Draw(text, { UI_SPACE_WIDTH * -0.48f + 3.0f , UI_SPACE_HEIGHT * 0.48f - 3.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f, 1.0f, { 0.0f, 1.0f });` | `//		wchar_t text[256];` |
| 79 | `m_fpsFont->Draw(text, { UI_SPACE_WIDTH * -0.48f, UI_SPACE_HEIGHT * 0.48f }, { 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f, 1.0f, { 0.0f, 1.0f });` | `//		swprintf(text, L"FPS = %0.2f", 1.0f / time);` |
| 80 | `m_fpsFont->End(g_graphicsEngine->GetRenderContext());` | `//		const float fpsScale = 0.5f;` |
| 81 | `#endif` | `//		const float fpsShadowOffset = 1.5f;` |
|  |  | `//		// 右端をそろえるため、文字の幅だけ左から描き始める。位置は文字の左上。` |
|  |  | `//		const float fpsWidth = m_fpsFont->MeasureString(text).x * fpsScale;` |
|  |  | `//		const Vector2 fpsPosition = { UI_SPACE_WIDTH * 0.49f - fpsWidth, UI_SPACE_HEIGHT * 0.49f };` |
|  |  | `//		m_fpsFontShadow->Draw(text, { fpsPosition.x + fpsShadowOffset, fpsPosition.y - fpsShadowOffset }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f, fpsScale, { 0.0f, 1.0f });` |
|  |  | `//		m_fpsFont->Draw(text, fpsPosition, { 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f, fpsScale, { 0.0f, 1.0f });` |
|  |  | `//		m_fpsFont->End(g_graphicsEngine->GetRenderContext());` |
|  |  | `//#endif` |
| 82 | `m_graphicsEngine->EndRender();` | `m_graphicsEngine->EndRender();` |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `K2EngineLow::EndFrame()` | 毎フレームの最後に、エンジンが決まった処理をするため | 1フレームの最後に呼ばれる。以前はここで FPS を描いていた | k2EngineLow（エンジン）の [k2EngineLow.cpp](../../../../k2EngineLow/k2EngineLow.cpp) |
| 関数 | `Font::MeasureString()` | 文字の幅を測って、位置をずらすため | 文字の幅と高さ（大きさ 1 のとき）を返す | k2EngineLow（エンジン）の [Font.h](../../../../k2EngineLow/graphics/font/Font.h) |
| メンバー変数 | `m_fpsFont`・`m_fpsFontShadow` | エンジンが FPS の文字と影を描くため | `Font` を持つ `std::unique_ptr`。今は使っていない | k2EngineLow（エンジン）の [k2EngineLow.h](../../../../k2EngineLow/k2EngineLow.h) |

---

## 3. 使い方

| やりたいこと | すること |
|---|---|
| ほかの開発用の表示を足す（ステート名など） | `DebugHud` に `Text` を足し、`Update()` で値を入れる。値はゲームから const で受け取る |
| Release では FPS を消す | `DebugHud.cpp` の `SHOW_FPS` を、`K2_DEBUG` のときだけ true にする |
| 平均を取る間隔を変える | `FPS_AVERAGE_SECONDS` を変える |

---

## 4. 調整用の数値（`DebugHud.cpp`）

| 内容 | 値 |
|---|---|
| 平均を取る間隔 | 0.5秒 |
| 大きさ | 0.4 |
| 右端・上端からの距離 | 20・20（右端は「FPS 0000.0」の右端） |
| 色 / 影 | 少し透けた白 / 黒、ずれ 1.5 |

---

## 5. 既知の課題

- 出す／消すをキーで切り替える方法はまだない。
- エンジンの `m_fpsFont`・`m_fpsFontShadow` は、使っていないが作られたままになっている。
- Debug ビルドでは、実際の FPS が 30 を下回っている。30 を下回ると、ゲームの経過時間が頭打ちになるので、ゲーム全体の進みがゆっくりになる（[フレームレートの設計](../../../最適化/描画/フレームレート/フレームレートの設計.md) の既知の課題）。
