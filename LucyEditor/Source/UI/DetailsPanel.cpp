#include "DetailsPanel.h"
#include "SceneExplorerPanel.h"

#include "Utils/Utils.h"

#include "imgui.h"

namespace Lucy {

	DetailsPanel& DetailsPanel::GetInstance() {
		static DetailsPanel s_Instance;
		return s_Instance;
	}

	DetailsPanel::DetailsPanel() {
		ImageCreateInfo createInfo;
		createInfo.Format = ImageFormat::R8G8B8A8_UNORM;
		createInfo.ImageType = ImageType::Type2DColor;
		createInfo.Parameter.Mag = ImageFilterMode::LINEAR;
		createInfo.Parameter.Min = ImageFilterMode::LINEAR;
		createInfo.Parameter.U = ImageAddressMode::REPEAT;
		createInfo.Parameter.V = ImageAddressMode::REPEAT;
		createInfo.Parameter.W = ImageAddressMode::REPEAT;
		createInfo.GenerateSampler = true;
		createInfo.ImGuiUsage = true;

		s_CheckerBoardTexture = Image::Create("Assets/Textures/Checkerboard.png", createInfo);
	}

	void DetailsPanel::Render() {
		LUCY_PROFILE_NEW_EVENT("DetailsPanel::Render");
		
		ImGui::Begin("Details", 0, ImGuiWindowFlags_NoBringToFrontOnFocus);

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
					UIUtils::TransformControl("Translation Control", pos.x, pos.y, pos.z, 0.0f, 0.1f);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Rotation");
					ImGui::SameLine();
					ImGui::TableSetColumnIndex(1);
					UIUtils::TransformControl("Rotation Control", rot.x, rot.y, rot.z, 0.0f, 0.1f);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Scale");
					ImGui::SameLine();
					ImGui::TableSetColumnIndex(1);
					UIUtils::TransformControl("Scale Control", scale.x, scale.y, scale.z, 1.0f, 0.1f);

					t.CalculateMatrix();

					ImGui::EndTable();
				}
			}
		});

		DrawComponentPanel<DirectionalLightComponent>(entityContext, [&](DirectionalLightComponent& lightComponent) {
			if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
				auto& dir = entityContext.GetComponent<TransformComponent>().GetRotation();
				auto& color = lightComponent.GetColor();
				//TODO: the color should be the material color
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::DragFloat3("##hidelabel color", (float*)&color, 0.01f, 0.0f, 100.0f, nullptr, 1.0f);

				lightComponent.GetDirection() = dir;
			}
		});

		DrawComponentPanel<MeshComponent>(entityContext, [&](MeshComponent& c) {
			const Ref<Mesh> mesh = c.GetMesh();

			if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {

				char buf[1024];
				memset(buf, 0, sizeof(buf));

				if (mesh) {
					std::string& path = mesh->GetPath();
					strncpy_s(buf, path.c_str(), sizeof(buf));
				}

				ImGui::AlignTextToFramePadding();
				ImGui::Text("Path");
				ImGui::SameLine();
				if (ImGui::InputText("##hideLabel MeshPath", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
					c.SetMesh(Mesh::Create(buf));
					entityContext.GetComponent<TagComponent>().SetTag(c.GetMesh()->GetName());
				}

				ImGui::SameLine(0, 20);

				if (ImGui::Button("L")) {
					std::string outPath;
					Utils::OpenDialog(outPath, Utils::MeshFilterList, 1, "Assets/");
					if (!outPath.empty()) {
						c.SetMesh(Mesh::Create(outPath));
						entityContext.GetComponent<TagComponent>().SetTag(c.GetMesh()->GetName());
					}
				}
			}

			if (mesh) {
				if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
					static int32_t selectedMaterial = -1;

					for (uint32_t i = 0; i < mesh->GetSubmeshes().size(); i++) {
						ImGui::PushID(i);

						Submesh& submesh = mesh->GetSubmeshes()[i];
						Ref<Material> m = mesh->GetMaterials()[submesh.MaterialIndex];

						void* textureID = 0;

						if (m->HasImage(Material::ALBEDO_TYPE))
							textureID = m->GetImage(Material::ALBEDO_TYPE)->GetImGuiID();
						else
							textureID = s_CheckerBoardTexture->GetImGuiID();

						ImGui::ImageButton((ImTextureID)textureID, { 64.0f, 64.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f }, 0);
						ImGui::SameLine();

						if (ImGui::BeginCombo("##hideLabel combo", m->GetName().c_str())) {
							for (uint32_t j = 0; j < mesh->GetSubmeshes().size(); j++) {
								ImGui::PushID(j);
								Ref<Material> comboMaterial = mesh->GetMaterials()[j];
								if (ImGui::Selectable(comboMaterial->GetName().c_str())) {
									selectedMaterial = j;
									submesh.MaterialIndex = j;
								}
								if (selectedMaterial == j)
									ImGui::SetItemDefaultFocus();
								ImGui::PopID();
							}
							ImGui::EndCombo();
						}

						float& roughness = m->GetRoughnessValue();
						float& metallic = m->GetMetallicValue();
						float& ao = m->GetAOContribution();

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
			Ref<Image> cubemap = component.GetCubemapImage();
			if (ImGui::CollapsingHeader("Cubemap", ImGuiTreeNodeFlags_DefaultOpen)) {
				char buf[1024];
				memset(buf, 0, sizeof(buf));

				if (cubemap) {
					const std::string& path = cubemap->GetPath();
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
		s_CheckerBoardTexture->Destroy();
	}
}