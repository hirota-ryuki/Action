# UIの構成の設計

作成日: 2026-11-08
対象: UI 全体（`Game/Source/UI/` に作る予定。**まだ実装していない**）
経緯: [UIの経緯](UIの経緯.md)

画面に出す UI（HPゲージ・アイコン・文字・ボタン）を、どのクラスがどう受け持つかのページです。
HUD の中身（Player 用・Boss 用に何を出すか）は [HUDの設計](HUDの設計.md) を見てください。

- **UI はゲームを読むだけ。** 矢印は UI → ゲームの一方向で、Player や Boss は UI のことを知らない。UI はキャラクターを `const` で受け取り、値を書き換えない。
- **UI に話しかけるのは Game だけ。** UIManager をどこからでも呼べる形（シングルトン・グローバル変数）にはしない。
- **3段に分ける。** UIManager（まとめ役）→ 〇〇Hud（どの値をどの部品に入れるか）→ 部品（描くだけ）。

---

## 1. 全体像

```
Game ──作る・ShowBoss / HideBoss──▶ UIManager（IGameObject。1つだけ）
                                      ├ PlayerHud ──読む（const）──▶ Player
                                      └ BossHud   ──読む（const）──▶ Boss
                                            │ 値を入れる
                                            ▼
                                      部品（UIElement の派生）: Gauge・Icon・Text・Button
                                            │
                                            ▼
                                      SpriteRender / FontRender（エンジン）
```

| 段 | クラス | 受け持つこと | 知っているもの |
|---|---|---|---|
| まとめ役 | `UIManager` | HUD を持ち、順に更新して描く。Game からの指示（ボスを出す・消す）を受ける | 各 Hud |
| 係 | `PlayerHud`・`BossHud` | キャラクターの値を読み、自分の部品に入れる | 自分のキャラクター（const）と部品 |
| 部品 | `Gauge`・`Icon`・`Text`・`Button` | 言われた値を描く | エンジンの描画だけ。キャラクターは知らない |

---

## 2. 部品：UIElement

```
UIElement（基底）… 位置・表示する／しない・透明度・Update()・Draw(rc)
 ├ Gauge   … バー。SetRate(0〜1)。遅れて減るバーをオンにできる
 ├ Icon    … 画像1枚
 ├ Text    … 文字
 └ Button  … 押されたら登録した処理を呼ぶ（メニュー画面を作るときに作る）
```

- **UIElement を継承するのは、画面に出る部品だけ。** `PlayerHud`・`BossHud` は自分では何も描かないので、継承しないふつうのクラスにする。
- 部品はキャラクターを知らないので、HPにもクールダウンにも、Player にも Boss にも同じ `Gauge` を使える。

---

## 3. 実装方法

### UIManager は HUD をメンバーで持つ

```cpp
class UIManager : public IGameObject
{
public:
    void SetPlayer(const Player* player) { m_playerHud.SetTarget(player); }
    void ShowBoss(const Boss* boss)      { m_bossHud.Show(boss); }
    void HideBoss()                      { m_bossHud.Hide(); }

    void Update() override                  { m_playerHud.Update(); m_bossHud.Update(); }
    void Render(RenderContext& rc) override { m_playerHud.Draw(rc); m_bossHud.Draw(rc); }  // 後に描いたものが上

private:
    PlayerHud m_playerHud;
    BossHud   m_bossHud;
};
```

| 書き方 | 理由 |
|---|---|
| HUD は一覧（`vector`）でなくメンバーで持つ | ヘッダーを見れば画面に何があるか全部分かる。HUD が消えて指し先がなくなることがない |
| `NewGO<UIManager>` の優先度は 3 | キャラクター（1）・カメラ（2）が動いたあとの値を表示する。1フレーム遅れない |
| 値は毎フレーム読みに行く（イベントで知らせてもらわない） | HP のように常に変わる値は、読む方が単純。登録の解除漏れが起きない |
| 文字は値が変わったときだけ `SetText` する | 毎フレーム文字列を作り直さない |

### ゲーム側に足すもの

| クラス | 足すもの |
|---|---|
| `Status` | `float GetHpRate() const`（0〜1。最大HPが0なら0） |
| `Character` | `const StatusEffects& GetStatusEffects() const`、`const Status& GetStatus() const` |
| `Player` | `const PlayerStatus& GetStatus() const` |
| `Boss`（新しく作る） | `const BossStatus& GetStatus() const`、`const wchar_t* GetDisplayName() const`（Config から） |

---

## 4. 呼ぶ順番

1. **ゲーム開始**: `Game` が Player と UIManager を作り、`ui->SetPlayer(player)` を呼ぶ。
2. **毎フレーム**: キャラクターが動く（優先度 1）→ `UIManager::Update`（優先度 3）が PlayerHud・BossHud を更新 → 各 Hud が `GetHpRate()` などを読んで部品に入れる。
3. **ボス登場**: `Game` が Boss を作り、`ui->ShowBoss(boss)` を呼ぶ。
4. **ボスを倒した**: `Game` が先に `ui->HideBoss()` を呼び、そのあとで `DeleteGO(boss)` する。
5. **ゲーム終了**: `Game` のデストラクタで、UIManager をキャラクターより先に消す。

4・5 の順番を守れば、消えたキャラクターを UI が読みに行くことはない。順番は Game の中だけで守る。

---

## 5. 使い方

| やりたいこと | すること |
|---|---|
| HUD に表示を足す | その Hud に部品を1つメンバーで足し、`Update()` で値を入れ、`Draw()` で描く |
| 新しいキャラクター用の HUD を作る | `〇〇Hud` を作り、UIManager にメンバーと `Show〇〇` / `Hide〇〇` を足す |
| 新しい種類の部品を作る | `UIElement` を継承し、`Draw()` を書く。キャラクターのことは書かない |

---

## 6. 決めたこと

- UIManager が HUD の種類を知っている形（一覧で持たない）。HUD は数種類で決まっているため。開いたり閉じたりするメニュー画面が増えたら、それだけを画面を重ねて持つ仕組みにする。
- `PlayerHud`・`BossHud` は UIElement を継承しない。UIManager が部品を直接持つ形にもしない（UIManager に全キャラクターの読み方が入って太るため）。
- 比べた案と理由は [UIの経緯](UIの経緯.md) にある。

---

## 7. 既知の課題

- Boss のクラスはまだない。`Enemy` を継承するか、`Character` から作るかは決めていない。
- ゲージの画像はまだない。なければ白い四角の画像を `SetMulColor` で色付けして使う。
- ダメージの数字のような「一度きりの出来事」を出すときは、読みに行く形では足りない。そのときに知らせ方を決める。

---

## 8. 関連するページ

- [HUDの設計](HUDの設計.md): PlayerHud・BossHud の中身、遅れて減るゲージ
- [キャラクターの構成](../キャラクター/キャラクターの構成.md): Character と Status
- [状態異常の設計](../キャラクター/状態異常の設計.md): 凍結・燃焼（アイコンで出すもの）
