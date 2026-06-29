#pragma once

namespace nsK2Engine {
	enum EnSkyCubeType {
		enSkyCubeType_Day,		// ����
		enSkyCubeType_Night,	// ���
		enSkyCubeType_Snow,		// ��R
		enSkyCubeType_Snow_2,	// ��R_2
		enSkyCubeType_Wild,		// �r��
		enSkyCubeType_Wild_2,	// �r��Q
		enSkyCubeType_Wild_Night,	// �r��(���)
		enSkyCubeType_Grass,	// �Ő�
		enSkyCubeType_Euro,		// ���B
		enSkyCubeType_DayToon,	// ����(�g�D�[����)
		enSkyCubeType_DayToon_2,// ����(�g�D�[����)�Q
		enSkyCubeType_DayToon_3,// ����(�g�D�[����)�R
		enSkyCubeType_DayToon_4,// ����(�g�D�[����)�S
		enSkyCubeType_NightToon, // ���(�g�D�[����)
		enSkyCubeType_NightToon_2, // ���(�g�D�[����)�Q
		enSkyCubeType_SunriseToon,	// ������(�g�D�[����)
		//enSkyCubeType_SpaceToon,	// ��C��(�g�D�[����) ����ɂłȂ��̂ŃR�����g�A�E�g�B
		enSkyCubeType_SpaceToon_2,	// ��C��(�g�D�[����)�Q
		enSkyCubeType_Num,
	};
	/// <summary>
	/// ?
	/// </summary>
	class SkyCube : public IGameObject
	{
	public:
		SkyCube();
		~SkyCube();
		bool Start()override final;
		void Update()override final;
		void Render(RenderContext& rc)override final;
	public:
		void SetPosition(const Vector3& pos)
		{
			m_position = pos;
			m_isDirty = true;
		}

		void SetScale(const Vector3& scale)
		{
			m_scale = scale;
			m_isDirty = true;
		}
		void SetScale(const float scale)
		{
			m_scale = g_vec3One;
			m_scale.Scale(scale);
			m_isDirty = true;
		}
		/// <summary>
		/// �X�J�C�L���[�u�̃^�C�v��ݒ�B
		/// </summary>
		/// <param name="type"></param>
		void SetType(EnSkyCubeType type) 
		{
			m_type = type;
		}
		/// <summary>
		/// ���邳��ݒ�B
		/// </summary>
		/// <param name="lum"></param>
		void SetLuminance(float lum)
		{
			m_luminance = lum;
		}
		const wchar_t* GetTextureFilePath()
		{
			return m_textureFilePaths[m_type];
		}
	private:
		ModelRender m_modelRender;
		
		Texture m_texture[enSkyCubeType_Num];
		const wchar_t* m_textureFilePaths[enSkyCubeType_Num];
		Vector3 m_position = g_vec3Zero;
		Vector3 m_scale = g_vec3One * 1000.0f;
		float m_luminance = 1.0f;
		bool m_isDirty = false;
		EnSkyCubeType m_type = enSkyCubeType_Day;
	};
}

