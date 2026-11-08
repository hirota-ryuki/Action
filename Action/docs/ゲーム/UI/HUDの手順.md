# HUDの手順

作成日: 2026-11-08
設計: [HUDの設計](HUDの設計.md)
経緯: [UIの経緯](UIの経緯.md)
確かめた記録: [ゲームの手順書の経緯](../土台/ゲームの手順書の経緯.md)

HP ゲージの部品 `Gauge` と、Player 用の `PlayerHud`、ボス用の `BossHud` を作るまでの手順です。上から順にやると、[HUDの設計](HUDの設計.md) の形になります。

**出来上がるもの:** 画面下の中央に、Player の細い緑の HP ゲージと「今のHP / 最大HP」の数字が出ます。凍結中・燃焼中は、ゲージの右に水色・橙色の四角が出ます。ボスの HP ゲージ（画面上の中央）は、`UIManager::ShowBoss()` を呼んだときだけ出ます。

**前提:**

- Visual Studio 2022 で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [UIの構成の手順](UIの構成の手順.md) を済ませておく（ゲームの手順書の順番は [README の「ゲームを作る順番」](../../README.md#ゲームを作る順番)）。この手順では、そこで作った `UIManager`・`Text`・`Icon`・`UISprite` を使う。
- 作業を始める前に、main からブランチを切る。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | 横向きのバーの部品 `Gauge` | ビルドが通る |
| 2 | `PlayerHud` で、Player の HP ゲージと数字を出す | 画面下に HP が出る |
| 3 | `PlayerHud` に、状態異常のアイコンを足す | 凍結・燃焼で四角が出る |
| 4 | `BossHud` で、ボスの名前と HP ゲージを出す | `ShowBoss()` で上に出る |

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加 | 1・2・4 |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加 | 1・2・4 |
| [Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) | 新しく作る | 1 |
| [Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) | 新しく作る | 1 |
| [Game.cpp](../../../Game/Source/Core/Game.cpp) | 追加 | 2 |
| [PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) | 新しく作る・追加 | 2・3 |
| [PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) | 新しく作る・追加 | 2・3 |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加 | 2・4 |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加 | 2・4 |
| [BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) | 新しく作る | 4 |
| [BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) | 新しく作る | 4 |

消すファイル・コメントアウトする行はありません。

`Game.vcxproj` と `Game.vcxproj.filters` は、Visual Studio でファイルやフィルターを足すと自動で書き換わります。何が変わるかがわかるように diff を載せています。新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: 横向きのバーの部品 Gauge を作る

HP などの割合（0〜1）を、横向きのバーで出す部品です。下から「枠 → 空き → 遅れて減るバー → 中身」の4枚の画像を重ねて描きます。遅れて減るバーをオンにすると、減った分がしばらく白く残り、あとから追いかけて縮みます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（2行。自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（6行。自動で入る） |
| [Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) | 新しく作る（4枚の画像を重ねて描く。遅れて減るバー） |
| [Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) | 新しく作る（設定の構造体、`SetRate()`・`ResetTrail()`） |

### 1-1. ファイルを作る

`Source\UI\Elements` フィルターに `Gauge.h` と `Gauge.cpp` を作ります。場所は `Action/Game/Source/UI/Elements/`。

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の2行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClCompile Include="Source\Enemy\Enemy.cpp" />
     <ClCompile Include="Source\UI\UIManager.cpp" />
+    <ClCompile Include="Source\UI\Elements\Gauge.cpp" />
     <ClCompile Include="Source\UI\Elements\Icon.cpp" />
     <ClCompile Include="Source\UI\Elements\Text.cpp" />
```

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClInclude Include="Source\UI\Elements\UIElement.h" />
     <ClInclude Include="Source\UI\Elements\UISprite.h" />
+    <ClInclude Include="Source\UI\Elements\Gauge.h" />
     <ClInclude Include="Source\UI\Elements\Icon.h" />
     <ClInclude Include="Source\UI\Elements\Text.h" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、次の6行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI</Filter>
     </ClCompile>
+    <ClCompile Include="Source\UI\Elements\Gauge.cpp">
+      <Filter>Source\UI\Elements</Filter>
+    </ClCompile>
     <ClCompile Include="Source\UI\Elements\Icon.cpp">
       <Filter>Source\UI\Elements</Filter>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI\Elements</Filter>
     </ClInclude>
+    <ClInclude Include="Source\UI\Elements\Gauge.h">
+      <Filter>Source\UI\Elements</Filter>
+    </ClInclude>
     <ClInclude Include="Source\UI\Elements\Icon.h">
       <Filter>Source\UI\Elements</Filter>
```

### 1-2. Gauge.h を書く

見た目の設定は `Settings` にまとめ、`Init()` に1回渡します。

<!-- new: ../../../Game/Source/UI/Elements/Gauge.h -->

```cpp
#pragma once

#include "UIElement.h"
#include "UISprite.h"

// 横向きのバー。SetRate(0〜1) で中身の長さを変える（HP・クールダウンなど）。
// 位置は「バーの左端の中央」（枠は含まない）。中身は左端から右へ伸びる。
// 下から 枠 → 空き → 遅れて減るバー → 中身 の順に重ねて描く。
// 遅れて減るバー（useTrail）をオンにすると、減った分が残り、少し待ってから追いかけて縮む。
class Gauge : public UIElement
{
public:
	struct Settings
	{
		float width = 400.0f;
		float height = 20.0f;
		float borderSize = 0.0f;                            // 枠の太さ。0 なら枠を描かない。
		Vector4 borderColor = { 0.0f, 0.0f, 0.0f, 0.8f };
		Vector4 backColor = { 0.0f, 0.0f, 0.0f, 0.6f };     // 空きの部分。
		Vector4 barColor = { 0.85f, 0.15f, 0.15f, 1.0f };   // 中身。
		const char* barImagePath = UISprite::WHITE_PATH;     // 中身と遅れて減るバーの画像。色はこれに掛ける。
		bool useTrail = false;                               // 遅れて減るバーを使うか。
		Vector4 trailColor = { 1.0f, 1.0f, 1.0f, 0.9f };
		float trailDelay = 0.5f;                             // 減ってから、遅れて減るバーが縮み始めるまでの秒数。
		float trailSpeed = 0.5f;                             // 遅れて減るバーが縮む速さ（1秒にゲージ全体の何割か）。
	};

	void Init(const Settings& settings);
	// 中身の割合。0〜1 に収めてから使う。
	// 増えたときは遅れて減るバーもすぐそろえる（残らないように）。
	void SetRate(float rate);
	// 遅れて減るバーを今の割合にそろえる。表示し始めるときに呼ぶ（減っていく演出にならないように）。
	void ResetTrail();
	float GetRate() const
	{
		return m_rate;
	}
	const Settings& GetSettings() const
	{
		return m_settings;
	}

	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

private:
	// 左端の中央を x の位置に合わせ、横の長さを rate 倍にして置く。
	void UpdateBar(SpriteRender& sprite, float x, float rate, const Vector4& color);

	Settings m_settings;
	SpriteRender m_border;
	SpriteRender m_back;
	SpriteRender m_trail;
	SpriteRender m_bar;
	float m_rate = 1.0f;
	float m_trailRate = 1.0f;
	float m_trailWait = 0.0f;  // 遅れて減るバーが縮み始めるまでの残り秒数。
};
```

### 1-3. Gauge.cpp を書く

<!-- new: ../../../Game/Source/UI/Elements/Gauge.cpp -->

```cpp
#include "stdafx.h"
#include "Gauge.h"

#include "UISprite.h"

namespace
{
	// 左端の中央を基準にする（ピボットの x は 0 が左端、0.5 が中央）。
	const Vector2 LEFT_CENTER_PIVOT = { 0.0f, 0.5f };
}

void Gauge::Init(const Settings& settings)
{
	m_settings = settings;
	const float border = m_settings.borderSize;
	m_border.Init(UISprite::WHITE_PATH, m_settings.width + border * 2.0f, m_settings.height + border * 2.0f);
	m_back.Init(UISprite::WHITE_PATH, m_settings.width, m_settings.height);
	m_trail.Init(m_settings.barImagePath, m_settings.width, m_settings.height);
	m_bar.Init(m_settings.barImagePath, m_settings.width, m_settings.height);
	m_border.SetPivot(LEFT_CENTER_PIVOT);
	m_back.SetPivot(LEFT_CENTER_PIVOT);
	m_trail.SetPivot(LEFT_CENTER_PIVOT);
	m_bar.SetPivot(LEFT_CENTER_PIVOT);
}

void Gauge::SetRate(float rate)
{
	rate = min(1.0f, max(0.0f, rate));
	if (rate < m_rate) {
		// 減った。白いバーは今の位置に残し、少し待ってから縮める。
		m_trailWait = m_settings.trailDelay;
	}
	m_rate = rate;
	if (m_trailRate < m_rate) {
		m_trailRate = m_rate;
	}
}

void Gauge::ResetTrail()
{
	m_trailRate = m_rate;
	m_trailWait = 0.0f;
}

void Gauge::Update(float deltaTime)
{
	if (m_settings.useTrail && m_trailRate > m_rate) {
		if (m_trailWait > 0.0f) {
			m_trailWait -= deltaTime;
		}
		else {
			m_trailRate = max(m_rate, m_trailRate - m_settings.trailSpeed * deltaTime);
		}
	}
	else {
		m_trailRate = m_rate;
	}

	// 枠は、中身より borderSize だけ左から始まり、上下左右に borderSize ずつはみ出す。
	UpdateBar(m_border, m_position.x - m_settings.borderSize, 1.0f, m_settings.borderColor);
	UpdateBar(m_back, m_position.x, 1.0f, m_settings.backColor);
	UpdateBar(m_trail, m_position.x, m_trailRate, m_settings.trailColor);
	UpdateBar(m_bar, m_position.x, m_rate, m_settings.barColor);
}

void Gauge::UpdateBar(SpriteRender& sprite, float x, float rate, const Vector4& color)
{
	sprite.SetPosition(Vector3(x, m_position.y, 0.0f));
	sprite.SetScale(Vector3(rate, 1.0f, 1.0f));
	sprite.SetMulColor(ApplyAlpha(color));
	sprite.Update();
}

void Gauge::OnDraw(RenderContext& rc)
{
	// 後に描いたものが上に重なる。枠 → 空き → 遅れて減るバー → 中身の順。
	if (m_settings.borderSize > 0.0f) {
		m_border.Draw(rc);
	}
	m_back.Draw(rc);
	if (m_settings.useTrail) {
		m_trail.Draw(rc);
	}
	m_bar.Draw(rc);
}
```

- **位置は「バーの左端の中央」。** 画像の pivot を `{ 0, 0.5 }` にして、横の大きさ（`SetScale` の x）を割合にすると、左端から右へ伸び縮みします。
- **減ったときだけ待つ。** `SetRate()` で減ったら `m_trailWait` に待つ秒数を入れ、`Update()` で待ち終わってから遅れて減るバーを縮めます。増えたときは、遅れて減るバーもすぐそろえます。

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: PlayerHud で、Player の HP ゲージと数字を出す

Player の値を読んで部品に入れる係を `PlayerHud` にします。自分では何も描かないので `UIElement` は継承せず、`UIManager` がメンバーとして持ちます。Player は読むだけなので、`const Player*` で受け取ります。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（2行。自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（フィルター1つと、ファイル2つ。自動で入る） |
| [Game.cpp](../../../Game/Source/Core/Game.cpp) | 追加（`SetPlayer()` を呼ぶ） |
| [PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) | 新しく作る（置き場所・色の定数、`Init()`・`Update()`・`Draw()`） |
| [PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) | 新しく作る（HPゲージと数字） |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加（`Init()`・`Update()`・`Draw()` を呼ぶ） |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加（`#include`、`SetPlayer()`、メンバー `m_playerHud`） |

### 2-1. フィルターとファイルを作る

`Source\UI` フィルターの下に `Hud` フィルターを作り、`PlayerHud.h` と `PlayerHud.cpp` を作ります。場所は `Action/Game/Source/UI/Hud/`。

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の2行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClCompile Include="Source\Enemy\Enemy.cpp" />
     <ClCompile Include="Source\UI\UIManager.cpp" />
+    <ClCompile Include="Source\UI\Hud\PlayerHud.cpp" />
     <ClCompile Include="Source\UI\Elements\Gauge.cpp" />
     <ClCompile Include="Source\UI\Elements\Icon.cpp" />
```

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClInclude Include="Source\Enemy\Enemy.h" />
     <ClInclude Include="Source\UI\UIManager.h" />
+    <ClInclude Include="Source\UI\Hud\PlayerHud.h" />
     <ClInclude Include="Source\UI\Elements\UIElement.h" />
     <ClInclude Include="Source\UI\Elements\UISprite.h" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、フィルター1つとファイル2つが**追加**されます。

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <UniqueIdentifier>{6c2e8f14-3a7b-4d59-9e01-8b4f2c7d5a36}</UniqueIdentifier>
     </Filter>
+    <Filter Include="Source\UI\Hud">
+      <UniqueIdentifier>{0f9d4a27-8e3c-4b16-a5d2-71c6e3b8f049}</UniqueIdentifier>
+    </Filter>
     <Filter Include="Source\UI\Elements">
       <UniqueIdentifier>{b83a5e60-2d1f-4c97-8a4e-5f07d9c2e1b3}</UniqueIdentifier>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI</Filter>
     </ClCompile>
+    <ClCompile Include="Source\UI\Hud\PlayerHud.cpp">
+      <Filter>Source\UI\Hud</Filter>
+    </ClCompile>
     <ClCompile Include="Source\UI\Elements\Gauge.cpp">
       <Filter>Source\UI\Elements</Filter>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI</Filter>
     </ClInclude>
+    <ClInclude Include="Source\UI\Hud\PlayerHud.h">
+      <Filter>Source\UI\Hud</Filter>
+    </ClInclude>
     <ClInclude Include="Source\UI\Elements\UIElement.h">
       <Filter>Source\UI\Elements</Filter>
```

### 2-2. PlayerHud.h を書く

<!-- new: ../../../Game/Source/UI/Hud/PlayerHud.h -->

```cpp
#pragma once

#include "Gauge.h"
#include "Text.h"

class Player;

// Player 用の HUD。Player の値を読んで、自分の部品に入れる。
// 画面下の中央に、細い緑の HPゲージと、その下に「今のHP / 最大HP」の数字を出す。状態異常のアイコンはゲージの右。
// UIElement は継承しない（自分では描かず、部品を持つだけ）。UIManager が持つ。
class PlayerHud
{
public:
	void Init();
	// 表示する Player。読むだけ（const）。nullptr なら何も描かない。
	void SetTarget(const Player* player)
	{
		m_player = player;
	}
	void Update(float deltaTime);
	void Draw(RenderContext& rc);

private:
	const Player* m_player = nullptr;
	Gauge m_hp;
	Text m_hpNumber;  // 「今のHP / 最大HP」。
};
```

### 2-3. PlayerHud.cpp を書く

<!-- new: ../../../Game/Source/UI/Hud/PlayerHud.cpp -->

```cpp
#include "stdafx.h"
#include "PlayerHud.h"

#include "Player.h"
#include "UISprite.h"

namespace
{
	// HPゲージ。画面下の中央に、細く置く。
	constexpr float HP_WIDTH = 360.0f;
	constexpr float HP_HEIGHT = 10.0f;
	constexpr float HP_BORDER = 2.0f;
	constexpr float HP_BOTTOM = 80.0f;  // 画面の下端から、ゲージの中央までの高さ。
	// HPゲージの左端の中央。
	const Vector2 HP_POSITION = { -HP_WIDTH * 0.5f, -static_cast<float>(UI_SPACE_HEIGHT) * 0.5f + HP_BOTTOM };
	const Vector4 HP_COLOR = { 0.55f, 0.88f, 0.35f, 1.0f };      // 明るい緑。
	const Vector4 HP_BACK_COLOR = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 HP_BORDER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 HP_TRAIL_COLOR = { 1.0f, 1.0f, 1.0f, 0.75f };

	// HP の数字。ゲージの下に小さく。
	constexpr float NUMBER_SCALE = 0.42f;
	constexpr float NUMBER_GAP = 18.0f;  // ゲージの中央から、数字の中央までの距離。
	const Vector4 NUMBER_COLOR = { 1.0f, 1.0f, 1.0f, 0.95f };
	const Vector4 NUMBER_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float NUMBER_SHADOW_OFFSET = 1.5f;
}

void PlayerHud::Init()
{
	Gauge::Settings hp;
	hp.width = HP_WIDTH;
	hp.height = HP_HEIGHT;
	hp.borderSize = HP_BORDER;
	hp.borderColor = HP_BORDER_COLOR;
	hp.backColor = HP_BACK_COLOR;
	hp.barColor = HP_COLOR;
	hp.barImagePath = UISprite::GAUGE_PATH;
	hp.useTrail = true;
	hp.trailColor = HP_TRAIL_COLOR;
	m_hp.Init(hp);
	m_hp.SetPosition(HP_POSITION);

	m_hpNumber.SetPosition({ 0.0f, HP_POSITION.y - NUMBER_GAP });
	m_hpNumber.SetScale(NUMBER_SCALE);
	m_hpNumber.SetColor(NUMBER_COLOR);
	m_hpNumber.SetShadow(NUMBER_SHADOW_OFFSET, NUMBER_SHADOW_COLOR);
}

void PlayerHud::Update(float deltaTime)
{
	if (m_player == nullptr) {
		return;
	}
	const PlayerStatus& status = m_player->GetStatus();
	m_hp.SetRate(status.GetHpRate());

	// 小数は切り上げる（0.3 残っているのに 0 と出さないように）。
	wchar_t number[64];
	swprintf_s(number, L"%d / %d",
		static_cast<int>(ceilf(status.GetHp())),
		static_cast<int>(ceilf(status.GetMaxHp())));
	m_hpNumber.SetText(number);

	m_hp.Update(deltaTime);
	m_hpNumber.Update(deltaTime);
}

void PlayerHud::Draw(RenderContext& rc)
{
	if (m_player == nullptr) {
		return;
	}
	m_hp.Draw(rc);
	m_hpNumber.Draw(rc);
}
```

- **HP は `Status::GetHpRate()`（0〜1）をそのまま入れる。** ゲージは割合だけを知っていればよく、最大 HP が変わっても同じ部品で出せます。
- **数字は切り上げて出す。** 0.3 残っているのに 0 と出さないためです。

### 2-4. UIManager に持たせる

[UIManager.h](../../../Game/Source/UI/UIManager.h) に、`#include`、Player を渡す `SetPlayer()`、メンバー `m_playerHud` を**追加**します。

<!-- file: ../../../Game/Source/UI/UIManager.h -->

```diff
 #pragma once
 
+#include "PlayerHud.h"
+
+class Player;
+
 // 画面の UI をまとめて持ち、毎フレーム更新して描く。ゲームに1つだけ。
 // UI はゲームを読むだけで、ゲーム側は UI のことを知らない。UIManager に話しかけるのは Game だけにする
```

<!-- file: ../../../Game/Source/UI/UIManager.h -->

```diff
 	void Update() override;
 	void Render(RenderContext& rc) override;
+
+	void SetPlayer(const Player* player)
+	{
+		m_playerHud.SetTarget(player);
+	}
+
+private:
+	PlayerHud m_playerHud;
 };
```

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の `Start()`・`Update()`・`Render()` から、`m_playerHud` の `Init()`・`Update()`・`Draw()` を呼ぶ行を**追加**します。

<!-- file: ../../../Game/Source/UI/UIManager.cpp -->

```diff
 bool UIManager::Start()
 {
+	// Init は部品の画像を読むだけで、SetPlayer / ShowBoss で入れた値は消さない。
+	m_playerHud.Init();
 	return true;
 }
```

<!-- file: ../../../Game/Source/UI/UIManager.cpp -->

```diff
 {
 	const float deltaTime = g_gameTime->GetFrameDeltaTime();
+	m_playerHud.Update(deltaTime);
 }
 
```

<!-- file: ../../../Game/Source/UI/UIManager.cpp -->

```diff
 {
 	// 後に描いたものが上に重なる。
+	m_playerHud.Draw(rc);
 }
```

### 2-5. Game から Player を渡す

[Game.cpp](../../../Game/Source/Core/Game.cpp) の `Game::Start()` で、`UIManager` を作った行の下に**追加**します。

<!-- file: ../../../Game/Source/Core/Game.cpp -->

```diff
 	m_fireEffect->SetScale(FIRE_SCALE);
 	m_uiManager = NewGO<UIManager>(3, "uiManager");
+	m_uiManager->SetPlayer(m_player);
 
 	// SkyCubeのテクスチャをIBLの環境マップとしても使い回す。
```

**確かめ方:** ゲームを起動すると、画面下の中央に緑の細いゲージと「100 / 100」が出る。

---

## 手順3: PlayerHud に、状態異常のアイコンを足す

凍結中・燃焼中がわかるように、ゲージの右にアイコンを2つ並べます。画像ができるまでは、白い画像に色を掛けた四角（凍結は水色、燃焼は橙色）にします。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) | 追加（アイコンの定数、`Init()`・`Update()`・`Draw()` にアイコン） |
| [PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) | 追加（`#include "Icon.h"` と、アイコン2つ） |

[PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) に、`Icon.h` の `#include` と、アイコン2つを**追加**します。

<!-- file: ../../../Game/Source/UI/Hud/PlayerHud.h -->

```diff
 
 #include "Gauge.h"
+#include "Icon.h"
 #include "Text.h"
 
```

<!-- file: ../../../Game/Source/UI/Hud/PlayerHud.h -->

```diff
 	Gauge m_hp;
 	Text m_hpNumber;  // 「今のHP / 最大HP」。
+	Icon m_frozen;    // 凍結中だけ出す。
+	Icon m_burning;   // 燃焼中だけ出す。
 };
```

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) に、アイコンの大きさ・色の定数と、`Init()`（置き場所）・`Update()`（出す／出さない）・`Draw()` の行を**追加**します。

<!-- file: ../../../Game/Source/UI/Hud/PlayerHud.cpp -->

```diff
 	const Vector4 NUMBER_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
 	constexpr float NUMBER_SHADOW_OFFSET = 1.5f;
+
+	// 状態異常のアイコン（画像ができるまでは色付きの四角）。ゲージの右に並べる。
+	constexpr float ICON_SIZE = 18.0f;
+	constexpr float ICON_GAP = 10.0f;
+	const Vector4 FROZEN_COLOR = { 0.4f, 0.85f, 1.0f, 1.0f };
+	const Vector4 BURNING_COLOR = { 1.0f, 0.45f, 0.1f, 1.0f };
 }
 
```

<!-- file: ../../../Game/Source/UI/Hud/PlayerHud.cpp -->

```diff
 	m_hpNumber.SetColor(NUMBER_COLOR);
 	m_hpNumber.SetShadow(NUMBER_SHADOW_OFFSET, NUMBER_SHADOW_COLOR);
+
+	const float iconY = HP_POSITION.y;
+	const float frozenX = HP_POSITION.x + HP_WIDTH + HP_BORDER + ICON_GAP + ICON_SIZE * 0.5f;
+	const float burningX = frozenX + ICON_SIZE + ICON_GAP;
+
+	m_frozen.Init(UISprite::WHITE_PATH, ICON_SIZE, ICON_SIZE);
+	m_frozen.SetColor(FROZEN_COLOR);
+	m_frozen.SetPosition({ frozenX, iconY });
+
+	m_burning.Init(UISprite::WHITE_PATH, ICON_SIZE, ICON_SIZE);
+	m_burning.SetColor(BURNING_COLOR);
+	m_burning.SetPosition({ burningX, iconY });
 }
 
```

<!-- file: ../../../Game/Source/UI/Hud/PlayerHud.cpp -->

```diff
 		static_cast<int>(ceilf(status.GetMaxHp())));
 	m_hpNumber.SetText(number);
+
+	const StatusEffects& effects = m_player->GetStatusEffects();
+	m_frozen.SetVisible(effects.IsFrozen());
+	m_burning.SetVisible(effects.IsBurning());
 
 	m_hp.Update(deltaTime);
 	m_hpNumber.Update(deltaTime);
+	m_frozen.Update(deltaTime);
+	m_burning.Update(deltaTime);
 }
 
```

<!-- file: ../../../Game/Source/UI/Hud/PlayerHud.cpp -->

```diff
 	m_hp.Draw(rc);
 	m_hpNumber.Draw(rc);
+	m_frozen.Draw(rc);
+	m_burning.Draw(rc);
 }
```

出す／出さないは、`StatusEffects::IsFrozen()`・`IsBurning()`（[状態異常の手順](../キャラクター/状態異常の手順.md) で作ったもの）をそのまま `SetVisible()` に入れるだけです。

**確かめ方:** 敵の氷魔法に当たると水色、燃える床に乗ると橙色の四角が、ゲージの右に出る。

---

## 手順4: BossHud で、ボスの名前と HP ゲージを出す

ボス用の HUD を足します。画面上の中央に名前と赤い HP ゲージを出し、`Show()` でフェードイン、`Hide()` でフェードアウトします。Boss クラスはまだないので、`Character` と名前を受け取ります。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（2行。自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（6行。自動で入る） |
| [BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) | 新しく作る（置き場所・色の定数、フェード） |
| [BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) | 新しく作る（`Show()`・`Hide()` と、名前・HPゲージ・透明度） |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加（`Init()`・`Update()`・`Draw()` を呼ぶ） |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加（`#include`、`ShowBoss()`・`HideBoss()`、メンバー `m_bossHud`） |

### 4-1. ファイルを作る

`Source\UI\Hud` フィルターに `BossHud.h` と `BossHud.cpp` を作ります。

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の2行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClCompile Include="Source\UI\UIManager.cpp" />
     <ClCompile Include="Source\UI\Hud\PlayerHud.cpp" />
+    <ClCompile Include="Source\UI\Hud\BossHud.cpp" />
     <ClCompile Include="Source\UI\Elements\Gauge.cpp" />
     <ClCompile Include="Source\UI\Elements\Icon.cpp" />
```

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClInclude Include="Source\UI\UIManager.h" />
     <ClInclude Include="Source\UI\Hud\PlayerHud.h" />
+    <ClInclude Include="Source\UI\Hud\BossHud.h" />
     <ClInclude Include="Source\UI\Elements\UIElement.h" />
     <ClInclude Include="Source\UI\Elements\UISprite.h" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、次の6行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI\Hud</Filter>
     </ClCompile>
+    <ClCompile Include="Source\UI\Hud\BossHud.cpp">
+      <Filter>Source\UI\Hud</Filter>
+    </ClCompile>
     <ClCompile Include="Source\UI\Elements\Gauge.cpp">
       <Filter>Source\UI\Elements</Filter>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI\Hud</Filter>
     </ClInclude>
+    <ClInclude Include="Source\UI\Hud\BossHud.h">
+      <Filter>Source\UI\Hud</Filter>
+    </ClInclude>
     <ClInclude Include="Source\UI\Elements\UIElement.h">
       <Filter>Source\UI\Elements</Filter>
```

### 4-2. BossHud.h を書く

<!-- new: ../../../Game/Source/UI/Hud/BossHud.h -->

```cpp
#pragma once

#include "Gauge.h"
#include "Text.h"

class Character;

// ボス用の HUD。画面上の中央に、名前とHPゲージ（遅れて減るバー付き）を出す。
// Show() でフェードインし、Hide() でフェードアウトする。UIElement は継承しない。UIManager が持つ。
// Boss クラスはまだないので、今は Character と名前を受け取る。Boss ができたら const Boss* に変える。
class BossHud
{
public:
	void Init();
	// 表示を始める。ゲージの白いバーを今のHPにそろえる。
	void Show(const Character* boss, const wchar_t* name);
	// 表示をやめる。ボスへのポインタはすぐ捨てる（このあと DeleteGO してよい）。
	// フェードアウトが終わるまでは、最後の値のまま描く。
	void Hide();
	bool IsShown() const
	{
		return m_boss != nullptr;
	}
	void Update(float deltaTime);
	void Draw(RenderContext& rc);

private:
	void SetAlpha(float alpha);

	const Character* m_boss = nullptr;
	Text m_name;
	Gauge m_hp;
	float m_alpha = 0.0f;        // 今の透明度。
	float m_targetAlpha = 0.0f;  // Show で 1、Hide で 0。
};
```

### 4-3. BossHud.cpp を書く

<!-- new: ../../../Game/Source/UI/Hud/BossHud.cpp -->

```cpp
#include "stdafx.h"
#include "BossHud.h"

#include "Character.h"
#include "UISprite.h"

namespace
{
	// HPゲージ。画面上の中央に、横長に置く（Player のゲージは画面下なので重ならない）。
	constexpr float HP_WIDTH = 720.0f;
	constexpr float HP_HEIGHT = 12.0f;
	constexpr float HP_BORDER = 2.0f;
	constexpr float HP_TOP = 100.0f;  // 画面の上端から、ゲージの中央までの距離。
	// HPゲージの左端の中央。
	const Vector2 HP_POSITION = { -HP_WIDTH * 0.5f, static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - HP_TOP };
	const Vector4 HP_COLOR = { 0.9f, 0.28f, 0.22f, 1.0f };      // 赤。
	const Vector4 HP_BACK_COLOR = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 HP_BORDER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 HP_TRAIL_COLOR = { 1.0f, 0.92f, 0.8f, 0.8f };  // 少し暖かい白。

	// 名前は HPゲージの上に小さく。
	constexpr float NAME_SCALE = 0.55f;
	constexpr float NAME_GAP = 24.0f;  // ゲージの中央から、名前の中央までの距離。
	const Vector2 NAME_POSITION = { 0.0f, HP_POSITION.y + NAME_GAP };
	const Vector4 NAME_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float NAME_SHADOW_OFFSET = 1.5f;

	constexpr float TRAIL_DELAY = 0.5f;  // 減ってから、遅れて減るバーが縮み始めるまでの秒数。
	constexpr float TRAIL_SPEED = 0.5f;  // 1秒にゲージ全体の 50% 縮む。
	constexpr float FADE_SECONDS = 0.3f; // フェードイン・アウトにかける秒数。
}

void BossHud::Init()
{
	Gauge::Settings hp;
	hp.width = HP_WIDTH;
	hp.height = HP_HEIGHT;
	hp.borderSize = HP_BORDER;
	hp.borderColor = HP_BORDER_COLOR;
	hp.backColor = HP_BACK_COLOR;
	hp.barColor = HP_COLOR;
	hp.barImagePath = UISprite::GAUGE_PATH;
	hp.useTrail = true;
	hp.trailColor = HP_TRAIL_COLOR;
	hp.trailDelay = TRAIL_DELAY;
	hp.trailSpeed = TRAIL_SPEED;
	m_hp.Init(hp);
	m_hp.SetPosition(HP_POSITION);

	m_name.SetPosition(NAME_POSITION);
	m_name.SetScale(NAME_SCALE);
	m_name.SetShadow(NAME_SHADOW_OFFSET, NAME_SHADOW_COLOR);

	SetAlpha(0.0f);
}

void BossHud::Show(const Character* boss, const wchar_t* name)
{
	m_boss = boss;
	m_name.SetText(name);
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}
	// 戦い始めに白いバーが減っていく演出にならないよう、今のHPにそろえる。
	m_hp.ResetTrail();
	m_targetAlpha = 1.0f;
}

void BossHud::Hide()
{
	m_boss = nullptr;
	m_targetAlpha = 0.0f;
}

void BossHud::Update(float deltaTime)
{
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}

	// 目標の透明度へ、一定の速さで近づける。
	const float step = deltaTime / FADE_SECONDS;
	if (m_alpha < m_targetAlpha) {
		SetAlpha(min(m_targetAlpha, m_alpha + step));
	}
	else if (m_alpha > m_targetAlpha) {
		SetAlpha(max(m_targetAlpha, m_alpha - step));
	}

	m_name.Update(deltaTime);
	m_hp.Update(deltaTime);
}

void BossHud::Draw(RenderContext& rc)
{
	m_name.Draw(rc);
	m_hp.Draw(rc);
}

void BossHud::SetAlpha(float alpha)
{
	m_alpha = alpha;
	// 名前とゲージに同じ透明度を入れる。0 のときは部品の側で描かない。
	m_name.SetAlpha(alpha);
	m_hp.SetAlpha(alpha);
}
```

- **`Hide()` ではポインタをすぐ捨てる。** そのあとすぐ `DeleteGO` されてもよいように、フェードアウトの間は最後の値のまま描きます。
- **`Show()` で遅れて減るバーをそろえる。** 戦い始めに白いバーが減っていく演出にならないためです。

### 4-4. UIManager に持たせる

[UIManager.h](../../../Game/Source/UI/UIManager.h) に、`#include`、`ShowBoss()`・`HideBoss()`、メンバー `m_bossHud` を**追加**します。

<!-- file: ../../../Game/Source/UI/UIManager.h -->

```diff
 
 #include "PlayerHud.h"
+#include "BossHud.h"
 
 class Player;
+class Character;
 
 // 画面の UI をまとめて持ち、毎フレーム更新して描く。ゲームに1つだけ。
```

<!-- file: ../../../Game/Source/UI/UIManager.h -->

```diff
 		m_playerHud.SetTarget(player);
 	}
+	// ボスの HPバーを出す。ボスを DeleteGO する前に、必ず HideBoss() を呼ぶ。
+	void ShowBoss(const Character* boss, const wchar_t* name)
+	{
+		m_bossHud.Show(boss, name);
+	}
+	void HideBoss()
+	{
+		m_bossHud.Hide();
+	}
 
 private:
 	PlayerHud m_playerHud;
+	BossHud m_bossHud;
 };
```

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) に、`m_bossHud` の `Init()`・`Update()`・`Draw()` を呼ぶ行を**追加**します。

<!-- file: ../../../Game/Source/UI/UIManager.cpp -->

```diff
 	// Init は部品の画像を読むだけで、SetPlayer / ShowBoss で入れた値は消さない。
 	m_playerHud.Init();
+	m_bossHud.Init();
 	return true;
 }
```

<!-- file: ../../../Game/Source/UI/UIManager.cpp -->

```diff
 	const float deltaTime = g_gameTime->GetFrameDeltaTime();
 	m_playerHud.Update(deltaTime);
+	m_bossHud.Update(deltaTime);
 }
 
```

<!-- file: ../../../Game/Source/UI/UIManager.cpp -->

```diff
 	// 後に描いたものが上に重なる。
 	m_playerHud.Draw(rc);
+	m_bossHud.Draw(rc);
 }
```

**確かめ方（これで出来上がり）:**

- ソリューションのビルドが通る。
- `Game::Start()` の最後に、仮に `m_uiManager->ShowBoss(m_enemy, L"ボス");` と書いて起動すると、画面上の中央に「ボス」と赤いゲージがふわっと出る。確かめたら、仮に書いた行は消す。
