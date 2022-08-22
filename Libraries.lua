LibraryPath = {}

LibraryPath["GLFW"] = "%{wks.location}/LucyEngine/ThirdParty/GLFW"
LibraryPath["spdlog"] = "%{wks.location}/LucyEngine/ThirdParty/spdlog"
LibraryPath["entt"] = "%{wks.location}/LucyEngine/ThirdParty/entt"
LibraryPath["stb"] = "%{wks.location}/LucyEngine/ThirdParty/stb"
LibraryPath["assimp"] = "%{wks.location}/LucyEngine/ThirdParty/assimp"
LibraryPath["nativefiledialog"] = "%{wks.location}/LucyEngine/ThirdParty/nativefiledialog"
LibraryPath["ImGui"] = "%{wks.location}/LucyEngine/ThirdParty/ImGui"
LibraryPath["ImGuizmo"] = "%{wks.location}/LucyEditor/ThirdParty/ImGuizmo"
LibraryPath["glm"] = "%{wks.location}/LucyEngine/ThirdParty/glm"

LibraryPath["VMA"] = "%{wks.location}/LucyEngine/ThirdParty/vma"
LibraryPath["VMADebug"] = "%{wks.location}/LucyEngine/ThirdParty/vma/vma/Debug/VulkanMemoryAllocatord.lib"
LibraryPath["VMARelease"] = "%{wks.location}/LucyEngine/ThirdParty/vma/vma/Release/VulkanMemoryAllocator.lib"

LibraryPath["VulkanSDK"] = os.getenv("VULKAN_SDK")
LibraryPath["VulkanInclude"] = "%{LibraryPath.VulkanSDK}/Include"
LibraryPath["VulkanLib"] = "%{LibraryPath.VulkanSDK}/Lib"

LibraryPath["Optick"] = "%{wks.location}/LucyEngine/ThirdParty/Optick"

--For Debug
LibraryPath["ShaderCDebug"] = "%{LibraryPath.VulkanLib}/shaderc_sharedd.lib"
LibraryPath["SPIRVDebug"] = "%{LibraryPath.VulkanLib}/spirv-cross-cored.lib"
LibraryPath["SPIRVGLSLDebug"] = "%{LibraryPath.VulkanLib}/spirv-cross-glsld.lib"
LibraryPath["SPIRVTools"] = "%{LibraryPath.VulkanLib}/SPIRV-Toolsd.lib"

--For Release
LibraryPath["ShaderCRelease"] = "%{LibraryPath.VulkanLib}/shaderc_shared.lib"
LibraryPath["SPIRVGLSLRelease"] = "%{LibraryPath.VulkanLib}/spirv-cross-glsl.lib"
LibraryPath["SPIRVRelease"] = "%{LibraryPath.VulkanLib}/spirv-cross-core.lib"