#include "DetailsPanel.h"
#include "SceneExplorerPanel.h"

#include "Renderer/Image/Image.h"
#include "Renderer/Renderer.h"
#include "Renderer/Material/PBRMaterial.h"

#include "Utilities/Utilities.h"

#include "imgui.h"

namespace Lucy {
	
	DetailsPanel& DetailsPanel::GetInstance() {
		static DetailsPanel s_Instance;
		return s_Instance;
	}

	DetailsPanel::DetailsPanel() {
		Renderer::EnqueueToRenderCommandQueue([](const Ref<RenderDevice>& device) {
			ImageCreateInfo createInfo;
			createInfo.Format = ImageFormat::R8G8B8A8_UNORM;
			createInfo.ImageType = ImageType::Type2D;
			createInfo.ImageUsage = ImageUsage::AsColorTransferAttachment;
			createInfo.Parameter.Mag = ImageFilterMode::LINEAR;
			createInfo.Parameter.Min = ImageFilterMode::LINEAR;
			createInfo.Parameter.U = ImageAddressMode::REPEAT;
			createInfo.Parameter.V = ImageAddressMode::REPEAT;
			createInfo.Parameter.W = ImageAddressMode::REPEAT;
			createInfo.GenerateSampler = true;
			createInfo.ImGuiUsage = true;

			s_CheckerBoardTextureHandle = device->CreateImage("Assets/Textures/Checkerboard.png", createInfo);
		});
	}

	void DetailsPanel::Render() {
		LUCY_PROFILE_NEW_EVENT("DetailsPanel::Render");
		
		ImGui::Begin("Details", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus);

		Entity& entityContext = SceneExplorerPanel::GetInstance().GetEntityContext();
		if (!entityContext.IsValid()) {
			ImGui::End();
			return;
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Name");
		ImGui::SameLine();

		TagComponent& tagComponent = entityContext.GetComponent<TagComponent>();
		char buffer[512];
		std::string tag = tagComponent.GetTag();
		strncpy_s(buffer, tag.c_str(), sizeof(buffer));
		ImGui::InputText("##hidelabel EntityTagLabel", buffer, sizeof(buffer));
		tagComponent.SetTag(buffer);

		ImGui::SameLine(0, 20);
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponentPopup", ImGuiPopupFlags_AnyPopup);

		if (ImGui::BeginPopup("AddComponentPopup")) {
			if (ImGui::BeginMenu("Mesh")) {
				if (ImGui::MenuItem("Mesh Renderer")) {
					entityContext.AddComponent<MeshComponent>();
				}
				if (ImGui::MenuItem("Script")) {
					//later
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Light")) {
				if (ImGui::MenuItem("Directional Light")) {
					entityContext.AddComponent<DirectionalLightComponent>();
				}
				if (ImGui::MenuItem("Cubemap")) {
					entityContext.AddComponent<HDRCubemapComponent>();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Camera")) {
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Physics")) {
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("UI")) {
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Audio")) {
				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		//Components
		DrawComponentPanel<TransformComponent>(entityContext, [&](TransformComponent& t) {
			if (ImGui::CollapsingHeader("Transforms", ImGuiTreeNodeFlags_DefaultOpen)) {

				static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoHostExtendX;

				if (ImGui::BeginTable("Transform Table", 2, flags)) {

					glm::vec3& pos = t.GetPosition();
					glm::vec3& rot = t.GetRotation();
					glm::vec3& scale = t.GetScale();

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Translation");
					ImGui::SameLine();
					ImGui::TableSetColumnIndex(1);
					UI::TransformControl("Translation Control", pos.x, pos.y, pos.z, 0.0f, 0.1f);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Rotation");
					ImGui::SameLine();
					ImGui::TableSetColumnIndex(1);
					UI::TransformControl("Rotation Control", rot.x, rot.y, rot.z, 0.0f, 0.1f);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Scale");
					ImGui::SameLine();
					ImGui::TableSetColumnIndex(1);
					UI::TransformControl("Scale Control", scale.x, scale.y, scale.z, 1.0f, 0.1f);

					t.CalculateMatrix();

					ImGui::EndTable();
				}
			}
		});

		DrawComponentPanel<DirectionalLightComponent>(entityContext, [&](DirectionalLightComponent& lightComponent) {
			if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
				const auto& dir = entityContext.GetComponent<TransformComponent>().GetRotation();
				auto& color = lightComponent.GetColor();
				//TODO: the color should be the material color
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::DragFloat3("##hidelabel color", (float*)&color, 0.01f, 0.0f, 100.0f, nullptr, 1.0f);

				lightComponent.GetDirection() = -glm::normalize(Maths::EulerDegreesToLightDirection(dir));
			}
		});

		DrawComponentPanel<MeshComponent>(entityContext, [&](MeshComponent& c) {
			Ref<Mesh> mesh = c.GetMesh();

			if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {

				char buf[1024];
				memset(buf, 0, sizeof(buf));

				if (mesh) {
					const std::string& path = mesh->GetPath();
					strncpy_s(buf, path.c_str(), sizeof(buf));
				}

				ImGui::AlignTextToFramePadding();
				ImGui::Text("Path");
				ImGui::SameLine();
				if (ImGui::InputText("##hideLabel MeshPath", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
					c.LoadMesh(buf);
					entityContext.GetComponent<TagComponent>().SetTag(c.GetMesh()->GetName());
				}

				ImGui::SameLine(0, 20);

				if (ImGui::Button("L")) {
					std::string outPath;
					Utils::OpenDialog(outPath, Utils::MeshFilterList, 1, "Assets/");
					if (!outPath.empty()) {
						c.LoadMesh(outPath);
						entityContext.GetComponent<TagComponent>().SetTag(c.GetMesh()->GetName());
					}
				}
			}

			if (mesh) {
				const auto& materialManager = Renderer::GetMaterialManager();

				if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
					static int32_t selectedMaterial = -1;

					for (uint32_t i = 0; i < mesh->GetSubmeshes().size(); i++) {
						ImGui::PushID(i);

						const Submesh& submesh = mesh->GetSubmeshes()[i];
						auto materialID = submesh.MaterialID;
						auto material = materialManager->GetMaterialByID(materialID)->As<PBRMaterial>();

						void* textureID = nullptr;
						if (material->HasImage(PBRMaterial::ALBEDO_TYPE))
							textureID = material->GetImage(PBRMaterial::ALBEDO_TYPE)->GetImGuiID();
						else
							textureID = Renderer::AccessResource<Image>(s_CheckerBoardTextureHandle)->GetImGuiID();

						if (textureID) //could be that the checker board texture isnt ready
							ImGui::ImageButton("", (ImTextureID)textureID, {64.0f, 64.0f}, {0.0f, 0.0f}, {1.0f, 1.0f});
						ImGui::SameLine();

						if (ImGui::BeginCombo("##hideLabel combo", std::to_string(materialID).c_str())) {
							for (uint32_t j = 0; j < mesh->GetSubmeshes().size(); j++) {
								Submesh& submeshInner = mesh->GetSubmeshes()[i];

								ImGui::PushID(j);
								Ref<Material> comboMaterial = materialManager->GetMaterialByID(submeshInner.MaterialID);
								if (ImGui::Selectable(std::to_string(j).c_str())) {
									selectedMaterial = j;
									submeshInner.MaterialID = j;
								}
								if (selectedMaterial == j)
									ImGui::SetItemDefaultFocus();
								ImGui::PopID();
							}
							ImGui::EndCombo();
						}

						float& roughness = material->GetRoughnessValue();
						float& metallic = material->GetMetallicValue();
						float& ao = material->GetAOContribution();

						ImGui::Text("Roughness");
						ImGui::SameLine();
						ImGui::DragFloat("##hidelabel roughness", &roughness, 0.001f, 0.0f, 1.0f, nullptr, 1.0f);
						ImGui::Text("Metallic");
						ImGui::SameLine();
						ImGui::DragFloat("##hidelabel metallic", &metallic, 0.001f, 0.0f, 1.0f, nullptr, 1.0f);
						ImGui::Text("AO");
						ImGui::SameLine();
						ImGui::DragFloat("##hidelabel ao", &ao, 0.001f, 0.0f, 1.0f, nullptr, 1.0f);

						ImGui::PopID();
					}
				}
				ImGui::Unindent();
			}
		});

		DrawComponentPanel<HDRCubemapComponent>(entityContext, [](HDRCubemapComponent& component) {
			auto cubemapImage = component.GetCubemapImage();

			if (ImGui::CollapsingHeader("Cubemap", ImGuiTreeNodeFlags_DefaultOpen)) {
				char buf[1024];
				memset(buf, 0, sizeof(buf));

				if (cubemapImage) {
					const std::string& path = cubemapImage->GetPath().string();
					strncpy_s(buf, path.c_str(), sizeof(buf));
				}

				ImGui::Text("Path");
				ImGui::SameLine();
				if (ImGui::InputText("##hideLabel CubemapPath", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
					component.LoadCubemap(buf);
				}
				ImGui::SameLine(0, 20);

				if (ImGui::Button("L")) {
					std::string outPath;
					Utils::OpenDialog(outPath, Utils::CubemapFilterList, 1, "Assets/");
					if (!outPath.empty()) {
						component.LoadCubemap(outPath);
					}
				}

				if (ImGui::RadioButton("Primary", component.IsPrimary))
					component.IsPrimary = !component.IsPrimary;
			}
		});

		static bool demoOpen = false;
		if (ImGui::RadioButton("Demo Window", demoOpen)) demoOpen = !demoOpen;
		if (demoOpen) ImGui::ShowDemoWindow();

		ImGui::End();
	}

	void DetailsPanel::OnDestroy() {
		Renderer::EnqueueResourceDestroy(s_CheckerBoardTextureHandle);
	}
}