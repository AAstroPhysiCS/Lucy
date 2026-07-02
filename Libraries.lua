LibraryPath = {}

LibraryPath["GLFW"] = "%{wks.location}/LucyEngine/ThirdParty/GLFW"
LibraryPath["spdlog"] = "%{wks.location}/LucyEngine/ThirdParty/spdlog"
LibraryPath["entt"] = "%{wks.location}/LucyEngine/ThirdParty/entt"
LibraryPath["stb"] = "%{wks.location}/LucyEngine/ThirdParty/stb"
LibraryPath["assimp"] = "%{wks.location}/LucyEngine/ThirdParty/assimp"
LibraryPath["nativefiledialog"] = "%{wks.location}/LucyEngine/ThirdParty/nativefiledialog"
LibraryPath["ImGui"] = "%{wks.location}/LucyEngine/ThirdParty/ImGui"
LibraryPath["ImGuizmo"] = "%{wks.location}/LucyEditor/ThirdParty/ImGuizmo"
LibraryPath["Tracy"] = "%{wks.location}/LucyEngine/ThirdParty/Tracy"
LibraryPath["glm"] = "%{wks.location}/LucyEngine/ThirdParty/glm"

LibraryPath["VulkanSDK"] = os.getenv("VULKAN_SDK")
LibraryPath["VulkanInclude"] = "%{LibraryPath.VulkanSDK}/Include"
LibraryPath["VulkanLib"] = "%{LibraryPath.VulkanSDK}/Lib"

LibraryPath["SlangDebug"] = "%{LibraryPath.VulkanLib}/slangd.lib"
LibraryPath["SlangDebugRT"] = "%{LibraryPath.VulkanLib}/slang-rtd.lib"

LibraryPath["SlangRelease"] = "%{LibraryPath.VulkanLib}/slang.lib"
LibraryPath["SlangReleaseRT"] = "%{LibraryPath.VulkanLib}/slang-rt.lib"
