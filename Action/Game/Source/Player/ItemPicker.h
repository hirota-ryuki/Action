#pragma once

class Item;
class ItemManager;
class PlayerStatus;

// プレイヤーの部品。拾える半径の中にあるアイテムを、近い順に並べて持つ。
// 拾うかどうかは決めない（地上のステートが、拾うボタンを見て PickUpFirst を呼ぶ）。
// 拾える一覧の UI（PickupHud）も、この一覧を読んで描く。
class ItemPicker
{
public:
	// manager が nullptr の間は、一覧はいつも空。
	void SetManager(ItemManager* manager)
	{
		m_manager = manager;
	}
	// 足元からこの距離以内のアイテムを拾える。
	void SetRadius(float radius)
	{
		m_radius = radius;
	}
	// 毎フレーム、ステートを動かす前に呼ぶ。足元から半径の中を探し直す。
	void Update(const Vector3& footPosition);

	// 近い順。先頭が、拾うボタンで拾うもの。
	const std::vector<Item*>& GetCandidates() const
	{
		return m_candidates;
	}
	bool HasCandidate() const
	{
		return !m_candidates.empty();
	}
	// いちばん近いものを拾い、一覧から外す。一覧が空なら何もしない。
	void PickUpFirst(PlayerStatus& status);

private:
	ItemManager* m_manager = nullptr;
	float m_radius = 0.0f;
	std::vector<Item*> m_candidates;
};
