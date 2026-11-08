# NPCのAIの手順

作成日: 2026-11-08
設計: [NPCのAIの設計](NPCのAIの設計.md)
経緯: [入力の経緯](入力の経緯.md)
確かめた記録: [ゲームの手順書の経緯](../土台/ゲームの手順書の経緯.md)

NPC の AI（`NpcInputSource`）を作り、Enemy の入力元にするまでの手順です。AI は「どうしたいか」だけを決めて `CharacterInput` に入れ、実際の移動や発射はキャラクター側が行います。

**出来上がるもの:** Enemy が、敵対する相手を探して近づき、射程に入ったら撃つ入力を作ります。今の Enemy の設定は、探す範囲を 0 にして止めてあります（動かすときは 900 に戻す）。

**前提:**

- Visual Studio 2022 で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [入力の手順](入力の手順.md) を済ませておく（順番は [README の「ゲームを作る順番」](../../README.md#ゲームを作る順番)）。相手は [陣営と当たり判定の手順](陣営と当たり判定の手順.md) で作った `HitTargetRegistry` から探す。
- 作業を始める前に、main からブランチを切る。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | NPC の AI `NpcInputSource` | ビルドが通る |
| 2 | Enemy の入力元を AI にする | ビルドが通る |

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加 | 1 |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加 | 1 |
| [NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) | 新しく作る | 1 |
| [NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) | 新しく作る | 1 |
| [Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) | 追加 | 2 |
| [Enemy.h](../../../Game/Source/Enemy/Enemy.h) | 追加・削除・書き換え | 2 |

手順2で、[入力の手順](入力の手順.md) で Enemy に足した仮の入力元 `EmptyInputSource` を**削除**します。コメントアウトする行はありません。

---

## 手順1: NPC の AI NpcInputSource を作る

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（2行。自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（6行。自動で入る） |
| [NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) | 新しく作る（敵対する相手を探して近づき、射程に入ったら撃つ入力を作る） |
| [NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) | 新しく作る（探す範囲・止まる距離・撃つ間隔などの設定） |

`Source\Character\Input` フィルターに `NpcInputSource.h` と `NpcInputSource.cpp` を作ります。

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClCompile Include="Source\Enemy\Enemy.cpp" />
     <ClCompile Include="Source\Character\Input\PlayerInputSource.cpp" />
+    <ClCompile Include="Source\Character\Input\NpcInputSource.cpp" />
     <ClCompile Include="Source\Character\Components\HitTarget.cpp" />
     <ClCompile Include="Source\Core\Game.cpp" />
```

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClInclude Include="Source\Enemy\Enemy.h" />
     <ClInclude Include="Source\Character\Input\PlayerInputSource.h" />
+    <ClInclude Include="Source\Character\Input\NpcInputSource.h" />
     <ClInclude Include="Source\Character\Components\HitTarget.h" />
     <ClInclude Include="Source\Character\Faction.h" />
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\Character\Input</Filter>
     </ClCompile>
+    <ClCompile Include="Source\Character\Input\NpcInputSource.cpp">
+      <Filter>Source\Character\Input</Filter>
+    </ClCompile>
     <ClCompile Include="Source\Character\Components\HitTarget.cpp">
       <Filter>Source\Character\Components</Filter>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\Character\Input</Filter>
     </ClInclude>
+    <ClInclude Include="Source\Character\Input\NpcInputSource.h">
+      <Filter>Source\Character\Input</Filter>
+    </ClInclude>
     <ClInclude Include="Source\Character\Components\HitTarget.h">
       <Filter>Source\Character\Components</Filter>
```

<!-- new: ../../../Game/Source/Character/Input/NpcInputSource.h -->

```cpp
#pragma once

#include "CharacterInput.h"

// NPCのAI。敵対する相手を探して近づき、射程に入ったら魔法を撃つ入力を作る。
// AIは「どうしたいか」だけを決め、実際の移動や発射はキャラクター側が行う。
class NpcInputSource : public IInputSource
{
public:
	struct Settings
	{
		float searchRange = 900.0f;    // この距離以内の敵対する相手を狙う。
		float stopDistance = 90.0f;    // この距離まで近づいたら止まる。
		float moveRate = 1.0f;         // 移動入力の大きさ（0〜1）。
		bool canFire = true;           // 魔法を撃つか。
		float fireRange = 700.0f;      // この距離以内なら撃つ。
		float fireInterval = 4.0f;     // 撃つ間隔（秒）。
		float firstFireDelay = 1.5f;   // 相手を見つけてから最初に撃つまでの秒数。
		float fireHoldSeconds = 0.0f;  // 発射ボタンを押し続ける秒数。長押しの魔法用。0なら1フレームだけ押す。
		float castHeight = 60.0f;      // 足元からこの高さで撃つ。
	};

	NpcInputSource() = default;
	explicit NpcInputSource(const Settings& settings);
	void SetSettings(const Settings& settings)
	{
		m_settings = settings;
	}
	void Update(const InputSourceContext& context, CharacterInput& input) override;

private:
	Settings m_settings;
	float m_fireTimer = 0.0f;      // 次に撃てるまでの秒数。
	float m_fireHoldTimer = 0.0f;  // 発射ボタンを押し続ける残り秒数。
	bool m_hasTarget = false;      // 前フレームに相手を見つけていたか。
	bool m_isFiring = false;       // 発射ボタンを押している最中か。
};
```

<!-- new: ../../../Game/Source/Character/Input/NpcInputSource.cpp -->

```cpp
#include "stdafx.h"
#include "NpcInputSource.h"
#include "HitTarget.h"

NpcInputSource::NpcInputSource(const Settings& settings)
	: m_settings(settings)
{
}

void NpcInputSource::Update(const InputSourceContext& context, CharacterInput& input)
{
	input.moveDirection = Vector3::Zero;
	input.lookDirection = Vector3::Zero;
	input.isRun = false;
	input.jump.Update(false);
	input.dash.Update(false);
	input.magicSwitch = 0;
	input.hasAim = false;

	const HitTarget* target = HitTargetRegistry::FindNearestHostile(
		context.position, m_settings.searchRange, context.faction, context.self);

	if (target == nullptr) {
		m_hasTarget = false;
		m_isFiring = false;
		input.fire.Update(false);
		return;
	}
	if (!m_hasTarget) {
		// 見つけた直後にいきなり撃たないよう、少し待つ。
		m_fireTimer = m_settings.firstFireDelay;
		m_hasTarget = true;
	}

	// 高低差を無視して水平方向に追う。
	Vector3 toTarget = target->GetCenter() - context.position;
	toTarget.y = 0.0f;
	const float distanceSq = toTarget.LengthSq();
	if (distanceSq > FLT_EPSILON) {
		Vector3 direction = toTarget;
		direction.Normalize();
		input.lookDirection = direction;
		if (distanceSq > m_settings.stopDistance * m_settings.stopDistance) {
			input.moveDirection = direction * m_settings.moveRate;
		}
	}

	// 撃つかどうかを決める。
	const bool isInFireRange = distanceSq <= m_settings.fireRange * m_settings.fireRange;
	if (m_fireTimer > 0.0f) {
		m_fireTimer -= context.deltaTime;
	}
	if (m_isFiring) {
		m_fireHoldTimer -= context.deltaTime;
		if (m_fireHoldTimer <= 0.0f) {
			m_isFiring = false;
		}
	}
	else if (m_settings.canFire && isInFireRange && m_fireTimer <= 0.0f) {
		m_isFiring = true;
		m_fireHoldTimer = m_settings.fireHoldSeconds;
		m_fireTimer = m_settings.fireInterval;
	}
	input.fire.Update(m_isFiring);

	// 相手の胸のあたりを狙う。
	input.hasAim = true;
	input.aimOrigin = context.position;
	input.aimOrigin.y += m_settings.castHeight;
	input.aimDirection = target->GetCenter() - input.aimOrigin;
}
```

- **相手は `HitTargetRegistry::FindNearestHostile()` で探す。** 範囲の中で、いちばん近い敵対する相手です。自分（`context.self`）は除きます。
- **高さの差は無視して、水平に追う。** `stopDistance` まで近づいたら止まり、向きだけ合わせます。
- **見つけた直後にいきなり撃たない。** `firstFireDelay` だけ待ってから撃ち、その後は `fireInterval` ごとに撃ちます。長押しの魔法用に、押し続ける秒数（`fireHoldSeconds`）も決められます。
- **撃つ向きは相手の胸のあたり。** `hasAim` を立て、撃つ位置と向きを入れます。

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: Enemy の入力元を AI にする

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) | 追加（AI の設定を作る関数と、コンストラクターで渡す） |
| [Enemy.h](../../../Game/Source/Enemy/Enemy.h) | 追加・削除・書き換え（`#include`、説明のコメント、メンバー `m_npcInputSource`。仮の入力元を消し、既定の入力元を AI に書き換える） |

[Enemy.h](../../../Game/Source/Enemy/Enemy.h) を次のように変えます。

- `#include "NpcInputSource.h"` と、説明のコメント、メンバー `m_npcInputSource` を**追加**する。
- 仮の入力元 `EmptyInputSource` のクラスとメンバーを**削除**する。
- `GetDefaultInputSource()` が返すものを `m_npcInputSource` に**書き換え**る。

<!-- file: ../../../Game/Source/Enemy/Enemy.h -->

```diff
 
 #include "Character.h"
+#include "NpcInputSource.h"
 
-// NPCのAIができるまでの、何も入力しない入力元。NPCのAIの手順で消す。
-class EmptyInputSource : public IInputSource
-{
-public:
-	void Update(const InputSourceContext& context, CharacterInput& input) override
-	{
-	}
-};
-
 // 敵キャラクター。
+// 入力元（既定ではAI）から CharacterInput を受け取って動くだけで、何をするかはAIが決める。
 // 陣営を変えれば（SetFaction）味方や中立のNPCとしても使える。
 class Enemy : public Character
```

<!-- file: ../../../Game/Source/Enemy/Enemy.h -->

```diff
 	IInputSource& GetDefaultInputSource() override
 	{
-		return m_emptyInputSource;
+		return m_npcInputSource;
 	}
 	void UpdateAction(float deltaTime) override;
```

<!-- file: ../../../Game/Source/Enemy/Enemy.h -->

```diff
 
 private:
-	EmptyInputSource m_emptyInputSource;
+	NpcInputSource m_npcInputSource;
 	Status m_status;
 };
```

[Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) に、AI の設定を作る関数と、コンストラクターで AI に設定を渡す行を**追加**します。今は探す範囲を 0 にして、敵を止めておきます（動かず・向きも変えず・撃たない）。

<!-- file: ../../../Game/Source/Enemy/Enemy.cpp -->

```diff
 	constexpr float HIT_RADIUS = 45.0f;        // 弾に当たる球の半径。
 	constexpr float HIT_CENTER_HEIGHT = 60.0f;  // 足元から当たり判定の中心までの高さ。
+
+	NpcInputSource::Settings MakeAiSettings()
+	{
+		NpcInputSource::Settings settings;
+		// 今は敵を止めておく（相手を探さないので、動かず・向きも変えず・撃たない）。動かすときは 900 に戻す。
+		settings.searchRange = 0.0f;     // この距離以内に入ると追跡を開始する。
+		settings.stopDistance = 90.0f;   // この距離まで近づいたら停止する（近づきすぎ防止）。
+		settings.canFire = true;
+		settings.fireRange = 700.0f;
+		settings.fireInterval = 4.0f;    // 凍結(2.5秒)より長くして、凍らされ続けないようにする。
+		settings.firstFireDelay = 1.5f;
+		settings.castHeight = 60.0f;
+		return settings;
+	}
 }
 
```

<!-- file: ../../../Game/Source/Enemy/Enemy.cpp -->

```diff
 Enemy::Enemy()
 	: Character(Faction::Enemy)
+	, m_npcInputSource(MakeAiSettings())
 {
 }
```

**確かめ方（これで出来上がり）:** ソリューションのビルドが通る。`MakeAiSettings()` の `searchRange` を 900 にすると、Enemy が Player の方を向く（動いて撃つのは、移動・魔法の手順のあと）。
