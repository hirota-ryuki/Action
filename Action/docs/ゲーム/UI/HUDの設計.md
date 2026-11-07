# HUDの設計

作成日: 2026-11-08
対象: `PlayerHud`・`BossHud`・`Gauge`（`Game/Source/UI/` に作る予定。**まだ実装していない**）
経緯: [UIの経緯](UIの経緯.md)

遊んでいる間ずっと画面に出ている UI（HUD）の中身のページです。
UIManager・部品の分け方は [UIの構成の設計](UIの構成の設計.md) を見てください。

- **キャラクターの種類ごとに Hud を1つ作る。** Player には `PlayerHud`、Boss には `BossHud`。Hud は自分のキャラクターだけを知っている。
- **HPゲージはどちらも同じ `Gauge`。** `Status::GetHpRate()`（0〜1）を入れるだけ。
- **Status はキャラクターごとに分ける。** `PlayerStatus`・`BossStatus` は `Status` を継承し、Hud はそれぞれの Status を読む。

---

## 1. 全体像

```
Status            … HP・最大HP・GetHpRate()
 ├ PlayerStatus   … 今は Status と同じ（あとで MP・スタミナ）
 └ BossStatus     … GetPhase()（第何形態か）

PlayerHud ──読む──▶ Player の PlayerStatus・StatusEffects
 ├ Gauge m_hp        … 左上
 ├ Icon  m_frozen    … HP の横。凍結中だけ出す
 └ Icon  m_burning   … HP の横。燃焼中だけ出す

BossHud ──読む──▶ Boss の BossStatus・名前
 ├ Text  m_name      … 画面下の中央
 └ Gauge m_hp        … 名前の下。遅れて減るバー付き
```

---

## 2. PlayerHud と BossHud の違い

| | PlayerHud | BossHud |
|---|---|---|
| 見るもの | `const Player*` | `const Boss*` |
| 読む Status | `PlayerStatus` | `BossStatus` |
| 出すもの | HP、凍結・燃焼のアイコン（あとで魔法） | 名前、HP（あとで第何形態か） |
| 出るとき | ゲーム中ずっと | `Show(boss)` から `Hide()` まで（フェード付き） |
| 遅れて減るバー | なし（あとで付けてもよい） | あり |

---

## 3. 実装方法

### PlayerHud

```cpp
class PlayerHud   // UIElement は継承しない
{
public:
    void SetTarget(const Player* player);
    void Update()
    {
        const PlayerStatus& status = m_player->GetStatus();
        m_hp.SetRate(status.GetHpRate());
        m_frozen.SetVisible(m_player->GetStatusEffects().IsFrozen());
        m_burning.SetVisible(m_player->GetStatusEffects().IsBurning());
    }
    void Draw(RenderContext& rc);
private:
    const Player* m_player = nullptr;
    Gauge m_hp;
    Icon  m_frozen;
    Icon  m_burning;
};
```

### BossHud

```cpp
class BossHud
{
public:
    void Show(const Boss* boss);  // 名前を入れ、遅れて減るバーを今のHPにそろえて、フェードイン
    void Hide();                  // フェードアウトし、m_boss を nullptr にする
    void Update()
    {
        if (m_boss == nullptr) { return; }
        m_hp.SetRate(m_boss->GetStatus().GetHpRate());
    }
    void Draw(RenderContext& rc);
private:
    const Boss* m_boss = nullptr;
    Text  m_name;
    Gauge m_hp;
};
```

- フェードは、`m_name` と `m_hp` の2つに同じ透明度を入れる。
- `Hide()` でポインタを捨てたあとも、フェードアウトが終わるまでは最後の値のまま描く。

### Gauge の遅れて減るバー

```
██████████████▒▒▒▒░░░░░░░░
 赤: 今のHP   ▒: 遅れて減る白い部分   ░: 空き
```

| 場面 | 赤いバー | 白いバー |
|---|---|---|
| HP が減った | すぐその長さにする | 少し待ってから、赤いバーまで一定の速さで縮む |
| HP が増えた | すぐその長さにする | すぐ赤いバーにそろえる（白が残らないように） |
| `Show` した直後 | 今のHP | 今のHPにそろえる（減っていく演出にならないように） |

`SetRate()` は 0〜1 に収めてから使う。

---

## 4. 使い方

| やりたいこと | すること |
|---|---|
| Player の HUD に MP を足す | `PlayerStatus` に MP を足し、`PlayerHud` に `Gauge m_mp` を足して `Update()` で入れる |
| ボスの第何形態かを出す | `BossHud` に部品を足し、`GetPhase()` を入れる |
| 敵の頭上にHPバーを出す | `Gauge` を使い回す。ワールド座標を画面座標に変える処理を足す |

---

## 5. 調整用の数値（実装するときに決める。今は仮）

| 内容 | 仮の値 |
|---|---|
| 白いバーが縮み始めるまで | 0.5秒 |
| 白いバーが縮む速さ | 1秒でゲージ全体の 50% |
| ボスの HUD のフェードイン・アウト | 0.3秒 |
| 位置・大きさ・色 | Player は左上、Boss は画面下の中央 |

---

## 6. 既知の課題

- Boss のクラス・`BossStatus` はまだない。
- ボスが2体同時に出るときは、`BossHud` を2つ並べる（`ShowBoss` に何本目かを渡す）。
- HPを何段かに分けるゲージ（第2形態など）は、まだ形を決めていない。
