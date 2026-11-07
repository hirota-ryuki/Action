# FPSの表示の設計

作成日: 2026-11-08
対象: [DebugHud](../../../Game/Source/UI/Hud/DebugHud.cpp)
経緯: [UIの経緯](UIの経緯.md)

開発中に画面の右上へ出す FPS の表示のページです。UIManager・部品の分け方は [UIの構成の設計](UIの構成の設計.md) を見てください。

- **FPS はゲームの UI（`DebugHud`）で出す。** エンジン（`K2EngineLow::EndFrame()`）で描いていた処理は、コメントアウトして止めている。
- **開発用の表示は HUD と分ける。** `DebugHud` は PlayerHud・BossHud と並べて UIManager が持ち、いちばん上に描く。
- **Debug・Preview ビルド（`K2_DEBUG`）のときだけ出す。** Release では何も描かない。

---

## 1. 全体像

```
UIManager（優先度 3）
 ├ PlayerHud
 ├ BossHud
 └ DebugHud  ── Text m_fps … 画面の右上に小さく、右寄せ。「FPS 60.0」
```

| 項目 | 内容 |
|---|---|
| 値 | 0.5秒ごとに、その間のフレーム数 ÷ 経過秒数（平均）を出す。毎フレームの値は揺れて読みにくいため |
| 場所 | 文字の右端が画面の右端から 20、文字の中央が上端から 20 |
| 起動した直後 | 最初の平均が出るまで「FPS --」と出す |

---

## 2. 実装方法

```cpp
void DebugHud::Update(float deltaTime)
{
    if (!kIsDebugBuild) { return; }     // K2_DEBUG でなければ何もしない
    m_elapsed += deltaTime;
    m_frameCount++;
    if (m_elapsed >= kFpsAverageSeconds) {  // 0.5秒たったら、平均に書き換えて数え直す
        swprintf_s(text, L"FPS %.1f", m_frameCount / m_elapsed);
        m_fps.SetText(text);
        m_elapsed = 0.0f;
        m_frameCount = 0;
    }
    m_fps.Update(deltaTime);
}
```

| 書き方 | 理由 |
|---|---|
| `K2_DEBUG` を `constexpr bool` にして使う | `#ifdef` をクラスのあちこちに書かずに済む。Release では `Init()` で `SetVisible(false)` にし、`Update()` もすぐ戻る |
| 文字は `Text`（`Align::Right`）で出す | 右端をそろえる処理を `Text` に任せる。文字の幅が変わっても右端が動かない |
| 文字は 0.5秒に1回だけ書き換える | `Text::SetText` は値が変わったときだけ書き換えるので、毎フレームの文字の作り直しも減る |

### エンジン側で止めたところ

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の `K2EngineLow::EndFrame()` で FPS を描いていた部分を、コメントアウトしている。エンジンだけで動かすとき（ゲームの UI がないとき）は、コメントを外せば右上に出る。文字の幅を測る `Font::MeasureString()` は、ゲームの `Text` が使うので残している。

---

## 3. 使い方

| やりたいこと | すること |
|---|---|
| ほかの開発用の表示を足す（ステート名など） | `DebugHud` に `Text` を足し、`Update()` で値を入れる。値はゲームから const で受け取る |
| Release でも FPS を出す | `DebugHud.cpp` の `kIsDebugBuild` を見ている所を外す |
| 平均を取る間隔を変える | `kFpsAverageSeconds` を変える |

---

## 4. 調整用の数値（`DebugHud.cpp`）

| 内容 | 値 |
|---|---|
| 平均を取る間隔 | 0.5秒 |
| 大きさ | 0.4 |
| 右端・上端からの距離 | 20・20 |
| 色 / 影 | 少し透けた白 / 黒、ずれ 1.5 |

---

## 5. 既知の課題

- 出す／消すをキーで切り替える方法はまだない。
- エンジンの `m_fpsFont`・`m_fpsFontShadow` は、使っていないが作られたままになっている。
