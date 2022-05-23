LibraryPath = {}

LibraryPath["GLFW"] = "%{wks.location}/LucyEngine/vendor/GLFW"
LibraryPath["spdlog"] = "%{wks.location}/LucyEngine/vendor/spdlog"
LibraryPath["Glad"] = "%{wks.location}/LucyEngine/vendor/Glad"
LibraryPath["entt"] = "%{wks.location}/LucyEngine/vendor/entt"
LibraryPath["stb"] = "%{wks.location}/LucyEngine/vendor/stb"
LibraryPath["assimp"] = "%{wks.location}/LucyEngine/vendor/assimp"
LibraryPath["nativefiledialog"] = "%{wks.location}/LucyEngine/vendor/nativefiledialog"
LibraryPath["ImGui"] = "%{wks.location}/LucyEngine/vendor/ImGui"
LibraryPath["ImGuizmo"] = "%{wks.location}/LucyEditor/vendor/ImGuizmo"
LibraryPath["glm"] = "%{wks.location}/LucyEngine/vendor/glm"

LibraryPath["VMA"] = "%{wks.location}/LucyEngine/vendor/vma"
LibraryPath["VMADebug"] = "%{wks.location}/LucyEngine/vendor/vma/vma/Debug/VulkanMemoryAllocatord.lib"
LibraryPath["VMARelease"] = "%{wks.location}/LucyEngine/vendor/vma/vma/Release/VulkanMemoryAllocator.lib"

LibraryPath["VulkanSDK"] = os.getenv("VULKAN_SDK")
LibraryPath["VulkanInclude"] = "%{LibraryPath.VulkanSDK}/Include"
LibraryPath["VulkanLib"] = "%{LibraryPath.VulkanSDK}/Lib"

--For Debug
LibraryPath["ShaderCDebug"] = "%{LibraryPath.VulkanLib}/shaderc_sharedd.lib"
LibraryPath["SPIRVDebug"] = "%{LibraryPath.VulkanLib}/spirv-cross-cored.lib"
LibraryPath["SPIRVGLSLDebug"] = "%{LibraryPath.VulkanLib}/spirv-cross-glsld.lib"
LibraryPath["SPIRVTools"] = "%{LibraryPath.VulkanLib}/SPIRV-Toolsd.lib"
LibraryPath["SPIRVReflectDebug"] = "%{LibraryPath.VulkanLib}/spirv-cross-reflectd.lib"

--For Release
LibraryPath["ShaderCRelease"] = "%{LibraryPath.VulkanLib}/shaderc_shared.lib"
LibraryPath["SPIRVRelease"] = "%{LibraryPath.VulkanLib}/spirv-cross-core.lib"
LibraryPath["SPIRVGLSLRelease"] = "%{LibraryPath.VulkanLib}/spirv-cross-glsl.lib"
