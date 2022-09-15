#pragma once

namespace Lucy {

	class Shader;
	
	class ShaderLibrary final {
	public:
		static ShaderLibrary& Get();

		ShaderLibrary() = default;
		~ShaderLibrary() = default;

		void Init();
		void Destroy();

		Ref<Shader> GetShader(const std::string& name) const;
		void PushShader(const Ref<Shader>& instance);
	private:
		std::vector<Ref<Shader>> m_Shaders;
	};
}