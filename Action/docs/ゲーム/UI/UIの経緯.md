# UIの経緯

設計: [UIの構成の設計](UIの構成の設計.md)、[HUDの設計](HUDの設計.md)

---

## 2026-11-07〜08: 設計を決めた（まだ実装していない）

UI を作る前に、結合と凝集・バグの起きにくさ・管理のしやすさ・使いやすさを考えて、形を相談した。

### 部品を継承の形にした

- 最初は「Hud が HudData（ただの struct）を作り、Widget に渡す」形を出したが、分かりにくかった。
- `UIElement` を継承した `Gauge`・`Icon`・`Text`・`Button` にした。部品ごとの役目が名前で分かる。
- `Button` は選択と決定の入力が要るので、HUD ではなくメニュー画面を作るときに作る。

### Hud をキャラクターごとに分けた

- Boss も Player と同じように `Status` を分ける（`BossStatus`）前提にした。
- そのうえで、`PlayerHud` と `BossHud` を別のクラスにした。それぞれ自分のキャラクターだけを読み、HPゲージには同じ `Gauge` を使う。
- UI が `FindGO` でボスを探すのではなく、ボス戦を始めた側（今は Game）が `ShowBoss(boss)` で渡す形にした。消えたボスを読みに行かないよう、`HideBoss()` してから `DeleteGO` する。

### UIManager でまとめた

比べた案:

| | 案A：HUD をメンバーで持つ | 案B：UIElement の一覧で持つ |
|---|---|---|
| 分かりやすさ | ヘッダーで全部分かる | どこで Add したか探す |
| バグ | HUD が消えることがない | 一覧から外したあとのポインタで落ちる |
| 向いている場面 | HUD が数種類で決まっている | 画面が何十種類もある |

HUD は数種類なので案Aにした。

- UIManager をシングルトンにする案はやめた。Boss が自分で UIManager を呼べると、ゲームが UI を知ることになるため。

### PlayerHud・BossHud は UIElement を継承しない

- 最初は、まとめて動かす・透明にするために `PlayerHud`・`BossHud` も UIElement を継承する形にしていた。
- 「部品を持つだけなら継承しなくてよいのでは」という指摘を受けて、ふつうのクラスにした。自分では何も描かず、フェードは中の部品2つに同じ透明度を入れれば済む。
- 逆に、Hud をなくして UIManager が部品を直接持つ形はやめた。UIManager に全キャラクターの読み方が入って太るため。

---

## 2026-11-08: 実装した

- `Game/Source/UI/` に、UIManager・`Hud/`（PlayerHud・BossHud）・`Elements/`（UIElement・Gauge・Icon・Text）を作った。Button はメニュー画面を作るときに回した。
- ゲーム側には、UI が読むための const の getter（`Status::GetHpRate`、`Character` の const の `GetStatus`・`GetStatusEffects`）だけを足した。
- **Boss は作らなかった。** Enemy を継承するか Character から作るかが決まっていないため。そのため `BossHud` は、設計の `const Boss*` ではなく `const Character*` と名前を受け取る形にした。Boss を作るときに直す。
- 画像がないので、4×4 の白い DDS（`Assets/sprite/white.dds`）を作り、色を付けてゲージとアイコンにした。
- 確かめたこと: 一時的に今の敵を `ShowBoss` し、起動して8秒後に敵へ40、Player へ25のダメージを与え、Player を凍らせて画面を撮った。ダメージの前は両方のゲージが満タン、あとは Player 75%・ボス 60% になり、凍結のアイコンが出た。白いバーが遅れて縮むところは撮れていない。
- 撮った画面で、Player の HPゲージが左上の FPS の表示と重なり、ボスの名前の下の端がゲージにかかっていたので、ゲージを上端から 130 に下げ、名前をゲージの 55 上に上げた（直したあとの画面は撮っていない）。
