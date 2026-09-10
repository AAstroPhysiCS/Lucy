#pragma once

#include "Core/Base.h"

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"
#include "slang/slang-com-helper.h"

namespace Lucy {

	using namespace slang;

	enum class ShaderStageType;

	class RenderDevice;
	class Shader;
	class VulkanDescriptorSetManager;

	//per name, it could have multiple shader programs
	//that means, that each file can uphold multiple in multiple shader programs (multiple entrypoints) (e.g. VertexFragment and Compute and Tessellation etc...)
	using ShaderStageMap = std::unordered_map<ShaderStageType, std::vector<Ref<Shader>>>;
	using ShaderLibrary = std::unordered_map<std::string, ShaderStageMap>;

	class ShaderManager final {
	private:
		static inline std::filesystem::path s_ShaderFolder = "Assets/Shaders";
		static inline std::filesystem::path s_CacheFolder = "Assets/Shaders/Cached";

		struct ShaderProgram {
			ShaderStageType Stage = ShaderStageType::Unknown;
			std::string EntryPointName;
			Slang::ComPtr<slang::IBlob> Blob;
			Slang::ComPtr<slang::IComponentType> LinkedProgram;
		};
	public:
		ShaderManager();
		~ShaderManager() = default;

		ShaderManager(const ShaderManager&) = delete;
		ShaderManager(ShaderManager&&) = delete;
		ShaderManager& operator=(const ShaderManager&) = delete;
		ShaderManager& operator=(ShaderManager&&) = delete;

		void InitializeShaders(Ref<RenderDevice> device, const std::initializer_list<const char*> excludeList = {});
		
		static std::filesystem::path& GetShaderFolder() { return s_ShaderFolder; }
		static std::filesystem::path& GetCacheFolder() { return s_CacheFolder; }

		std::vector<Ref<Shader>> ReloadShader(Ref<RenderDevice> device, const std::string& name);
		bool HasShader(const std::string& name) const { return m_Shaders.contains(name); }

		void DestroyAllShaders(Ref<RenderDevice> device);
		void Destroy();

		const ShaderStageMap& GetShaderStageMap(const std::string& name) const { return m_Shaders.at(name); }
		const ShaderLibrary& GetShaderLibrary() const { return m_Shaders; };

		Ref<Shader> GetShader(ShaderStageType type, const std::string& name, std::string_view entryPointName) const;
	private:
		Slang::ComPtr<slang::ISession> CreateNewSlangSession();

		void ProcessShaderFiles(const std::filesystem::path& directory, std::string_view extension, const std::function<void(const std::string&, const std::filesystem::path&)>& action,
			const std::initializer_list<const char*>& excludeList);

		void CompileShaders(Ref<RenderDevice> device, const std::initializer_list<const char*> excludeList);
		void CompileShadersFromCache(Ref<RenderDevice> device, const std::initializer_list<const char*> excludeList);

		Slang::ComPtr<IModule> CreateSlangModule(const std::string& name, const std::filesystem::path& path, Ref<RenderDevice> device, Slang::ComPtr<ISession> session);

		//key: shader stage, value: vector of compiled blobs
		std::vector<ShaderManager::ShaderProgram> RunSlangCompiler(const std::filesystem::path& path, Slang::ComPtr<slang::IModule> slangModule, Slang::ComPtr<ISession> session);
		Slang::ComPtr<slang::IComponentType> LoadProgram(const std::filesystem::path& path, Slang::ComPtr<slang::IModule> slangModule, Slang::ComPtr<slang::IEntryPoint> entryPoint, Slang::ComPtr<ISession> session, ShaderStageType shaderStage);
		IModule* LoadSlangModuleFromCache(std::string_view name, const std::filesystem::path& cachedFilePath, Slang::ComPtr<ISession> session);

		ShaderStageMap CreateShaders(const std::string& name, const std::filesystem::path& path, const Ref<RenderDevice>& device, const std::vector<ShaderManager::ShaderProgram>& shaderPrograms);

		void PrintDiagnosticsIfFails(const Slang::ComPtr<slang::IBlob>& diagnosticsBlob, const std::filesystem::path& path) const;
		void PrintDiagnosticsIfFails(const Slang::ComPtr<slang::IBlob>& diagnosticsBlob, const std::filesystem::path& path, ShaderStageType stageFlag) const;

		std::string GetCachedFileExtension() const;

		ShaderLibrary m_Shaders;

		Slang::ComPtr<IGlobalSession> m_GlobalSession;
		Slang::ComPtr<ISession> m_Session;
	};
}