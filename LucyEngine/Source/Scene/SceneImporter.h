#pragma once

#include "assimp/Importer.hpp"
#include "assimp/material.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "Renderer/Mesh.h"

namespace Lucy {

	constexpr uint32_t INVALID_IMPORTED_SCENE_INDEX = std::numeric_limits<uint32_t>::max();

	struct ImportedMaterialTextures {
		std::string Albedo;
		std::string Diffuse;
		std::string Normal;
		std::string NormalCamera;
		std::string ORM;
		std::string AO;
		std::string Roughness;
		std::string Metallic;
		std::string Specular;
		std::string Emissive;
	};

	struct ImportedMaterial {
		std::string Name;

		glm::vec4 BaseColor = glm::vec4{ 1.0f };
		glm::vec3 EmissiveColor = glm::vec3{ 0.0f };

		float AO = 1.0f;
		float Roughness = 0.8f;
		float Metallic = 0.0f;
		float EmissiveStrength = 0.0f;
		float NormalStrength = 1.0f;

		bool DoubleSided = false;

		ImportedMaterialTextures Textures;
	};

	struct ImportedMesh {
		std::string Name;

		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;

		uint32_t MaterialIndex = INVALID_IMPORTED_SCENE_INDEX;
	};

	struct ImportedSceneNode {
		std::string Name;

		uint32_t ParentIndex = INVALID_IMPORTED_SCENE_INDEX;

		std::vector<uint32_t> Children;
		std::vector<uint32_t> Meshes;

		glm::mat4 LocalTransform = glm::mat4{ 1.0f };
		glm::mat4 WorldTransform = glm::mat4{ 1.0f };
	};

	struct ImportedMeshInstance {
		uint32_t MeshIndex = INVALID_IMPORTED_SCENE_INDEX;
		uint32_t NodeIndex = INVALID_IMPORTED_SCENE_INDEX;

		glm::mat4 Transform = glm::mat4{ 1.0f };
	};

	enum class ImportedLightType : uint8_t {
		Undefined,
		Directional,
		Point,
		Spot,
		Ambient,
		Area
	};

	struct ImportedLight {
		std::string Name;

		ImportedLightType Type = ImportedLightType::Undefined;

		uint32_t NodeIndex = INVALID_IMPORTED_SCENE_INDEX;

		glm::vec3 Position = glm::vec3{ 0.0f };
		glm::vec3 Direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
		glm::vec3 Up = glm::vec3{ 0.0f, 1.0f, 0.0f };
		glm::vec3 Color = glm::vec3{ 1.0f };

		glm::vec2 Size = glm::vec2{ 0.0f };

		float AttenuationConstant = 1.0f;
		float AttenuationLinear = 0.0f;
		float AttenuationQuadratic = 0.0f;

		float Range = 0.0f;

		float InnerConeAngle = 0.0f;
		float OuterConeAngle = 0.0f;
	};

	struct ImportedCamera {
		std::string Name;

		uint32_t NodeIndex = INVALID_IMPORTED_SCENE_INDEX;

		glm::vec3 Position = glm::vec3{ 0.0f };
		glm::quat Orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };

		float HorizontalFOV = 0.0f;
		float VerticalFOV = 0.0f;
		float AspectRatio = 0.0f;

		float NearPlane = 0.01f;
		float FarPlane = 1000.0f;

		float OrthographicWidth = 0.0f;
	};

	struct ImportedVectorKey {
		double Time = 0.0;
		glm::vec3 Value = glm::vec3{ 0.0f };
	};

	struct ImportedQuaternionKey {
		double Time = 0.0;
		glm::quat Value = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
	};

	struct ImportedAnimationChannel {
		uint32_t NodeIndex = INVALID_IMPORTED_SCENE_INDEX;

		std::vector<ImportedVectorKey> PositionKeys;
		std::vector<ImportedQuaternionKey> RotationKeys;
		std::vector<ImportedVectorKey> ScaleKeys;
	};

	struct ImportedAnimation {
		std::string Name;

		double Duration = 0.0;
		double TicksPerSecond = 0.0;

		std::vector<ImportedAnimationChannel> Channels;
	};

	struct ImportedScene {
		std::string Name;

		std::vector<ImportedSceneNode> Nodes;

		std::vector<ImportedMaterial> Materials;

		std::vector<ImportedMesh> Meshes;
		std::vector<ImportedMeshInstance> MeshInstances;

		std::vector<ImportedCamera> Cameras;
		std::vector<ImportedLight> Lights;

		std::vector<ImportedAnimation> Animations;

		uint32_t FindNodeIndex(std::string_view name) const;

		const ImportedSceneNode& FindNode(std::string_view name) const;
		const ImportedCamera& FindCamera(std::string_view name) const;
		const ImportedLight& FindLight(std::string_view name) const;

		bool IsDescendantOf(uint32_t nodeIndex, uint32_t parentIndex) const;
	};

	class SceneImporter final {
	public:
		SceneImporter() = default;
		~SceneImporter() = default;

		SceneImporter(const SceneImporter&) = delete;
		SceneImporter& operator=(const SceneImporter&) = delete;
		SceneImporter(SceneImporter&&) = delete;
		SceneImporter& operator=(SceneImporter&&) = delete;

		bool Import(const std::filesystem::path& path);

		ImportedScene& GetScene() { return m_Scene; }
		ImportedScene&& TakeScene() { return std::move(m_Scene); }

		const std::string& GetError() const { return m_Error; }
	private:
		void ImportMaterials(const aiScene* sourceScene, ImportedScene& importedScene);
		void ImportMeshes(const aiScene* sourceScene, ImportedScene& importedScene);
		void ImportNode(const aiNode* sourceNode, ImportedScene& importedScene, uint32_t parentIndex, const glm::mat4& parentTransform, bool importMeshInstances);
		void ImportCameras(const aiScene* sourceScene, ImportedScene& importedScene);
		void ImportLights(const aiScene* sourceScene, ImportedScene& importedScene);
		void ImportAnimations(const aiScene* sourceScene, ImportedScene& importedScene);

		ImportedScene m_Scene;
		std::string m_Error;
	};
}
