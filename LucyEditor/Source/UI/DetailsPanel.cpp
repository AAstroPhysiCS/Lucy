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
		createInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
		createInfo.Target = ImageTarget::Color;
		createInfo.ImageType = ImageType::Type2D;
		createInfo.Parameter.Mag = VK_FILTER_LINEAR;
		createInfo.Parameter.Min = VK_FILTER_LINEAR;
		createInfo.Parameter.U = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.Parameter.V = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.Parameter.W = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.GenerateSampler = true;
		createInfo.ImGuiUsage = true;

		s_CheckerBoardTexture = Image2D::Create("Assets/Textures/Checkerboard.png", createInfo);
	}

	void DetailsPanel::Render() {
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
		std::strncpy(buffer, tag.c_str(), sizeof(buffer));
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

		DrawComponentPanel<MeshComponent>(entityContext, [&](MeshComponent& c) {
			const Ref<Mesh> mesh = c.GetMesh();

			if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {

				char buf[1024];
				memset(buf, 0, sizeof(buf));

				if (mesh) {
					std::string& path = mesh->GetPath();
					std::strncpy(buf, path.c_str(), sizeof(buf));
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
						Submesh& submesh = mesh->GetSubmeshes()[i];
						Ref<Material> m = mesh->GetMaterials()[submesh.MaterialIndex];

						void* textureID = 0;

						if (m->HasImage(Material::ALBEDO_TYPE))
							textureID = m->GetImage(Material::ALBEDO_TYPE)->GetImGuiID();
						else
							textureID = s_CheckerBoardTexture->GetImGuiID();

						ImGui::ImageButton((ImTextureID)textureID, { 64, 64 }, { 0, 0 }, { 1, 1 }, 0.0f);
						ImGui::SameLine();

						if (ImGui::BeginCombo(fmt::format("##hideLabel {0}", i).c_str(), m->GetName().c_str())) {
							for (uint32_t j = 0; j < mesh->GetSubmeshes().size(); j++) {
								Ref<Material> comboMaterial = mesh->GetMaterials()[j];
								if (ImGui::Selectable(comboMaterial->GetName().c_str())) {
									selectedMaterial = j;
									submesh.MaterialIndex = j;
								}
								if (selectedMaterial == j)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
					}
				}
				ImGui::Unindent();
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