#include "stdafx.h"
#include "MagicCaster.h"
#include "CharacterInput.h"

void MagicCaster::Init(IGameObject* owner, Faction faction, const Settings& settings)
{
	m_owner = owner;
	m_faction = faction;
	m_settings = settings;
}

void MagicCaster::AddMagic(std::unique_ptr<MagicBase> magic)
{
	if (magic != nullptr) {
		m_magics.push_back(std::move(magic));
	}
}

void MagicCaster::SetSubMagic(std::unique_ptr<MagicBase> magic)
{
	CancelSubHolding();
	m_subMagic = std::move(magic);
}

MagicBase* MagicCaster::GetCurrentMagic() const
{
	if (m_magics.empty()) {
		return nullptr;
	}
	return m_magics[m_currentIndex].get();
}

void MagicCaster::SelectMagic(int index)
{
	if (m_magics.empty()) {
		return;
	}
	const int count = static_cast<int>(m_magics.size());
	// 端まで行ったら反対側に回り込む。
	const int newIndex = ((index % count) + count) % count;
	if (newIndex == m_currentIndex) {
		return;
	}
	CancelHolding();
	m_currentIndex = newIndex;
}

void MagicCaster::Update(const CharacterInput& input, const Vector3& position, const Vector3& forward, float deltaTime, bool canAct)
{
	// 選んでいない魔法もクールダウンは進める。
	for (auto& magic : m_magics) {
		magic->Update(deltaTime);
	}
	if (m_subMagic != nullptr) {
		m_subMagic->Update(deltaTime);
	}

	if (!canAct) {
		CancelHolding();
		CancelSubHolding();
		return;
	}

	if (input.magicSwitch != 0) {
		SelectMagic(m_currentIndex + input.magicSwitch);
	}

	const MagicCastContext context = MakeContext(input, position, forward, deltaTime);
	if (MagicBase* magic = GetCurrentMagic()) {
		UpdateButton(*magic, input.fire, context, m_isHolding);
	}
	if (m_subMagic != nullptr) {
		UpdateButton(*m_subMagic, input.subFire, context, m_isSubHolding);
	}
}

void MagicCaster::UpdateButton(MagicBase& magic, const ButtonState& button, const MagicCastContext& context, bool& isHolding)
{
	if (button.isTrigger) {
		isHolding = true;
		magic.OnPress(context);
	}
	if (isHolding && button.isPress) {
		magic.OnHold(context);
	}
	if (isHolding && !button.isPress) {
		isHolding = false;
		magic.OnRelease(context);
	}
}

MagicCastContext MagicCaster::MakeContext(const CharacterInput& input, const Vector3& position, const Vector3& forward, float deltaTime) const
{
	MagicCastContext context;
	context.caster = m_owner;
	context.faction = m_faction;
	context.deltaTime = deltaTime;

	if (input.hasAim) {
		context.origin = input.aimOrigin;
		context.direction = input.aimDirection;
	}
	else {
		context.origin = position;
		context.origin.y += m_settings.castHeight;
		context.direction = forward;
		context.direction.y = 0.0f;
	}
	if (context.direction.LengthSq() <= FLT_EPSILON) {
		context.direction = Vector3::AxisZ;
	}
	context.direction.Normalize();
	context.origin += context.direction * m_settings.castForward;
	return context;
}

void MagicCaster::CancelHolding()
{
	if (!m_isHolding) {
		return;
	}
	m_isHolding = false;
	if (MagicBase* magic = GetCurrentMagic()) {
		magic->OnCancel();
	}
}

void MagicCaster::CancelSubHolding()
{
	if (!m_isSubHolding) {
		return;
	}
	m_isSubHolding = false;
	if (m_subMagic != nullptr) {
		m_subMagic->OnCancel();
	}
}
