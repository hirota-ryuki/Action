# 黒騎士の Meshy プロンプト

敵の黒騎士を Meshy で作るときに貼るプロンプトと設定の最終形です。
体と剣は別々に作ります。2026-11-10 に Meshy Agent（Meshy 7.1）で作ったときの形です。

---

## 1. デザインの決まり

| 番号 | 項目 | 決まり |
|---|---|---|
| 1 | 方向 | アニメ調のファンタジー RPG に出てくる、厳かで気品のある黒騎士。素早い暗殺者型（重戦車型ではない） |
| 2 | 絵の感じ | 原神のような、手描き風のきれいな塗りのゲームのキャラクター。ただし既存の作品のキャラクターはまねしない（プロンプトにゲームの名前を入れない） |
| 3 | 体型 | 7.5頭身。頭は小さく、手足は長い。華奢すぎず、肩幅と胸板がある |
| 4 | 装甲 | 薄い板を重ね、縁は流れる曲線。関節まわりは薄く。鎧と布を重ねる（高い襟・腰の帯・房飾り） |
| 5 | 兜 | 顔の見えないフルフェイスで、細い目のすき間。**フードは付けない** |
| 6 | マント | 肩と襟の後ろから下げ、**ふくらはぎより少し下（足首の少し上）**まで。裾は2〜3枚に分かれる。T ポーズで腕にくっつけない |
| 7 | 腰 | 2〜3枚に分かれた長い垂れ布 |
| 8 | 装飾 | 金の曲線の紋様、細かい彫りと鋲、胸の真ん中に紫に光る小さな宝石 |
| 9 | 色 | 本体 `#1a1a22` 前後の黒、縁はくすんだ金、光るラインは `#8a5cff` 前後の紫。明るい色は使わない |
| 10 | 避けるもの | トゲ、角、丸み、大きな頭、明るい色、おもちゃっぽさ、ゴツい鎧 |
| 11 | 剣 | 片手で持つ細身の剣。ゆるく反り、刀身は約 60 cm。紫の発光ラインを1本。柄は黒とくすんだ金 |

---

## 2. 体のプロンプト

### 2-1. Meshy Agent に最初に送る文

Meshy Agent のチャットの欄に貼って送ります。Agent はまず設定画（下絵）を1枚作り、その絵から立体にします。

```
Create an original 3D character: an anime-style fantasy RPG dark knight, solemn and noble, an agile assassin type (not a heavy tank), in a polished stylized anime game art style.

Shape:
- Realistic athletic proportions, 7.5 heads tall, small head, long limbs, broad shoulders, firm chest, a sturdy build but still lean and agile.
- T-pose: arms straight out horizontally, palms down, feet shoulder-width apart, empty hands (no weapon).
- Fitted armor made of thin layered plates with flowing curved edges, mixed with layered fabric: high collar, cloth sash at the waist, small decorative tassels and ribbon-like cloth details. Thin armor at the joints.
- Moderate shoulder guards that stick out a little, solid gauntlets and greaves.
- Full-face sleek sharp helmet with a narrow visor slit. No hood.
- Two or three long split cloth panels hanging from the waist.
- A cape attached at the back of the shoulders and collar, hanging straight down behind the body to just below the calves, slightly above the ankles. The cape must not touch the arms in the T-pose. The hem is split into two or three panels.

Texture:
- Clean, crisp shapes, smooth hand-painted textures, soft cel-shaded look, elegant and refined rather than gritty.
- Near-black armor (#1a1a22) with dull antique gold filigree trim with clean flowing lines.
- Glowing violet (#8a5cff) accent lines on chest, shoulders and gauntlets, and a small glowing violet gemstone in the center of the chest piece.
- Fine engraving, small rivets. Dark charcoal cloth and cape with gold trim along the edges. No bright colors.

Avoid: chibi, big head, rounded shapes, toy-like look, bright colors, spikes, horns, bulky armor, hood, holding a sword.

Settings: symmetric, T-pose, about 35,000 polygons, PBR textures, 2048 texture resolution. No rigging, no animation.
```

### 2-2. 設定画のプロンプト（「画像生成プロンプトを編集」の欄）

「テキストから画像」の確認画面で「画像生成プロンプトを編集」を開き、中の文をこれに差し替えてから「承認」を押します。

```
Single full body character, front view only, polished stylized anime game art style, solemn and noble agile assassin dark knight. Clean crisp shapes, readable silhouette, smooth hand-painted textures, soft cel-shaded look, elegant and refined. Realistic athletic proportions, 7.5 heads tall, small head, long limbs, broad shoulders, firm chest, slightly thicker arms and legs, a sturdy masculine build but still lean and agile, not bulky. Strict T-pose, front view, arms stretched straight out horizontally, palms facing down, feet shoulder-width apart, empty hands, completely unarmed, perfectly symmetrical. Sleek, sharp fitted dark armor made of thin layered plates with flowing curved edges, mixed with layered fabric: high collar, cloth sash at the waist, small decorative tassels and ribbon-like cloth details, articulated flexible joints for acrobatic agility. Moderate shoulder guards that stick out a little, solid gauntlets and greaves. Full-face sleek sharp helmet with a narrow horizontal visor slit, no hood, helmet shape fully visible. Two to three long split dark charcoal cloth tassets hanging from the waist. A dark charcoal cape attached at the back of the shoulders and collar, hanging straight down behind the body to just below the calves, slightly above the ankles, gold trim along the edges, hem split into two or three panels, a clear gap between the arms and the cape, a little of the cape's edge visible on both sides of the body. Color palette: near-black charcoal metal armor (#1a1a22) with dull antique gold filigree trim with clean flowing lines along the plate edges, curved engraved scrollwork on chestplate, pauldrons, and gauntlets, with glowing violet (#8a5cff) lines within the engravings, a small glowing violet gemstone in the center of the chest piece. Semi-matte subdued metallic finish, dark muted atmosphere, clean solid light grey background, full body centered view, high detail, fine engraving, small rivets, no hood, no weapons, no spikes, no horns, no bulky armor, no chibi.
```

### 2-3. 設定画を直すときの文

設定画が出たあと「How would you like to proceed」で **Request changes** を選んで送信し、チャットの欄に貼ります。

| 番号 | 気になるところ | 送る文 |
|---|---|---|
| 1 | 剣などを持っている | `Remove the weapon. Both hands must be empty.` |
| 2 | 腕が下がっている | `Arms must be straight out horizontally, palms down.` |
| 3 | 頭が大きい | `Make the head smaller, 7.5 heads tall.` |
| 4 | 華奢すぎる | `Make the body a bit sturdier: broader shoulders, thicker chest, slightly thicker arms and legs. Still agile, not bulky.` |
| 5 | フードが付いている | `Remove the hood completely. Show the full helmet shape.` |
| 6 | マントが腕にくっついている | `The cape must not touch the arms. Keep a clear gap between the arms and the cape.` |
| 7 | トゲや角がある | `Remove all spikes and horns.` |
| 8 | 色が明るい | `Make it darker overall, near-black with dull gold trim only.` |

---

## 3. 剣のプロンプト

体とは別の会話で送ります。

```
Create a 3D object: no character, sword only, isolated object, in a polished stylized anime game art style.

Shape:
- Slender one-handed sword with a gently curved blade, blade about 60 cm long.
- Black grip, guard and pommel with dull antique gold accents and fine curved engravings.

Texture:
- Dark steel blade with one thin glowing violet (#8a5cff) line along the blade.
- Black grip, dull antique gold guard and pommel with fine engraving.
- Smooth hand-painted textures, semi-matte, no bright colors.

Avoid: spikes, horns, a character or hand holding it, bright colors, toy-like look.

Settings: symmetric, about 3,000 polygons, PBR textures, 2048 texture resolution. No rigging, no animation.
```

---

## 4. 画像から 3D にするときの設定（「画像から3Dへ」の確認画面）

| 番号 | 項目 | 値 | 理由 |
|---|---|---|---|
| 1 | 元画像 | 最新の設定画 | 古い絵のときは拒否して `Use the latest concept image.` と頼む |
| 2 | テクスチャモデル | デフォルト：PBR対応 | 色・メタリック・ラフネスのテクスチャが作られる |
| 3 | モデル | Meshy 7.1（いちばん新しいもの） | |
| 4 | 解像度 | 標準 | あとでポリゴン数を減らすので、「高」にしなくてよい |
| 5 | テクスチャを生成 | オン | |
| 6 | テクスチャサイズ | 2048 | |

費用は 30 クレジットでした。押すのは「承認」で、「承認し、今後確認しない」は押しません（クレジットを使う確認が出なくなるため）。

---

## 5. ポリゴン数を合わせる

「画像から3Dへ」にはポリゴン数の欄がないので、できたあとに Agent へ送ります。剣は `35,000` を `3,000` にします。

```
Please remesh this model to about 35,000 polygons (triangles), keeping the textures. No rigging, no animation.
```

合計は体 35,000 + 剣 3,000 = 38,000 で、主人公（VRoid）の 41,507 以内に収めます。

---

## 6. 書き出す

| 番号 | 項目 | 値 |
|---|---|---|
| 1 | 形式 | FBX（GLB でもよい） |
| 2 | テクスチャ | 含める |
| 3 | 骨付け（Rig）・アニメーション（Animate） | 使わない。骨は SK_Player の骨格に合わせて付ける |

---

## 7. 気をつけること

1. Meshy では、メタリック・ラフネスを数値で指定できない。言葉で寄せるだけなので、数値は取り込んだあとにマテリアルで合わせる。
2. 色コード（`#1a1a22` など）はそのまま効くとは限らない。色がずれたら、色コードを消して色の名前だけで試す。
3. 正面の設定画からは、背中のマントの形が見えない。3D にしたら必ず後ろから回して見る。
4. SK_Player の骨格にはマント用の骨がない。揺らしたいときは、マント用の骨を足す。
