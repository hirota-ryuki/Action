# UIの構成の設計

作成日: 2026-11-08
対象: [Game/Source/UI/](../../../Game/Source/UI/)（UIManager・Hud・部品）
経緯: [UIの経緯](UIの経緯.md)

画面に出す UI（HPゲージ・アイコン・文字）を、どのクラスがどう受け持つかのページです。
HUD の中身（Player 用・Boss 用に何を出すか）は [HUDの設計](HUDの設計.md) を見てください。

- **UI はゲームを読むだけ。** 矢印は UI → ゲームの一方向で、Player や Enemy は UI のことを知らない。UI はキャラクターを `const` で受け取り、値を書き換えない。
- **UI に話しかけるのは Game だけ。** UIManager をどこからでも呼べる形（シングルトン・グローバル変数）にはしない。
- **3段に分ける。** UIManager（まとめ役）→ 〇〇Hud（どの値をどの部品に入れるか）→ 部品（描くだけ）。

---

## 1. 全体像

```
Game ──作る・SetPlayer・ShowBoss / HideBoss──▶ UIManager（IGameObject。1つだけ）
                                                 ├ PlayerHud ──読む（const）──▶ Player
                                                 └ BossHud   ──読む（const）──▶ ボスにする Character
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
| 係 | `PlayerHud`・`BossHud` | キャラクターの値を読み、自分の部品に入れる | 自分のキャラクター（const）と部品 |
| 部品 | `Gauge`・`Icon`・`Text` | 言われた値を描く | エンジンの描画だけ。キャラクターは知らない |

ソースのフォルダ:

| フォルダ | 中身 |
|---|---|
| [UI/](../../../Game/Source/UI/) | [UIManager.h](../../../Game/Source/UI/UIManager.h) |
| [UI/Hud/](../../../Game/Source/UI/Hud/) | [PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h)、[BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) |
| [UI/Elements/](../../../Game/Source/UI/Elements/) | [UIElement.h](../../../Game/Source/UI/Elements/UIElement.h)、[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h)、[Icon.h](../../../Game/Source/UI/Elements/Icon.h)、[Text.h](../../../Game/Source/UI/Elements/Text.h)、[UISprite.h](../../../Game/Source/UI/Elements/UISprite.h)（画像のパス） |

---

## 2. 部品：UIElement

```
UIElement（基底）… 位置・表示する／しない・透明度。Update(deltaTime)・Draw(rc)
 ├ Gauge   … 横向きのバー。SetRate(0〜1)。遅れて減るバーをオンにできる。位置はバーの左端の中央
 ├ Icon    … 画像1枚。位置は画像の中央
 └ Text    … 文字。値が変わったときだけ書き換える。位置は文字の中央
```

- **UIElement を継承するのは、画面に出る部品だけ。** `PlayerHud`・`BossHud` は自分では何も描かないので、継承しないふつうのクラス。
- 座標は画面の中心が原点で、右が +x、上が +y（画面は 1920 × 1080）。
- `Draw()` は、見えない（`SetVisible(false)` か透明度 0）ときは何もせず、見えるときだけ派生クラスの `OnDraw()` を呼ぶ。
- 透明度は `ApplyAlpha()` で各部品の色のアルファに掛ける。フェードはこれで行う。
- 画像がまだないものは、4×4 の白い画像（`Assets/sprite/white.dds`）に `SetMulColor` で色を付けて使う。
- `Button`（押されたら登録した処理を呼ぶ）は、メニュー画面を作るときに作る。

---

## 3. 実装方法

### UIManager は HUD をメンバーで持つ

```cpp
class UIManager : public IGameObject
{
public:
    bool Start() override;   // 各 Hud の Init（画像を読む）
    void Update() override;  // PlayerHud → BossHud の順に Update(deltaTime)
    void Render(RenderContext& rc) override;  // 同じ順に Draw。後に描いたものが上

    void SetPlayer(const Player* player);
    void ShowBoss(const Character* boss, const wchar_t* name);
    void HideBoss();
private:
    PlayerHud m_playerHud;
    BossHud   m_bossHud;
};
```

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

---

## 4. 呼ぶ順番

1. **ゲーム開始**: `Game::Start()` が Player などを作ったあと、`NewGO<UIManager>(3, "uiManager")` を作り、`SetPlayer(m_player)` を呼ぶ。
2. **毎フレーム**: キャラクターが動く（優先度 1）→ `UIManager::Update`（優先度 3）が PlayerHud・BossHud を更新 → 各 Hud が `GetHpRate()` などを読んで部品に入れる。
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
- 比べた案と理由は [UIの経緯](UIの経緯.md) にある。

---

## 7. 既知の課題

- Boss のクラスはまだない。今は `BossHud` が `const Character*` と名前を受け取る。Boss を作ったら `const Boss*` に変え、名前は Boss の Config から取る。
- Game はまだ `ShowBoss` を呼んでいない（ボス戦がないため）。
- ゲージ・アイコンの画像はまだない（白い四角に色を付けている）。
- ダメージの数字のような「一度きりの出来事」を出すときは、読みに行く形では足りない。そのときに知らせ方を決める。

---

## 8. 関連するページ

- [HUDの設計](HUDの設計.md): PlayerHud・BossHud の中身、遅れて減るゲージ
- [キャラクターの構成](../キャラクター/キャラクターの構成.md): Character と Status
- [状態異常の設計](../キャラクター/状態異常の設計.md): 凍結・燃焼（アイコンで出すもの）
