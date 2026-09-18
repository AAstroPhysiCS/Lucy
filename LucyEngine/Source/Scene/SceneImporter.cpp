#include "lypch.h"
#include "SceneImporter.h"

#include "Core/FileSystem.h"
#include "Core/Application.h"
#include "Core/Timer.h"

namespace Lucy {

	//constexpr static inline uint32_t ASSIMP_FLAGS = aiProcess_FlipUVs | aiProcessPreset_TargetRealtime_Quality;

	constexpr static uint32_t ASSIMP_FLAGS = aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals |
		//aiProcess_FixInfacingNormals |
		aiProcess_FlipUVs |
		aiProcess_LimitBoneWeights |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_ValidateDataStructure |
		aiProcess_Triangulate |
		//aiProcess_PreTransformVertices | (animations won't work, if you enable this)
		aiProcess_SplitLargeMeshes |
		aiProcess_OptimizeMeshes;

	glm::mat4 ConvertMatrix(const aiMatrix4x4& matrix) {
		return glm::transpose(reinterpret_cast<const glm::mat4&>(matrix));
	}

	glm::vec3 ConvertVector(const aiVector3D& vector) {
		return { vector.x, vector.y, vector.z };
	}

	glm::quat ConvertQuaternion(const aiQuaternion& quaternion) {
		return { quaternion.w, quaternion.x, quaternion.y, quaternion.z };
	}

	glm::vec3 ConvertColor(const aiColor3D& color) {
		return { color.r, color.g, color.b };
	}

	bool SceneImporter::Import(const std::filesystem::path& path) {
		ScopedTimer scopedTimer("Scene import");

		Assimp::Importer importer;
		const aiScene* sourceScene = importer.ReadFile(path.string(), ASSIMP_FLAGS);
		if (!sourceScene || (sourceScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !sourceScene->mRootNode) {
			m_Error = importer.GetErrorString();
			return false;
		}

		m_Scene.Name = sourceScene->mRootNode->mName.C_Str();

		ImportMaterials(sourceScene, m_Scene);
		ImportMeshes(sourceScene, m_Scene);
		ImportNode(sourceScene->mRootNode, m_Scene, INVALID_IMPORTED_SCENE_INDEX, glm::mat4{ 1.0f }, true);
		ImportCameras(sourceScene, m_Scene);
		ImportLights(sourceScene, m_Scene);
		ImportAnimations(sourceScene, m_Scene);

		return true;
	}

	void SceneImporter::ImportMaterials(const aiScene* sourceScene, ImportedScene& importedScene) {
		static constexpr float NOT_SET_MATERIAL_PROPERTY = -1.0f;

		importedScene.Materials.resize(sourceScene->mNumMaterials);

		for (uint32_t materialIndex = 0; materialIndex < sourceScene->mNumMaterials; materialIndex++) {
			const aiMaterial& sourceMaterial = *sourceScene->mMaterials[materialIndex];
			ImportedMaterial& importedMaterial = importedScene.Materials[materialIndex];

			aiString name;
			sourceMaterial.Get(AI_MATKEY_NAME, name);
			importedMaterial.Name = name.C_Str();

			aiColor3D baseColor{ 1.0f };

			if (sourceMaterial.Get(AI_MATKEY_BASE_COLOR, baseColor) != aiReturn_SUCCESS)
				sourceMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);

			float metallic = 0.0f;
			float roughness = 1.0f;

			bool hasMetallicFactor = sourceMaterial.Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS;
			bool hasRoughnessFactor = sourceMaterial.Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS;
			bool hasMetallicRoughnessTexture = sourceMaterial.GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS) > 0;
			bool hasRoughnessTexture = sourceMaterial.GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0;

			if (!hasMetallicFactor)
				metallic = 0.0f;

			if (!hasRoughnessFactor) {
				if (hasMetallicRoughnessTexture || hasRoughnessTexture) {
					roughness = 1.0f;
				} else {
					float shininess = 0.0f;
					if (sourceMaterial.Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS)
						roughness = glm::sqrt(2.0f / (shininess + 2.0f));
					else
						roughness = 1.0f;
				}
			}

			roughness = glm::clamp(roughness, 0.0f, 1.0f);
			metallic = glm::clamp(metallic, 0.0f, 1.0f);

			aiColor3D emissiveColor{ 0.0f };
			sourceMaterial.Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);

			int32_t doubleSided = 0;
			sourceMaterial.Get(AI_MATKEY_TWOSIDED, doubleSided);

			float specularFactor = 1.0f;
			sourceMaterial.Get(AI_MATKEY_SPECULAR_FACTOR, specularFactor);

			aiColor3D specularColor{ 1.0f };
			sourceMaterial.Get(AI_MATKEY_COLOR_SPECULAR, specularColor);

			float clearcoatFactor = 0.0f;
			sourceMaterial.Get(AI_MATKEY_CLEARCOAT_FACTOR, clearcoatFactor);

			float clearcoatRoughness = 0.0f;
			sourceMaterial.Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, clearcoatRoughness);

			specularFactor = glm::clamp(specularFactor, 0.0f, 1.0f);
			clearcoatFactor = glm::clamp(clearcoatFactor, 0.0f, 1.0f);
			clearcoatRoughness = glm::clamp(clearcoatRoughness, 0.0f, 1.0f);

			const auto GetTexturePath = [](const aiMaterial& material, aiTextureType textureType) -> std::string {
				aiString path;
				if (material.GetTexture(textureType, 0, &path) != aiReturn_SUCCESS || path.length == 0)
					return {};
				return FileSystem::DecodeURI(std::string(path.C_Str()));
			};

			importedMaterial.BaseColor = { baseColor.r, baseColor.g, baseColor.b, 1.0f };
			importedMaterial.Roughness = glm::clamp(roughness, 0.0f, 1.0f);
			importedMaterial.Metallic = glm::clamp(metallic, 0.0f, 1.0f);
			importedMaterial.EmissiveColor = ConvertColor(emissiveColor);
			importedMaterial.DoubleSided = doubleSided != 0;
			importedMaterial.SpecularColor = { specularColor.r, specularColor.g, specularColor.b };
			importedMaterial.SpecularFactor = specularFactor;
			importedMaterial.ClearcoatFactor = clearcoatFactor;
			importedMaterial.ClearcoatRoughness = clearcoatRoughness;
			importedMaterial.Textures.Albedo = GetTexturePath(sourceMaterial, aiTextureType_BASE_COLOR);
			importedMaterial.Textures.Diffuse = GetTexturePath(sourceMaterial, aiTextureType_DIFFUSE);
			importedMaterial.Textures.Normal = GetTexturePath(sourceMaterial, aiTextureType_NORMALS);
			importedMaterial.Textures.NormalCamera = GetTexturePath(sourceMaterial, aiTextureType_NORMAL_CAMERA);
			importedMaterial.Textures.ORM = GetTexturePath(sourceMaterial, aiTextureType_GLTF_METALLIC_ROUGHNESS);
			importedMaterial.Textures.AO = GetTexturePath(sourceMaterial, aiTextureType_AMBIENT_OCCLUSION);
			importedMaterial.Textures.Roughness = GetTexturePath(sourceMaterial, aiTextureType_DIFFUSE_ROUGHNESS);
			importedMaterial.Textures.Metallic = GetTexturePath(sourceMaterial, aiTextureType_METALNESS);
			importedMaterial.Textures.Specular = GetTexturePath(sourceMaterial, aiTextureType_SPECULAR);
			importedMaterial.Textures.Emissive = GetTexturePath(sourceMaterial, aiTextureType_EMISSIVE);
		}
	}

	void SceneImporter::ImportMeshes(const aiScene* sourceScene, ImportedScene& importedScene) {
		const auto& taskScheduler = Application::GetTaskScheduler();

		uint32_t meshCount = sourceScene->mNumMeshes;
		importedScene.Meshes.resize(meshCount);

		taskScheduler->ScheduleBatch(TaskScheduler::Launch::Async, TaskPriority::High, [sourceScene, &importedScene](const TaskArgs& args, const TaskBatchArgs&) {
			uint32_t meshIndex = static_cast<uint32_t>(args.TaskIndex);
			const aiMesh* sourceMesh = sourceScene->mMeshes[meshIndex];
			ImportedMesh& importedMesh = importedScene.Meshes[meshIndex];

			importedMesh.Name = sourceMesh->mName.C_Str();
			importedMesh.MaterialIndex = sourceMesh->mMaterialIndex;

			importedMesh.Vertices.resize(sourceMesh->mNumVertices);
			importedMesh.Indices.resize(sourceMesh->mNumFaces * 3);

			aiVector3D* positions = sourceMesh->HasPositions() ? sourceMesh->mVertices : nullptr;
			aiVector3D* normals = sourceMesh->HasNormals() ? sourceMesh->mNormals : nullptr;
			aiVector3D* textureCoords = sourceMesh->HasTextureCoords(0) ? sourceMesh->mTextureCoords[0] : nullptr;

			bool hasTangents = sourceMesh->HasTangentsAndBitangents();

			aiVector3D* tangents = hasTangents ? sourceMesh->mTangents : nullptr;
			aiVector3D* bitangents = hasTangents ? sourceMesh->mBitangents : nullptr;

			for (uint32_t vertexIndex = 0; vertexIndex < importedMesh.Vertices.size(); vertexIndex++) {
				Vertex& vertex = importedMesh.Vertices[vertexIndex];
				if (positions) {
					const aiVector3D& position = positions[vertexIndex];
					vertex.Position = { position.x, position.y, position.z };
				}
				if (normals) {
					const aiVector3D& normal = normals[vertexIndex];
					vertex.Normal = { normal.x, normal.y, normal.z };
				}
				if (textureCoords) {
					const aiVector3D& textureCoordinate = textureCoords[vertexIndex];
					vertex.TexCoords = { textureCoordinate.x, textureCoordinate.y };
				}
				if (hasTangents) {
					const aiVector3D& tangent = tangents[vertexIndex];
					const aiVector3D& bitangent = bitangents[vertexIndex];

					vertex.Tangent = { tangent.x, tangent.y, tangent.z };
					vertex.Bitangent = { bitangent.x, bitangent.y, bitangent.z };
				}
			}

			uint32_t* destination = importedMesh.Indices.data();

			for (uint32_t faceIndex = 0; faceIndex < sourceMesh->mNumFaces; faceIndex++) {
				const aiFace& face = sourceMesh->mFaces[faceIndex];
				destination[0] = face.mIndices[0];
				destination[1] = face.mIndices[1];
				destination[2] = face.mIndices[2];

				destination += 3;
			}
		}, meshCount, 1);

		taskScheduler->WaitForAllTasks();
	}

	void SceneImporter::ImportNode(const aiNode* sourceNode, ImportedScene& importedScene, uint32_t parentIndex, const glm::mat4& parentTransform, bool importMeshInstances) {
		size_t nodeIndex = importedScene.Nodes.size();

		ImportedSceneNode node;
		node.Name = sourceNode->mName.C_Str();
		node.ParentIndex = parentIndex;
		node.LocalTransform = ConvertMatrix(sourceNode->mTransformation);
		node.WorldTransform = parentTransform * node.LocalTransform;
		node.Meshes.reserve(sourceNode->mNumMeshes);
		for (uint32_t meshIndex = 0; meshIndex < sourceNode->mNumMeshes; meshIndex++)
			node.Meshes.emplace_back(sourceNode->mMeshes[meshIndex]);

		importedScene.Nodes.emplace_back(std::move(node));

		if (parentIndex != INVALID_IMPORTED_SCENE_INDEX)
			importedScene.Nodes[parentIndex].Children.emplace_back(nodeIndex);

		if (importMeshInstances) {
			for (uint32_t meshIndex : importedScene.Nodes[nodeIndex].Meshes) {
				ImportedMeshInstance instance;
				instance.MeshIndex = meshIndex;
				instance.NodeIndex = nodeIndex;
				instance.Transform = importedScene.Nodes[nodeIndex].WorldTransform;

				importedScene.MeshInstances.emplace_back(std::move(instance));
			}
		}

		for (uint32_t childIndex = 0; childIndex < sourceNode->mNumChildren; childIndex++) {
			ImportNode(sourceNode->mChildren[childIndex], importedScene, nodeIndex, importedScene.Nodes[nodeIndex].WorldTransform, importMeshInstances);
		}
	}

	void SceneImporter::ImportCameras(const aiScene* sourceScene, ImportedScene& importedScene) {
		importedScene.Cameras.reserve(sourceScene->mNumCameras);

		for (uint32_t cameraIndex = 0; cameraIndex < sourceScene->mNumCameras; cameraIndex++) {
			const aiCamera* sourceCamera = sourceScene->mCameras[cameraIndex];
			uint32_t nodeIndex = importedScene.FindNodeIndex(sourceCamera->mName.C_Str());

			if (nodeIndex == INVALID_IMPORTED_SCENE_INDEX)
				continue;

			const glm::mat4& worldTransform = importedScene.Nodes[nodeIndex].WorldTransform;

			glm::vec3 position = { worldTransform * glm::vec4(ConvertVector(sourceCamera->mPosition), 1.0f) };
			glm::vec3 forward = { worldTransform * glm::vec4(ConvertVector(sourceCamera->mLookAt), 0.0f) };
			glm::vec3 up = { worldTransform * glm::vec4(ConvertVector(sourceCamera->mUp), 0.0f) };

			forward = glm::normalize(forward);
			up = glm::normalize(up);

			float hFov = sourceCamera->mHorizontalFOV * 2.0f;
			float vFov = 2.0f * glm::atan(glm::tan(hFov * 0.5f) / sourceCamera->mAspect);

			importedScene.Cameras.emplace_back(ImportedCamera{
				.Name = sourceCamera->mName.C_Str(),
				.NodeIndex = nodeIndex,
				.Position = position,
				.Orientation = glm::quatLookAt(forward, up),
				.HorizontalFOV = hFov,
				.VerticalFOV = vFov,
				.AspectRatio = sourceCamera->mAspect,
				.NearPlane = sourceCamera->mClipPlaneNear,
				.FarPlane = sourceCamera->mClipPlaneFar,
				.OrthographicWidth = sourceCamera->mOrthographicWidth,
			});
		}
	}

	void SceneImporter::ImportLights(const aiScene* sourceScene, ImportedScene& importedScene) {
		importedScene.Lights.reserve(sourceScene->mNumLights);

		for (uint32_t lightIndex = 0; lightIndex < sourceScene->mNumLights; lightIndex++) {
			const aiLight& sourceLight = *sourceScene->mLights[lightIndex];
			uint32_t nodeIndex = importedScene.FindNodeIndex(sourceLight.mName.C_Str());

			if (nodeIndex == INVALID_IMPORTED_SCENE_INDEX)
				continue;

			ImportedLightType lightType = ImportedLightType::Undefined;

			switch (sourceLight.mType) {
				case aiLightSource_DIRECTIONAL:
					lightType = ImportedLightType::Directional;
					break;
				case aiLightSource_POINT:
					lightType = ImportedLightType::Point;
					break;
				case aiLightSource_SPOT:
					lightType = ImportedLightType::Spot;
					break;
				case aiLightSource_AMBIENT:
					lightType = ImportedLightType::Ambient;
					break;
				case aiLightSource_AREA:
					lightType = ImportedLightType::Area;
					break;
				default:
					LUCY_ASSERT(false, "Unsupported light type: {0}", static_cast<int32_t>(sourceLight.mType));
					break;
			}

			const glm::mat4& worldTransform = importedScene.Nodes[nodeIndex].WorldTransform;

			glm::vec3 position = { worldTransform * glm::vec4(ConvertVector(sourceLight.mPosition), 1.0f) };
			glm::vec3 direction = { worldTransform * glm::vec4(ConvertVector(sourceLight.mDirection), 0.0f) };
			glm::vec3 up = { worldTransform * glm::vec4(ConvertVector(sourceLight.mUp), 0.0f) };

			direction = glm::normalize(direction);
			up = glm::normalize(up);

			float range = 0.0f;

			const aiNode* sourceNode = sourceScene->mRootNode->FindNode(sourceLight.mName);
			if (sourceNode && sourceNode->mMetaData)
				sourceNode->mMetaData->Get("PBR_LightRange", range);

			importedScene.Lights.emplace_back(ImportedLight{
				.Name = sourceLight.mName.C_Str(),
				.Type = lightType,
				.NodeIndex = nodeIndex,
				.Position = position,
				.Direction = direction,
				.Up = up,
				.Color = ConvertColor(sourceLight.mColorDiffuse),
				.Size = { sourceLight.mSize.x, sourceLight.mSize.y },
				.AttenuationConstant = sourceLight.mAttenuationConstant,
				.AttenuationLinear = sourceLight.mAttenuationLinear,
				.AttenuationQuadratic = sourceLight.mAttenuationQuadratic,
				.Range = range,
				.InnerConeAngle = sourceLight.mAngleInnerCone,
				.OuterConeAngle = sourceLight.mAngleOuterCone,
			});
		}
	}

	void SceneImporter::ImportAnimations(const aiScene* sourceScene, ImportedScene& importedScene) {
		importedScene.Animations.reserve(sourceScene->mNumAnimations);

		for (uint32_t animationIndex = 0; animationIndex < sourceScene->mNumAnimations; animationIndex++) {
			aiAnimation* sourceAnimation = sourceScene->mAnimations[animationIndex];

			ImportedAnimation animation;
			animation.Name = sourceAnimation->mName.C_Str();
			animation.Duration = sourceAnimation->mDuration;
			animation.TicksPerSecond = sourceAnimation->mTicksPerSecond;
			animation.Channels.reserve(sourceAnimation->mNumChannels);

			for (uint32_t channelIndex = 0; channelIndex < sourceAnimation->mNumChannels; channelIndex++) {
				const aiNodeAnim& sourceChannel = *sourceAnimation->mChannels[channelIndex];
				uint32_t nodeIndex = importedScene.FindNodeIndex(sourceChannel.mNodeName.C_Str());
				if (nodeIndex == INVALID_IMPORTED_SCENE_INDEX)
					continue;

				ImportedAnimationChannel channel;
				channel.NodeIndex = nodeIndex;
				channel.PositionKeys.reserve(sourceChannel.mNumPositionKeys);
				channel.RotationKeys.reserve(sourceChannel.mNumRotationKeys);
				channel.ScaleKeys.reserve(sourceChannel.mNumScalingKeys);

				for (uint32_t keyIndex = 0; keyIndex < sourceChannel.mNumPositionKeys; keyIndex++) {
					const aiVectorKey& key = sourceChannel.mPositionKeys[keyIndex];

					channel.PositionKeys.emplace_back(ImportedVectorKey{
						.Time = key.mTime,
						.Value = ConvertVector(key.mValue)
					});
				}

				for (uint32_t keyIndex = 0; keyIndex < sourceChannel.mNumRotationKeys; keyIndex++) {
					const aiQuatKey& key = sourceChannel.mRotationKeys[keyIndex];

					channel.RotationKeys.emplace_back(ImportedQuaternionKey{
						.Time = key.mTime,
						.Value = ConvertQuaternion(key.mValue)
					});
				}

				for (uint32_t keyIndex = 0; keyIndex < sourceChannel.mNumScalingKeys; keyIndex++) {
					const aiVectorKey& key = sourceChannel.mScalingKeys[keyIndex];

					channel.ScaleKeys.emplace_back(ImportedVectorKey{
						.Time = key.mTime,
						.Value = ConvertVector(key.mValue)
					});
				}

				animation.Channels.emplace_back(std::move(channel));
			}
			importedScene.Animations.emplace_back(std::move(animation));
		}
	}

	uint32_t ImportedScene::FindNodeIndex(std::string_view name) const {
		for (uint32_t nodeIndex = 0; nodeIndex < Nodes.size(); nodeIndex++) {
			if (Nodes[nodeIndex].Name == name)
				return nodeIndex;
		}
		return INVALID_IMPORTED_SCENE_INDEX;
	}

	const ImportedSceneNode& ImportedScene::FindNode(std::string_view name) const {
		uint32_t nodeIndex = FindNodeIndex(name);
		if (nodeIndex == INVALID_IMPORTED_SCENE_INDEX) {
			LUCY_ASSERT(false, "Node with name {0} not found", name);
			return {};
		}
		return Nodes[nodeIndex];
	}

	const ImportedCamera& ImportedScene::FindCamera(std::string_view name) const {
		for (const ImportedCamera& camera : Cameras) {
			if (camera.Name == name)
				return camera;
		}

		LUCY_ASSERT(false, "Camera with name {0} not found", name);
		return {};
	}

	const ImportedLight& ImportedScene::FindLight(std::string_view name) const {
		for (const ImportedLight& light : Lights) {
			if (light.Name == name)
				return light;
		}

		LUCY_ASSERT(false, "Light with name {0} not found", name);
		return {};
	}

	bool ImportedScene::IsDescendantOf(uint32_t nodeIndex, uint32_t parentIndex) const {
		uint32_t currentNodeIndex = nodeIndex;
		while (currentNodeIndex != INVALID_IMPORTED_SCENE_INDEX) {
			if (currentNodeIndex == parentIndex)
				return true;
			currentNodeIndex = Nodes[currentNodeIndex].ParentIndex;
		}
		return false;
	}
}