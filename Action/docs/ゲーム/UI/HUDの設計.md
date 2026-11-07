# HUDの設計

作成日: 2026-11-08
対象: [PlayerHud](../../../Game/Source/UI/Hud/PlayerHud.cpp)・[BossHud](../../../Game/Source/UI/Hud/BossHud.cpp)・[Gauge](../../../Game/Source/UI/Elements/Gauge.cpp)
経緯: [UIの経緯](UIの経緯.md)

遊んでいる間ずっと画面に出ている UI（HUD）の中身のページです。
UIManager・部品の分け方は [UIの構成の設計](UIの構成の設計.md) を見てください。

- **キャラクターの種類ごとに Hud を1つ作る。** Player には `PlayerHud`、ボスには `BossHud`。Hud は自分のキャラクターだけを知っている。
- **HPゲージはどちらも同じ `Gauge`。** `Status::GetHpRate()`（0〜1）を入れるだけ。
- **Status はキャラクターごとに分ける。** `PlayerStatus` は `Status` を継承し、Hud はそれぞれの Status を読む（Boss を作ったら `BossStatus` も同じ形にする）。

---

## 1. 全体像

```
PlayerHud ──読む──▶ Player の PlayerStatus・StatusEffects
 ├ Gauge m_hp        … 左上
 ├ Icon  m_frozen    … HP の右。凍結中だけ出す（水色の四角）
 └ Icon  m_burning   … その右。燃焼中だけ出す（橙色の四角）

BossHud ──読む──▶ ボスにする Character の Status と、Show で渡された名前
 ├ Text  m_name      … 画面下の中央
 └ Gauge m_hp        … 名前の下。遅れて減るバー付き
```

---

## 2. PlayerHud と BossHud の違い

| | PlayerHud | BossHud |
|---|---|---|
| 見るもの | `const Player*`（`SetTarget`） | `const Character*` と名前（`Show`）。Boss ができたら `const Boss*` |
| 出すもの | HP、凍結・燃焼のアイコン（あとで魔法） | 名前、HP（あとで第何形態か） |
| 出るとき | ゲーム中ずっと（対象が nullptr なら出さない） | `Show()` から `Hide()` まで（0.3秒のフェード付き） |
| 遅れて減るバー | なし | あり |

---

## 3. 実装方法

### PlayerHud

```cpp
void PlayerHud::Update(float deltaTime)
{
    if (m_player == nullptr) { return; }
    m_hp.SetRate(m_player->GetStatus().GetHpRate());
    const StatusEffects& effects = m_player->GetStatusEffects();
    m_frozen.SetVisible(effects.IsFrozen());
    m_burning.SetVisible(effects.IsBurning());
    m_hp.Update(deltaTime);  // 部品の Update で、位置・大きさ・色を描画用に反映する
    m_frozen.Update(deltaTime);
    m_burning.Update(deltaTime);
}
```

### BossHud

| 関数 | すること |
|---|---|
| `Show(boss, name)` | ボスと名前を覚え、HP を入れ、白いバーを今のHPにそろえる（`ResetTrail`）。透明度の目標を 1 にする |
| `Hide()` | ボスへのポインタをすぐ捨てる（このあと `DeleteGO` してよい）。透明度の目標を 0 にする |
| `Update(deltaTime)` | ボスがいれば HP を入れる。透明度を目標へ一定の速さで近づけ、名前とゲージに同じ値を入れる |

フェードアウトの間は、最後の値のまま描く。透明度が 0 になると、部品の側で描かなくなる。

### Gauge の遅れて減るバー

```
██████████████▒▒▒▒░░░░░░░░
 赤: 今のHP   ▒: 遅れて減る白い部分   ░: 空き（半透明の黒）
```

3枚の白い画像（空き・白いバー・中身）を、左端の中央を基準（ピボット x=0）にして重ね、横の大きさを割合にする。

| 場面 | 赤いバー | 白いバー |
|---|---|---|
| HP が減った | すぐその長さにする | `trailDelay` 秒待ってから、1秒に `trailSpeed` ずつ赤いバーまで縮む |
| HP が増えた | すぐその長さにする | すぐ赤いバーにそろえる（白が残らないように） |
| `ResetTrail()` した | そのまま | 今のHPにそろえる（減っていく演出にならないように） |

`SetRate()` は 0〜1 に収めてから使う。

---

## 4. 使い方

| やりたいこと | すること |
|---|---|
| Player の HUD に MP を足す | `PlayerStatus` に MP を足し、`PlayerHud` に `Gauge m_mp` を足して `Update()` で入れる |
| ボス戦でHPバーを出す | ボス戦を始める側で `ui->ShowBoss(boss, L"名前")`、倒したら `HideBoss()` してから `DeleteGO` |
| ゲージに遅れて減るバーを付ける | `Gauge::Settings` の `useTrail` を true にする |
| 敵の頭上にHPバーを出す | `Gauge` を使い回す。ワールド座標を画面座標に変える処理を足す |

---

## 5. 調整用の数値

| 内容 | 値 | 場所 |
|---|---|---|
| Player の HPゲージ | 480 × 24。左端から 60、上端から 130（FPS の表示と重ならないように） | `PlayerHud.cpp` |
| 状態異常のアイコン | 32 × 32。ゲージの右に 12 空けて並べる | 同上 |
| ボスの HPゲージ | 800 × 20。画面下の中央、下端から 90 | `BossHud.cpp` |
| ボスの名前 | ゲージの 55 上（文字の中央） | 同上 |
| 白いバーが縮み始めるまで | 0.5秒 | 同上（`kTrailDelay`） |
| 白いバーが縮む速さ | 1秒でゲージ全体の 50% | 同上（`kTrailSpeed`） |
| フェードイン・アウト | 0.3秒 | 同上（`kFadeSeconds`） |
| 色 | 中身は赤、空きは半透明の黒、白いバーは白 | `Gauge.h` の `Settings` |

---

## 6. 既知の課題

- Boss のクラス・`BossStatus` はまだない。
- `Text` に渡す文字は、エンジンの `FontRender` が書式として扱うので、`%` は `%%` と書く。
- ボスが2体同時に出るときは、`BossHud` を2つ並べる（`ShowBoss` に何本目かを渡す）。
- HPを何段かに分けるゲージ（第2形態など）は、まだ形を決めていない。
