#include "lypch.h"

#include <ranges>
#include <numeric>

#include "ShaderManager.h"
#include "Utilities/Utilities.h"

#include "Core/FileSystem.h"
#include "Core/Application.h"
#include "Core/Timer.h"

#include "VulkanGraphicsShader.h"
#include "VulkanComputeShader.h"
#include "VulkanRayTracingShader.h"

#include "Renderer/Renderer.h"

namespace Lucy {

#ifdef LUCY_DEBUG
	static void WriteSpirvForNsight(const std::filesystem::path& directory, std::string_view moduleName, std::string_view entryPointName, ShaderStageType type, const Slang::ComPtr<slang::IBlob>& blob) {
		LUCY_ASSERT(blob, "Cannot write an empty SPIR-V blob!");

		std::filesystem::create_directories(directory);
		const std::filesystem::path outputPath = directory / std::format("{}__{}__{}.spv", moduleName, entryPointName, ShaderStageToShaderString(type));

		std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);

		LUCY_ASSERT(output.is_open(), "Failed to open SPIR-V output file: {0}", outputPath.string());
		output.write(static_cast<const char*>(blob->getBufferPointer()), static_cast<std::streamsize>(blob->getBufferSize()));
		LUCY_ASSERT(output.good(), "Failed to write SPIR-V output file: {0}", outputPath.string());
	}
#endif

	ShaderManager::ShaderManager() {
		SlangGlobalSessionDesc desc = {};
		desc.minLanguageVersion = SLANG_LANGUAGE_VERSION_LATEST;

		createGlobalSession(&desc, m_GlobalSession.writeRef());

		m_Session = std::move(CreateNewSlangSession());
	}

	void ShaderManager::Destroy() {
		shutdown();
	}

	Ref<Shader> ShaderManager::GetShader(ShaderStageType type, const std::string& name, std::string_view entryPointName) const {
		const auto moduleIt = m_Shaders.find(name);
		LUCY_ASSERT(moduleIt != m_Shaders.end(), "Shader module {0} does not exist!", name);

		const auto stageIt = moduleIt->second.find(type);
		LUCY_ASSERT(stageIt != moduleIt->second.end(), "Shader module {0} does not contain stage {1}!", name, ShaderStageToShaderString(type));

		const auto shaderIt = std::ranges::find_if(stageIt->second, [entryPointName](const Ref<Shader>& shader) {
			return shader->GetEntryPointName() == entryPointName;
		});

		LUCY_ASSERT(shaderIt != stageIt->second.end(), "Shader module {0} does not contain entry point {1}!", name, entryPointName);

		return *shaderIt;
	}

	Slang::ComPtr<slang::ISession> ShaderManager::CreateNewSlangSession() {
		const auto& shaderFolder = GetShaderFolder();
		std::string shaderFolderString = shaderFolder.string();

		slang::TargetDesc targetDesc = {};
		targetDesc.format = SLANG_SPIRV;
		targetDesc.profile = m_GlobalSession->findProfile("sm_6_6");

		std::vector<slang::CompilerOptionEntry> stringOptions =
		{
			{
				slang::CompilerOptionName::Capability,
				{ slang::CompilerOptionValueKind::String, 0, 0, "vk_mem_model", "vk_mem_model" }
			},
			{
				slang::CompilerOptionName::LanguageVersion,
				{ slang::CompilerOptionValueKind::String, 0, 0, "2026", "2026" }
			},
		};

		slang::SessionDesc sessionDesc = {};
		sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;
		//sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(stringOptions.size());
		//sessionDesc.compilerOptionEntries = stringOptions.data();warning[E41012]: profile implicitly upgraded

		/*const std::array<slang::PreprocessorMacroDesc, 2> macros{
			slang::PreprocessorMacroDesc{
				"PBR_HAS_NORMAL_MAP",
				"1"
			},
			slang::PreprocessorMacroDesc{
				"PBR_ALPHA_MASKED",
				"1"
			}
		};

		sessionDesc.preprocessorMacros = macros.data();
		sessionDesc.preprocessorMacroCount = static_cast<uint32_t>(macros.size());*/

		const char* searchPaths[] = { shaderFolderString.c_str() };
		sessionDesc.searchPaths = searchPaths;
		sessionDesc.searchPathCount = 1;

		std::vector<slang::CompilerOptionEntry> options =
		{
			{
				slang::CompilerOptionName::Capability,
				{ slang::CompilerOptionValueKind::String, 0, 0, "vk_mem_model", "vk_mem_model" },
			},
			{
				slang::CompilerOptionName::Capability,
				{ slang::CompilerOptionValueKind::String, 0, 0, "spvGroupNonUniform", "spvGroupNonUniform" },
			},
			{
				slang::CompilerOptionName::Capability,
				{ slang::CompilerOptionValueKind::String, 0, 0, "scalar-block-layout", "scalar-block-layout" },
			},
			{
				slang::CompilerOptionName::EmitSpirvDirectly,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},
			{
				slang::CompilerOptionName::MatrixLayoutColumn,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},
			/*{
				slang::CompilerOptionName::BindlessSpaceIndex,
				{slang::CompilerOptionValueKind::Int, 0, 0, nullptr, nullptr}
			},*/
#ifdef LUCY_DEBUG
#if USE_INTEGRATED_GRAPHICS == 0
			{	// DOES NOT WORK WITH AMD INTEGRATED GPUS
				slang::CompilerOptionName::DebugInformation,
				{slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_MAXIMAL, 0, nullptr, nullptr}
			},
#endif
			{
				slang::CompilerOptionName::EnableWarning,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},
			{
				slang::CompilerOptionName::EnableExperimentalPasses,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},
			{
				slang::CompilerOptionName::EnableEffectAnnotations,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},
#endif
			/*{
				slang::CompilerOptionName::VulkanUseGLLayout,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},*/
			{
				slang::CompilerOptionName::VulkanUseEntryPointName,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},
			{
				slang::CompilerOptionName::GLSLForceScalarLayout,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},
			{
				slang::CompilerOptionName::VulkanUseDxPositionW,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},
			{
				slang::CompilerOptionName::MatrixLayoutColumn,
				{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			},
			{
				slang::CompilerOptionName::Optimization,
				{ slang::CompilerOptionValueKind::Int, SLANG_OPTIMIZATION_LEVEL_HIGH, 0, nullptr, nullptr }
			},
			{
				slang::CompilerOptionName::ValidateUniformity,
				{ slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr }
			},
			{
				slang::CompilerOptionName::Obfuscate,
#ifdef LUCY_DEBUG
				{ slang::CompilerOptionValueKind::Int, 0, 0, nullptr, nullptr }
#else
				{ slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr }
#endif
			}
		};

		//if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
			//options.emplace_back(slang::CompilerOptionName::VulkanInvertY, CompilerOptionValue{ slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr });

		targetDesc.compilerOptionEntries = options.data();
		targetDesc.compilerOptionEntryCount = options.size();

		sessionDesc.compilerOptionEntries = options.data();
		sessionDesc.compilerOptionEntryCount = options.size();

		Slang::ComPtr<ISession> session;
		m_GlobalSession->createSession(sessionDesc, session.writeRef());
		return session;
	}

	void ShaderManager::InitializeShaders(Ref<RenderDevice> device, const std::initializer_list<const char*> excludeList) {
		const auto& cachedFolder = GetCacheFolder();
		if (!FileSystem::DirectoryExists(cachedFolder) || FileSystem::GetDirectoryFileCount(cachedFolder) == 0) {
			FileSystem::CreateDir(cachedFolder);
			CompileShaders(device, excludeList);
			return;
		}
		CompileShadersFromCache(device, {});
	}

	void ShaderManager::ProcessShaderFiles(const std::filesystem::path& directory, std::string_view extension, const std::function<void(const std::string&, const std::filesystem::path&)>& action,
		const std::initializer_list<const char*>& excludeList) {
		TaskScheduler* taskScheduler = Application::GetTaskScheduler();
		{
			ScopedTimer timer("Shader Compilation");
			static std::mutex shaderMutex;

			for (const auto& entry : std::filesystem::directory_iterator(directory)) {
				if (!entry.is_regular_file() || entry.path().extension().string() != extension)
					continue;

				std::string name = entry.path().stem().stem().string();
				if (std::find(excludeList.begin(), excludeList.end(), name) != excludeList.end())
					continue;

				taskScheduler->Schedule(TaskScheduler::Launch::Async, TaskPriority::High,
					[=](const TaskArgs& args) {
					std::unique_lock lock(shaderMutex);
					action(name, entry.path());
				});
			}
		}
		taskScheduler->WaitForAllTasks();
		LUCY_INFO("Loaded {0} shaders", m_Shaders.size());
	}

	void ShaderManager::CompileShaders(Ref<RenderDevice> device, const std::initializer_list<const char*> excludeList) {
		ProcessShaderFiles(GetShaderFolder(), ".slang", [this, device](const std::string& name, const std::filesystem::path& path) {
			auto slangModule = CreateSlangModule(name, path, device, m_Session);

			auto shaderPrograms = RunSlangCompiler(path, slangModule, m_Session);
			//means that we are compiling a shader that has no entry points defined (utility shaders)
			if (shaderPrograms.empty())
				return;

			const auto& cachedFolder = GetCacheFolder();
			const auto& cachedFileExtension = GetCachedFileExtension();
			auto cachedFolderWithName = cachedFolder / (std::filesystem::path(name));

			std::ranges::stable_sort(shaderPrograms, {}, &ShaderProgram::Stage);

			const ShaderStageMap& shaderResources = CreateShaders(name, path, device, shaderPrograms);

			auto pathToWrite = cachedFolderWithName.replace_extension(cachedFileExtension);
			slangModule->writeToFile(pathToWrite.string().c_str());

			m_Shaders.try_emplace(name, shaderResources);
			LUCY_INFO("Done loading shader: {0}", name);
		}, excludeList);
	}

	void ShaderManager::CompileShadersFromCache(Ref<RenderDevice> device, const std::initializer_list<const char*> excludeList) {
		std::atomic_bool newBuildConfigurationDetected = false;
		ProcessShaderFiles(GetCacheFolder(), ".slang", [this, device, &excludeList, &newBuildConfigurationDetected](const std::string& name, const std::filesystem::path& path) {
			//if we do not find the right cache file that was compiled with the current build configuration
			//we need to recompile the shader according to the current solution configuration and create a new cache file for the current build configuration
			if (path.extension() != GetCachedFileExtension()) {
				newBuildConfigurationDetected = true;
				return;
			}

			Slang::ComPtr<slang::IModule> slangModule{ LoadSlangModuleFromCache(name, path, m_Session) };

			auto shaderPrograms = RunSlangCompiler(path, slangModule, m_Session);
			//means that we are compiling a shader that has no entry points defined (utility shaders)
			if (shaderPrograms.empty())
				return;

			std::ranges::stable_sort(shaderPrograms, {}, &ShaderProgram::Stage);

			const auto& shaderResources = CreateShaders(name, path, device, shaderPrograms);

			m_Shaders.try_emplace(name, shaderResources);
			LUCY_INFO("Done loading shader: {0} from cache", name);
		}, excludeList);

		if (newBuildConfigurationDetected) {
			LUCY_WARN("New build configuration detected, recompiling shaders...");
			CompileShaders(device, excludeList);
		}
	}

	static auto ProgramBlobToSpan = [](const Slang::ComPtr<IBlob>& blob) -> std::span<const uint32_t> {
		return std::span<const uint32_t>(
			reinterpret_cast<const uint32_t*>(blob->getBufferPointer()),
			blob->getBufferSize() / sizeof(uint32_t));
	};

	ShaderStageMap ShaderManager::CreateShaders(const std::string& name, const std::filesystem::path& path, const Ref<RenderDevice>& device, const std::vector<ShaderManager::ShaderProgram>& shaderPrograms) {
		LUCY_INFO("------------------{0}------------------", name);

		ShaderStageMap resources;
		resources.reserve(shaderPrograms.size());

		for (size_t i = 0; i < shaderPrograms.size(); i++) {
			const auto& shaderProgram = shaderPrograms[i];

			if (shaderProgram.Stage == ShaderStageType::Fragment)
				continue;

			if (!shaderProgram.Blob) {
				LUCY_CRITICAL("No compiled blobs found for shader stage: {0} in shader: {1}", ShaderStageToShaderString(shaderProgram.Stage), name);
				continue;
			}

			LUCY_INFO("Shader {0} has {1} bytes of compiled code", path.string(), shaderProgram.Blob->getBufferSize());

			/*
			* We cant use here the entry point name that we arbitarily set... some drivers still replace the name with "main"... so just default it to main
			*/
			switch (shaderProgram.Stage) {
				case ShaderStageType::Vertex: {
					//the next shader stage must be Fragment (the name of fragment and entrypoint must be the same as the vertex shader stage)
					const auto& fragmentProgram = shaderPrograms[i + 1];
					LUCY_ASSERT(fragmentProgram.Stage == ShaderStageType::Fragment, 
						"Shader {0} has vertex stage but next stage is not fragment, it is: {1}", path.string(), ShaderStageToShaderString(fragmentProgram.Stage));

					auto shader = Memory::CreateRef<VulkanGraphicsShader>(name, path, shaderProgram.EntryPointName, device, ProgramBlobToSpan(shaderProgram.Blob), ProgramBlobToSpan(fragmentProgram.Blob));
					shader->RunReflect(shaderProgram.LinkedProgram, ShaderStageType::Vertex, shaderProgram.EntryPointName);
					shader->RunReflect(fragmentProgram.LinkedProgram, ShaderStageType::Fragment, fragmentProgram.EntryPointName);
					shader->PrintReflectInfo();

					resources[ShaderStageType::VertexAndFragment].emplace_back(std::move(shader));
					break;
				}
				case ShaderStageType::Compute: {
					auto shader = Memory::CreateRef<VulkanComputeShader>(name, path, shaderProgram.EntryPointName, device, ProgramBlobToSpan(shaderProgram.Blob));
					shader->RunReflect(shaderProgram.LinkedProgram, ShaderStageType::Compute, shaderProgram.EntryPointName);
					shader->PrintReflectInfo();

					resources[ShaderStageType::Compute].emplace_back(std::move(shader));
					break;	
				}
				case ShaderStageType::RayGen:
				case ShaderStageType::Miss:
				case ShaderStageType::Closest:
				case ShaderStageType::AnyHit: {
					auto shader = Memory::CreateRef<VulkanRayTracingShader>(name, path, shaderProgram.EntryPointName, shaderProgram.Stage, device, ProgramBlobToSpan(shaderProgram.Blob));
					shader->RunReflect(shaderProgram.LinkedProgram, shaderProgram.Stage, shaderProgram.EntryPointName);
					shader->PrintReflectInfo();

					resources[shaderProgram.Stage].emplace_back(std::move(shader));
					break;
				}
				default: 
					LUCY_ASSERT(false, "Shader stage {0} is not supported yet!", ShaderStageToShaderString(shaderProgram.Stage));
			}
		}

		return resources;
	}

	std::vector<ShaderManager::ShaderProgram> ShaderManager::RunSlangCompiler(const std::filesystem::path& path, Slang::ComPtr<slang::IModule> slangModule, Slang::ComPtr<ISession> session) {
		Slang::ComPtr<IBlob> diagnosticsBlob;

		auto entryPointCount = slangModule->getDefinedEntryPointCount();

		std::vector<ShaderProgram> shaderPrograms;
		shaderPrograms.resize(entryPointCount);

		for (uint32_t i = 0; i < entryPointCount; i++) {
			Slang::ComPtr<IEntryPoint> entryPoint;
			slangModule->getDefinedEntryPoint(i, entryPoint.writeRef());

			const char* nameOfEntry = entryPoint->getFunctionReflection()->getName();
			ShaderStageType shaderStage = SlangStageToShaderStage(entryPoint->getLayout()->getEntryPointByIndex(0)->getStage());

			LUCY_INFO("Compiling {0} shader entry point: {1}", ShaderStageToShaderString(shaderStage), nameOfEntry);

			Slang::ComPtr<IComponentType> linkedProgram = LoadProgram(path, slangModule, entryPoint, session, shaderStage);

			Slang::ComPtr<IBlob> blob;
			linkedProgram->getEntryPointCode(0, 0, blob.writeRef(), diagnosticsBlob.writeRef());
			PrintDiagnosticsIfFails(diagnosticsBlob, path, shaderStage);
			LUCY_ASSERT(blob, "Failed to load {0} shader from path: {1}", ShaderStageToShaderString(shaderStage), path.string());

			shaderPrograms[i].Stage = shaderStage;
			shaderPrograms[i].EntryPointName = nameOfEntry;
			shaderPrograms[i].Blob = blob;
			shaderPrograms[i].LinkedProgram = linkedProgram;

#ifdef LUCY_DEBUG
			WriteSpirvForNsight(GetCacheFolder() / "Nsight", path.stem().string(), nameOfEntry, shaderStage, blob);
#endif
		}

		return shaderPrograms;
	}

	Slang::ComPtr<slang::IComponentType> ShaderManager::LoadProgram(const std::filesystem::path& path, Slang::ComPtr<slang::IModule> slangModule, Slang::ComPtr<slang::IEntryPoint> entryPoint, Slang::ComPtr<ISession> session, ShaderStageType shaderStage) {
		const auto LoadSlangInternals = [&]() -> Slang::ComPtr<slang::IComponentType> {
			LUCY_ASSERT(entryPoint, "Failed to find entry point 'main' in shader module from path: {0}", path.string());

			std::array<slang::IComponentType*, 2> componentTypes = {
				slangModule,
				entryPoint
			};

			Slang::ComPtr<slang::IComponentType> composedProgram;
			Slang::ComPtr<slang::IBlob> diagnosticsBlob;

			SlangResult result = session->createCompositeComponentType(
				componentTypes.data(),
				componentTypes.size(),
				composedProgram.writeRef(),
				diagnosticsBlob.writeRef());
			PrintDiagnosticsIfFails(diagnosticsBlob, path, shaderStage);
			if (SLANG_FAILED(result)) {
				LUCY_CRITICAL("Failed to create composite component type for shader. Path: {0}", path.string());
				return nullptr;
			}

			Slang::ComPtr<slang::IComponentType> linkedProgram;
			result = composedProgram->link(linkedProgram.writeRef(), diagnosticsBlob.writeRef());
			PrintDiagnosticsIfFails(diagnosticsBlob, path, shaderStage);

			if (SLANG_FAILED(result)) {
				LUCY_CRITICAL("Failed to link shader. Path: {0}", path.string());
				return nullptr;
			}

			return linkedProgram;
		};

		return LoadSlangInternals();
	}

	Slang::ComPtr<IModule> ShaderManager::CreateSlangModule(const std::string& name, const std::filesystem::path& path, Ref<RenderDevice> device, Slang::ComPtr<ISession> session) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan)
			LUCY_ASSERT(false, "Not yet implemented for other graphics api's");

		std::vector<std::string> lines;
		FileSystem::ReadFileLine<std::string>(path, lines);
		const auto& fullSource = Utils::CombineDataToSingleBuffer(lines.begin(), lines.end());

		Slang::ComPtr<IModule> slangModule;
		Slang::ComPtr<IBlob> diagnosticsBlob;
		slangModule = session->loadModuleFromSourceString(name.c_str(), path.string().c_str(), fullSource.c_str(), diagnosticsBlob.writeRef());
		PrintDiagnosticsIfFails(diagnosticsBlob, path);
		LUCY_ASSERT(slangModule, "Failed to load path: {0}", path.string());

		return slangModule;
	}

	std::vector<Ref<Shader>> ShaderManager::ReloadShader(Ref<RenderDevice> device, const std::string& name) {
		auto& shaderStageMap = m_Shaders.at(name);
		auto type = ShaderStageType::Vertex;

		std::filesystem::path path;
		if (shaderStageMap.contains(ShaderStageType::VertexAndFragment))
			type = ShaderStageType::VertexAndFragment;
		else
			type = ShaderStageType::Compute;

		path = shaderStageMap.at(type)[0]->GetPath();
		shaderStageMap.at(type).clear();

		if (path.parent_path() == GetCacheFolder())
			path = (GetShaderFolder() / path.stem().stem()).replace_extension(".slang");

		Slang::ComPtr<slang::ISession> hotReloadSession = CreateNewSlangSession();

		auto slangModule = CreateSlangModule(name, path, device, hotReloadSession);

		const std::vector<ShaderProgram>& shaderPrograms = RunSlangCompiler(path, slangModule, hotReloadSession);
		if (shaderPrograms.empty()) {
			LUCY_CRITICAL("Failed to compile shader: {0}", path.string());
			return {};
		}
		//TODO: Maybe clean it?

		for (size_t i = 0; i < shaderPrograms.size(); i++) {
			const auto& shaderProgram = shaderPrograms[i];

			if (shaderProgram.Stage == ShaderStageType::Fragment)
				continue;

			if (!shaderProgram.Blob) {
				LUCY_CRITICAL("No compiled blobs found for shader stage: {0} in shader: {1}", ShaderStageToShaderString(shaderProgram.Stage), name);
				continue;
			}

			LUCY_INFO("Shader {0} has {1} bytes of compiled code", path.string(), shaderProgram.Blob->getBufferSize());

			/*
			* We cant use here the entry point name that we arbitarily set... some drivers still replace the name with "main"... so just default it to main
			*/
			switch (shaderProgram.Stage) {
				case ShaderStageType::Vertex: {
					//the next shader stage must be Fragment (the name of fragment and entrypoint must be the same as the vertex shader stage)
					const auto& fragmentProgram = shaderPrograms[i + 1];
					LUCY_ASSERT(fragmentProgram.Stage == ShaderStageType::Fragment, 
						"Shader {0} has vertex stage but next stage is not fragment, it is: {1}", path.string(), ShaderStageToShaderString(fragmentProgram.Stage));

					auto shader = Memory::CreateRef<VulkanGraphicsShader>(name, path, shaderProgram.EntryPointName, device, ProgramBlobToSpan(shaderProgram.Blob), ProgramBlobToSpan(fragmentProgram.Blob));
					shader->RunReflect(shaderProgram.LinkedProgram, ShaderStageType::Vertex, shaderProgram.EntryPointName);
					shader->RunReflect(fragmentProgram.LinkedProgram, ShaderStageType::Fragment, fragmentProgram.EntryPointName);
					shader->PrintReflectInfo();

					shaderStageMap[type].emplace_back(std::move(shader));
					break;
				}
				case ShaderStageType::Compute: {
					auto shader = Memory::CreateRef<VulkanComputeShader>(name, path, shaderProgram.EntryPointName, device, ProgramBlobToSpan(shaderProgram.Blob));
					shader->RunReflect(shaderProgram.LinkedProgram, ShaderStageType::Compute, shaderProgram.EntryPointName);
					shader->PrintReflectInfo();

					shaderStageMap[type].emplace_back(std::move(shader));
					break;	
				}
				default: 
					LUCY_ASSERT(false, "Shader stage {0} is not supported yet!", ShaderStageToShaderString(shaderProgram.Stage));
			}
		}

		return shaderStageMap[type];
	}

	void ShaderManager::DestroyAllShaders(Ref<RenderDevice> device) {
		for (const auto& shadersPerStage : m_Shaders | std::views::values)
			for (const auto& shaderList : shadersPerStage | std::views::values)
				for (const auto& shader : shaderList)
					shader->RTDestroyResource(device);
	}

	IModule* ShaderManager::LoadSlangModuleFromCache(std::string_view name, const std::filesystem::path& cachedFilePath, Slang::ComPtr<ISession> session) {
		if (!FileSystem::FileExists(cachedFilePath)) {
			LUCY_CRITICAL("Shader cache file does not exist at path: {0}", cachedFilePath.string());
			return nullptr;
		}

		std::vector<uint8_t> fileData;
		FileSystem::ReadFile(cachedFilePath, fileData, OpenMode::Binary);
		if (fileData.empty()) {
			LUCY_CRITICAL("Shader cache file is empty at path: {0}", cachedFilePath.string());
			return nullptr;
		}

		class FileBlob : public slang::IBlob {
		public:
			FileBlob(std::vector<uint8_t>&& data) : m_data(std::move(data)) {}

			// IBlob methods
			SLANG_NO_THROW void const* SLANG_MCALL getBufferPointer() SLANG_OVERRIDE { return m_data.data(); }
			SLANG_NO_THROW size_t SLANG_MCALL getBufferSize() SLANG_OVERRIDE { return m_data.size(); }

			// IUnknown methods
			SLANG_NO_THROW SlangResult SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) SLANG_OVERRIDE {
				if (uuid == slang::IBlob::getTypeGuid()) {
					*outObject = static_cast<slang::IBlob*>(this);
					addRef();
					return SLANG_OK;
				}
				return SLANG_E_NO_INTERFACE;
			}

			SLANG_NO_THROW uint32_t SLANG_MCALL addRef() SLANG_OVERRIDE { return ++m_refCount; }
			SLANG_NO_THROW uint32_t SLANG_MCALL release() SLANG_OVERRIDE {
				if (--m_refCount == 0) {
					delete this;
					return 0;
				}
				return m_refCount;
			}
		private:
			std::vector<uint8_t> m_data;
			std::atomic<uint32_t> m_refCount = 1;
		};

		FileBlob fileBlob(std::move(fileData));
		Slang::ComPtr<slang::IBlob> cachedModule(&fileBlob);
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		IModule* module = session->loadModuleFromIRBlob(
			name.data(),
			cachedFilePath.string().c_str(),
			cachedModule,
			diagnosticsBlob.writeRef()
		);

		PrintDiagnosticsIfFails(diagnosticsBlob, cachedFilePath, ShaderStageType::VertexAndFragmentAndCompute);
		return module;
	}

	void Lucy::ShaderManager::PrintDiagnosticsIfFails(const Slang::ComPtr<slang::IBlob>& diagnosticsBlob, const std::filesystem::path& path) const {
		PrintDiagnosticsIfFails(diagnosticsBlob, path, ShaderStageType::Unknown);
	}

	void ShaderManager::PrintDiagnosticsIfFails(const Slang::ComPtr<slang::IBlob>& diagnosticsBlob, const std::filesystem::path& path, ShaderStageType shaderType) const {
		if (!diagnosticsBlob || diagnosticsBlob->getBufferSize() == 0)
			return;
		std::string_view diagnostics{
			static_cast<const char*>(diagnosticsBlob->getBufferPointer()),
			diagnosticsBlob->getBufferSize()
		};
		LUCY_CRITICAL("Slang diagnostics for {0} shader at path: {1}\n{2}", ShaderStageToShaderString(shaderType), path.string(), diagnostics);
	}

	std::string ShaderManager::GetCachedFileExtension() const {
		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
#if LUCY_DEBUG
			return { ".cached_vulkan_debug.slang" };
#else
			return { ".cached_vulkan_release.slang" };
#endif
		}
		return { ".undefined" };
	}
}

