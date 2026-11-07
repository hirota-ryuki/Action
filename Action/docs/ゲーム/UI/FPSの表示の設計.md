# FPSの表示の設計

作成日: 2026-11-08
対象: [DebugHud](../../../Game/Source/UI/Hud/DebugHud.cpp)
経緯: [UIの経緯](UIの経緯.md)

開発中に画面の右上へ出す FPS の表示のページです。UIManager・部品の分け方は [UIの構成の設計](UIの構成の設計.md) を見てください。

- **FPS はゲームの UI（`DebugHud`）で出す。** エンジン（`K2EngineLow::EndFrame()`）で描いていた処理は、コメントアウトして止めている。
- **開発用の表示は HUD と分ける。** `DebugHud` は PlayerHud・BossHud と並べて UIManager が持ち、いちばん上に描く。
- **Debug・Preview・Release のどのビルドでも出す。** Release で実際の速さを確かめるため。出すかどうかは `DebugHud.cpp` の `kShowFps` 1か所で切り替える。

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
| 値 | 0.5秒ごとに、その間のフレーム数 ÷ 実際にかかった秒数（平均）を出す。毎フレームの値は揺れて読みにくいため |
| 時間の測り方 | `std::chrono::steady_clock` で実際の時間を測る。ゲームの経過時間（`GetFrameDeltaTime()`）は 1/30 秒で頭打ちになるので、それで測ると 30 より下が出ない |
| 場所 | 左寄せ。左端は「FPS 0000.0」が右端から 20 に収まる位置に固定。文字の中央が上端から 20 |
| 起動した直後 | 最初の平均が出るまで「FPS --」と出す |

---

## 2. 実装方法

```cpp
void DebugHud::Update(float deltaTime)
{
    if (!kShowFps) { return; }           // FPS を出さない設定なら何もしない
    const Clock::time_point now = Clock::now();   // 実際の時刻（steady_clock）
    if (!m_isStarted) { m_averageStart = now; m_isStarted = true; }
    m_frameCount++;
    const float elapsed = std::chrono::duration<float>(now - m_averageStart).count();
    if (elapsed >= kFpsAverageSeconds) {          // 0.5秒たったら、平均に書き換えて数え直す
        swprintf_s(text, L"FPS %.1f", m_frameCount / elapsed);
        m_fps.SetText(text);
        m_averageStart = now;
        m_frameCount = 0;
    }
    m_fps.Update(deltaTime);
}
```

| 書き方 | 理由 |
|---|---|
| 時間は `GetFrameDeltaTime()` でなく `steady_clock` で測る | ゲームの経過時間は、ロード直後などに物が大きく飛ばないよう 1/30 秒で頭打ちにしている（[フレームレートの設計](../../エンジン/フレームレートの設計.md)）。それを足すと、30 FPS を下回っても 30 と出てしまう |
| 左寄せにし、左端を「FPS 0000.0」が収まる位置に固定する | 右寄せにすると、数字の幅が変わるたびに文字全体が左右に動いて、ガクガクして見える。左端を固定すれば「FPS」の文字は動かない |
| 出すかどうかを `constexpr bool kShowFps` にする | 今はどのビルドでも true。ビルドで分けたくなったら、ここだけを `#ifdef K2_DEBUG` などで切り替えればよい。false なら `Init()` で `SetVisible(false)` にし、`Update()` もすぐ戻る |
| 文字は 0.5秒に1回だけ書き換える | `Text::SetText` は値が変わったときだけ書き換えるので、毎フレームの文字の作り直しも減る |

### エンジン側で止めたところ

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の `K2EngineLow::EndFrame()` で FPS を描いていた部分を、コメントアウトしている。エンジンだけで動かすとき（ゲームの UI がないとき）は、コメントを外せば右上に出る。文字の幅を測る `Font::MeasureString()` は、ゲームの `Text` が使うので残している。

---

## 3. 使い方

| やりたいこと | すること |
|---|---|
| ほかの開発用の表示を足す（ステート名など） | `DebugHud` に `Text` を足し、`Update()` で値を入れる。値はゲームから const で受け取る |
| Release では FPS を消す | `DebugHud.cpp` の `kShowFps` を、`K2_DEBUG` のときだけ true にする |
| 平均を取る間隔を変える | `kFpsAverageSeconds` を変える |

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
- Debug ビルドでは、実際の FPS が 30 を下回っている。30 を下回ると、ゲームの経過時間が頭打ちになるので、ゲーム全体の進みがゆっくりになる（[フレームレートの設計](../../エンジン/フレームレートの設計.md) の既知の課題）。
