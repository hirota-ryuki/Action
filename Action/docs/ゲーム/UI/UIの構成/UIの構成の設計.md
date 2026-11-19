# UIの構成の設計

作成日: 2026-11-08
対象: [Game/Source/UI/](../../../../Game/Source/UI/)（UIManager・Hud・部品）
経緯: [UIの経緯](../UIの経緯.md)
手順: [UIの構成の手順](UIの構成の手順.md)（何もない状態から、どのファイルに何を足すか）

画面に出す UI（HPゲージ・アイコン・文字）を、どのクラスがどう受け持つかのページです。
HUD の中身（Player 用・Boss 用に何を出すか）は [HUDの設計](../HUD/HUDの設計.md) を見てください。

- **UI はゲームを読むだけ。** 矢印は UI → ゲームの一方向で、Player や Enemy は UI のことを知らない。UI はキャラクターを `const` で受け取り、値を書き換えない。
- **UI に話しかけるのは Game だけ。** UIManager をどこからでも呼べる形（シングルトン・グローバル変数）にはしない。
- **3段に分ける。** UIManager（まとめ役）→ 〇〇Hud（どの値をどの部品に入れるか）→ 部品（描くだけ）。

| 用語 | 何のことか |
|---|---|
| HUD | Head-Up Display。遊んでいる間ずっと画面に出ている、HPゲージなどの UI |
| シングルトン | クラスのインスタンスを1つだけ作り、どこからでも呼べるようにする作り方 |
| アルファ | 色の透明さを表す値（0 で見えない、1 で不透明）。`Vector4` の4つ目（`w`）に入っている |
| フェード | 透明度を少しずつ変えて、表示を出したり消したりすること |
| pivot | 画像や文字の「どこを基準に位置を決めるか」。エンジンの画像は割合（0〜1）で、文字は左上からのピクセル数で指定する |
| DDS | [用語集の「描画」](../../../用語集.md#描画) |

---

## 1. 全体像

```
Game ──作る・SetPlayer・ShowBoss / HideBoss──▶ UIManager（IGameObject。1つだけ）
                                                 ├ PlayerHud ──読む（const）──▶ Player
                                                 ├ BossHud   ──読む（const）──▶ ボスにする Character
                                                 └ DebugHud  … 開発用（FPS）。いちばん上に描く
                                                       │ 値を入れる
                                                       ▼
                                                 部品（UIElement の派生）: Gauge・Icon・Text
                                                       │
                                                       ▼
                                                 SpriteRender / FontRender（エンジン）
```

| 段 | クラス | 受け持つこと | 知っているもの |
|---|---|---|---|
| まとめ役 | `UIManager` | HUD を持ち、順に更新して描く。Game からの指示（ボスを出す・消す）を受ける | 各 Hud |
| 係 | `PlayerHud`・`BossHud`・`DebugHud` | キャラクターなどの値を読み、自分の部品に入れる | 自分のキャラクター（const）と部品 |
| 部品 | `Gauge`・`Icon`・`Text` | 言われた値を描く | エンジンの描画だけ。キャラクターは知らない |

ソースのフォルダ:

| フォルダ | 中身 |
|---|---|
| [UI/](../../../../Game/Source/UI/) | [UIManager.h](../../../../Game/Source/UI/UIManager.h) |
| [UI/Hud/](../../../../Game/Source/UI/Hud/) | [PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h)、[BossHud.h](../../../../Game/Source/UI/Hud/BossHud.h)、[DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h)（[FPSの表示の設計](../FPSの表示/FPSの表示の設計.md)） |
| [UI/Elements/](../../../../Game/Source/UI/Elements/) | [UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)、[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h)、[Icon.h](../../../../Game/Source/UI/Elements/Icon.h)、[Text.h](../../../../Game/Source/UI/Elements/Text.h)、[UISprite.h](../../../../Game/Source/UI/Elements/UISprite.h)（画像のパス） |

---

## 2. 部品：UIElement

```
UIElement（基底）… 位置・表示する／しない・透明度。Update(deltaTime)・Draw(rc)
 ├ Gauge   … 横向きのバー。SetRate(0〜1)。枠・遅れて減るバーを付けられる。位置はバーの左端の中央（枠は含まない）
 ├ Icon    … 画像1枚。位置は画像の中央
 └ Text    … 文字。値が変わったときだけ書き換える。位置は、横は Align（左端・中央・右端）、縦は文字の中央。影（縁取り）を付けられる
```

- **UIElement を継承するのは、画面に出る部品だけ。** `PlayerHud`・`BossHud` は自分では何も描かないので、継承しないふつうのクラス。
- 座標は画面の中心が原点で、右が +x、上が +y（画面は 1920 × 1080）。
- `Draw()` は、見えない（`SetVisible(false)` か透明度 0）ときは何もせず、見えるときだけ派生クラスの `OnDraw()` を呼ぶ。
- 透明度は `ApplyAlpha()` で各部品の色のアルファに掛ける。フェードはこれで行う。
- 画像がまだないものは、4×4 の白い画像（`Assets/sprite/white.dds`）に `SetMulColor` で色を付けて使う。ゲージの中身は、上が白く下へ少し暗くなる 4×16 の画像（`Assets/sprite/gauge.dds`）に色を付けて、立体感を出す。
- エンジンの文字（`Font`）は、pivot を割合ではなく「文字の左上からのピクセル数」として扱う。そのため `Text` は、左・中央・右のどこにそろえるか（`Align`）を pivot では決めず、`Font::MeasureString()` で測った幅だけ位置をずらして、そろえる。
- `Button`（押されたら登録した処理を呼ぶ）は、メニュー画面を作るときに作る。

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetMulColor` | 白い画像に色を付けて、ゲージや仮のアイコンにするため | 画像の色に、渡した色を掛ける | k2Engine（エンジン）の [SpriteRender.h](../../../../k2Engine/graphics/SpriteRender.h)（`SpriteRender` のメンバー） |
| 関数 | `ApplyAlpha()` | 透明度（フェード）を、各部品の色に反映するため | 渡した色のアルファに、部品の透明度を掛けた色を返す | ゲームの [UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) の `UIElement` |
| 関数 | `OnDraw()` | 部品ごとの描き方を、派生クラスに書かせるため | 純粋仮想関数。`Draw()` が、見えるときだけ呼ぶ | ゲームの `UIElement.h` の `UIElement`（`Gauge`・`Icon`・`Text` が上書きする） |
| C++ の書き方 | `UISprite` | UI の部品で使う画像のパスを1か所にまとめるため | `WHITE_PATH`（4×4 の白い画像）と `GAUGE_PATH`（ゲージの中身の画像）を持つ | ゲームの [UISprite.h](../../../../Game/Source/UI/Elements/UISprite.h) |
| 型 | `Align` | 文字をどの端にそろえるかを選ぶため | `Left`・`Center`・`Right` を持つ `enum class`。`Text::SetAlign()` で選ぶ | ゲームの [Text.h](../../../../Game/Source/UI/Elements/Text.h) の `Text` |
| 型 | `Font` | 文字の幅を測るため | 文字を描くエンジンのクラス。`MeasureString()` で、文字の幅と高さ（大きさ 1 のとき）を返す | k2EngineLow（エンジン）の [Font.h](../../../../k2EngineLow/graphics/font/Font.h) |

---

## 3. 実装方法

### UIManager は HUD をメンバーで持つ

```cpp
class UIManager : public IGameObject
{
public:
    bool Start() override;   // PlayerHud・BossHud・DebugHud の Init（画像を読む）
    void Update() override;  // PlayerHud → BossHud → DebugHud の順に Update(deltaTime)
    void Render(RenderContext& rc) override;  // 同じ順に Draw。後に描いたものが上

    void SetPlayer(const Player* player);
    void ShowBoss(const Character* boss, const wchar_t* name);
    void HideBoss();
private:
    PlayerHud m_playerHud;
    BossHud   m_bossHud;
    DebugHud  m_debugHud;    // 開発用の表示（FPS）。いちばん上に描く
};
```

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Player` | HPなどを読む相手のプレイヤーを指すため | プレイヤーのクラス。UI は `const` のポインターで受け取り、書き換えない | ゲームの [Player.h](../../../../Game/Source/Player/Player.h) |
| 型 | `Character` | ボスにするキャラクターを指すため | プレイヤー・敵の共通のクラス。`Boss` ができるまでは、これで受け取る | ゲームの [Character.h](../../../../Game/Source/Character/Character.h) |
| C++ の書き方 | `const wchar_t*` | ボスの名前（日本語）を渡すため | 1文字が2バイトの文字（`wchar_t`）の文字列を指すポインター。`L"黒騎士"` のように、引用符の前に `L` を付けて書く | C++ の書き方 |

| 書き方 | 理由 |
|---|---|
| HUD は一覧（`vector`）でなくメンバーで持つ | ヘッダーを見れば画面に何があるか全部分かる。HUD が消えて指し先がなくなることがない |
| `NewGO<UIManager>(3, ...)` | キャラクター（1）・カメラ（2）が動いたあとの値を表示する。1フレーム遅れない |
| 値は毎フレーム読みに行く（イベントで知らせてもらわない） | HP のように常に変わる値は、読む方が単純。登録の解除漏れが起きない |
| Hud の `Init()` は画像を読むだけで、入れた値は消さない | `SetPlayer` は `NewGO` の直後（`Start()` の前）に呼ばれるため |

### ゲーム側に足したもの

| クラス | 足したもの |
|---|---|
| `Status` | `float GetHpRate() const`（0〜1。最大HPが 0 以下なら 0） |
| `Character` | `const StatusEffects& GetStatusEffects() const`、`virtual const Status& GetStatus() const = 0` |
| `Player` / `Enemy` | `const PlayerStatus& GetStatus() const` / `const Status& GetStatus() const` |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Status` | HPなど、遊んでいる間に変わる値をまとめて持つため | HP と最大HP を持つ（`GetHp()`・`GetMaxHp()`・`GetHpRate()`） | ゲームの [Status.h](../../../../Game/Source/Character/Status.h) |
| 型 | `StatusEffects` | 凍結・燃焼のような一時的な状態を持つため | 凍結・燃焼がかかっているかを覚える。UI はこれを読んでアイコンを出す | ゲームの [StatusEffects.h](../../../../Game/Source/Character/Components/StatusEffects.h) |
| 型 | `PlayerStatus` | Player だけの値を足せるようにするため | `Status` を継承したクラス。今は `Status` と同じ | ゲームの [PlayerStatus.h](../../../../Game/Source/Player/PlayerStatus.h) |

---

## 4. 呼ぶ順番

1. **ゲーム開始**: `Game::Start()` が Player などを作ったあと、`NewGO<UIManager>(3, "uiManager")` を作り、`SetPlayer(m_player)` を呼ぶ。
2. **毎フレーム**: キャラクターが動く（優先度 1）→ `UIManager::Update`（優先度 3）が PlayerHud・BossHud・DebugHud を更新 → 各 Hud が `GetHpRate()` などを読んで部品に入れる。
3. **ボス戦の始まり**: ボス戦を始める側（今は Game）が `ShowBoss(boss, 名前)` を呼ぶ。
4. **ボスを倒した**: 先に `HideBoss()` を呼び、そのあとで `DeleteGO(boss)` する。
5. **ゲーム終了**: `Game` のデストラクタで、UIManager をキャラクターより先に `DeleteGO` する。

4・5 の順番を守れば、消えたキャラクターを UI が読みに行くことはない。順番は Game の中だけで守る。

---

## 5. 使い方

| やりたいこと | すること |
|---|---|
| HUD に表示を足す | その Hud に部品を1つメンバーで足し、`Init()` で置き、`Update()` で値を入れ、`Draw()` で描く |
| 新しいキャラクター用の HUD を作る | `UI/Hud/` に `〇〇Hud` を作り、UIManager にメンバーと `Show〇〇` / `Hide〇〇` を足す |
| 新しい種類の部品を作る | `UI/Elements/` に `UIElement` を継承して作り、`Update()` と `OnDraw()` を書く。キャラクターのことは書かない |
| ファイルを足したとき | `Game.vcxproj`・`Game.vcxproj.filters` に足す。フォルダを足したらインクルードパスにも足す |

---

## 6. 決めたこと

- UIManager が HUD の種類を知っている形（一覧で持たない）。HUD は数種類で決まっているため。開いたり閉じたりするメニュー画面が増えたら、それだけを画面を重ねて持つ仕組みにする。
- `PlayerHud`・`BossHud` は UIElement を継承しない。UIManager が部品を直接持つ形にもしない（UIManager に全キャラクターの読み方が入って太るため）。
- 比べた案と理由は [UIの経緯](../UIの経緯.md) にある。

---

## 7. 既知の課題

- Boss のクラスはまだない。今は `BossHud` が `const Character*` と名前を受け取る。Boss を作ったら `const Boss*` に変え、名前は Boss の Config から取る。
- Game はまだ `ShowBoss` を呼んでいない。ボス戦の広間と黒騎士はあり、黒騎士の HP は魔法が当たると減るが、ボスの HP バーは出ない。
- ゲージ・アイコンの画像はまだない（白い四角に色を付けている）。
- ダメージの数字のような「一度きりの出来事」を出すときは、読みに行く形では足りない。そのときに知らせ方を決める。

---

## 8. 関連するページ

- [HUDの設計](../HUD/HUDの設計.md): PlayerHud・BossHud の中身、遅れて減るゲージ
- [キャラクターの構成](../../キャラクター/キャラクターの構成/キャラクターの構成.md): Character と Status
- [状態異常の設計](../../キャラクター/状態異常/状態異常の設計.md): 凍結・燃焼（アイコンで出すもの）
