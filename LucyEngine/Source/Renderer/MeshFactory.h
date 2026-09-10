#pragma once

#include "Mesh.h"

namespace Lucy {

	namespace MeshFactory {

		/*
		* Change these to consteval functions once cmath is fucking constexpr
		*/
		inline constexpr Unique<Mesh> CreateCube() {
			constexpr auto MakeSkyCube = []() {
				constexpr std::array<float, 108> vertices = {
					//back face
					-1.0f, -1.0f, -1.0f,
					1.0f, 1.0f, -1.0f,
					1.0f, -1.0f, -1.0f,
					1.0f, 1.0f, -1.0f,
					-1.0f, -1.0f, -1.0f,
					-1.0f, 1.0f, -1.0f,
					// front face
					-1.0f, -1.0f, 1.0f,
					1.0f, -1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					-1.0f, 1.0f, 1.0f,
					-1.0f, -1.0f, 1.0f,
					// left face
					-1.0f, 1.0f, 1.0f,
					-1.0f, 1.0f, -1.0f,
					-1.0f, -1.0f, -1.0f,
					-1.0f, -1.0f, -1.0f,
					-1.0f, -1.0f, 1.0f,
					-1.0f, 1.0f, 1.0f,
					// right face
					1.0f, 1.0f, 1.0f,
					1.0f, -1.0f, -1.0f,
					1.0f, 1.0f, -1.0f,
					1.0f, -1.0f, -1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, -1.0f, 1.0f,
					// bottom face
					-1.0f, -1.0f, -1.0f,
					1.0f, -1.0f, -1.0f,
					1.0f, -1.0f, 1.0f,
					1.0f, -1.0f, 1.0f,
					-1.0f, -1.0f, 1.0f,
					-1.0f, -1.0f, -1.0f,
					// top face
					-1.0f, 1.0f, -1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, -1.0f,
					1.0f, 1.0f, 1.0f,
					-1.0f, 1.0f, -1.0f,
					-1.0f, 1.0f, 1.0f
				};

				std::array<uint32_t, vertices.size() / 3> indices;
				for (uint32_t i = 0; i < indices.size(); i++)
					indices[i] = i;

				return std::pair{ vertices, indices };
			};

			constexpr auto skyCube = MakeSkyCube();
			constexpr auto vertices = skyCube.first;
			constexpr auto indices = skyCube.second;

			return Memory::CreateUnique<Mesh>(vertices, indices);
		}

		inline constexpr Unique<Mesh> CreateSphere(uint32_t latitudeSegments, uint32_t longitudeSegments) {
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			vertices.reserve((latitudeSegments + 1) * (longitudeSegments + 1));
			indices.reserve(latitudeSegments * longitudeSegments * 6);

			for (float latitude = 0; latitude <= latitudeSegments; latitude++) {
				float v = latitude / static_cast<float>(latitudeSegments);
				float theta = v * glm::pi<float>();

				float sinTheta = glm::sin(theta);
				float cosTheta = glm::cos(theta);

				for (float longitude = 0; longitude <= longitudeSegments; longitude++) {
					float u = longitude / static_cast<float>(longitudeSegments);
					float phi = u * glm::two_pi<float>();

					float sinPhi = glm::sin(phi);
					float cosPhi = glm::cos(phi);

					glm::vec3 position{
						sinTheta * cosPhi,
						cosTheta,
						sinTheta * sinPhi
					};

					Vertex& vertex = vertices.emplace_back();
					vertex.Position = position;
					vertex.Normal = position;
					vertex.TexCoords = { u, v };
				}
			}

			uint32_t rowSize = longitudeSegments + 1;

			for (uint32_t latitude = 0; latitude < latitudeSegments; latitude++) {
				for (uint32_t longitude = 0; longitude < longitudeSegments; longitude++) {
					uint32_t i0 = latitude * rowSize + longitude;
					uint32_t i1 = i0 + 1;
					uint32_t i2 = i0 + rowSize;
					uint32_t i3 = i2 + 1;

					indices.push_back(i0);
					indices.push_back(i1);
					indices.push_back(i2);

					indices.push_back(i1);
					indices.push_back(i3);
					indices.push_back(i2);
				}
			}

			return Memory::CreateUnique<Mesh>(std::move(vertices), std::move(indices));
		}
	}
}