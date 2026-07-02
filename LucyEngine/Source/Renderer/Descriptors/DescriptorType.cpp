#include "lypch.h"
#include "DescriptorType.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	uint32_t ConvertDescriptorType(DescriptorType type) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan) {
			LUCY_ASSERT(false);
			return -1;
		}

		switch (type.Shape) {
			// Basic sampler (separate from textures)
			case DescriptorBaseShape::Sampler:
				return VK_DESCRIPTOR_TYPE_SAMPLER;
			// Combined image+sampler types (typical for sampled textures)
			case DescriptorBaseShape::Texture2D:
			case DescriptorBaseShape::Texture2DArray:
			case DescriptorBaseShape::TextureCube:
			case DescriptorBaseShape::Texture3D:
				return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			// Read-only texture without embedded sampler (used with separate sampler)
			case DescriptorBaseShape::SampledImage:
			case DescriptorBaseShape::SampledImageArray:
				return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			// Storage images (read/write)
			case DescriptorBaseShape::RWTexture2D:
			case DescriptorBaseShape::RWTexture2DArray:
			case DescriptorBaseShape::RWTexture3D:
				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			// Uniform buffers
			case DescriptorBaseShape::UniformBuffer:
				return type.isDynamic
					? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
					: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			// Storage buffers (read/write)
			case DescriptorBaseShape::SharedStorageBuffer:
			case DescriptorBaseShape::RWSharedStorageBuffer:
				return type.isDynamic
					? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
					: VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			// Formatted buffer views
			case DescriptorBaseShape::UniformTexelBuffer:
				return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			case DescriptorBaseShape::StorageTexelBuffer:
				return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
			// Input attachments (framebuffer inputs)
			case DescriptorBaseShape::InputAttachment:
				return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			// Ray tracing types
			case DescriptorBaseShape::AccelerationStructure:
				return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			case DescriptorBaseShape::RayTracingScene:
				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; // Or specialized RT type
			default:
				LUCY_ASSERT(false);
				return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}

	DescriptorType ConvertSlangKindToDescriptorBlockType(slang::TypeReflection::Kind kind) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan) {
			LUCY_ASSERT(false);
			return UndefinedDescriptorType;
		}

		switch (kind) {
			case slang::TypeReflection::Kind::SamplerState:
				return { DescriptorBaseShape::Sampler, false };
			case slang::TypeReflection::Kind::ConstantBuffer:
				return { DescriptorBaseShape::UniformBuffer, false };
			case slang::TypeReflection::Kind::ShaderStorageBuffer:
				return { DescriptorBaseShape::SharedStorageBuffer, false };
			// TextureBuffer maps to structured buffer in our system
			case slang::TypeReflection::Kind::TextureBuffer:
				return { DescriptorBaseShape::SharedStorageBuffer, false };
			// ParameterBlocks are treated as constant buffers
			case slang::TypeReflection::Kind::ParameterBlock:
				return { DescriptorBaseShape::UniformBuffer, false };
			default:
				return UndefinedDescriptorType;
		}
	}

	DescriptorType ConvertSlangResourceShapeToDescriptorBlockType(SlangResourceShape shape, SlangResourceAccess access) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan) {
			LUCY_ASSERT(false);
			return UndefinedDescriptorType;
		}

		switch (shape) {
			case SLANG_TEXTURE_1D:
			case SLANG_TEXTURE_2D:
				return access == SLANG_RESOURCE_ACCESS_READ_WRITE
					? DescriptorType{ DescriptorBaseShape::RWTexture2D, false } : DescriptorType{ DescriptorBaseShape::Texture2D, false };
			case SLANG_TEXTURE_2D_ARRAY:
				return access == SLANG_RESOURCE_ACCESS_READ_WRITE
					? DescriptorType{ DescriptorBaseShape::RWTexture2DArray, false } : DescriptorType{ DescriptorBaseShape::Texture2DArray, false };
			case SLANG_TEXTURE_CUBE:
			// Cubemaps are typically read-only
				return DescriptorType{ DescriptorBaseShape::TextureCube, false };
			case SLANG_TEXTURE_3D:
				return access == SLANG_RESOURCE_ACCESS_READ_WRITE
					? DescriptorType{ DescriptorBaseShape::RWTexture3D, false } : DescriptorType{ DescriptorBaseShape::Texture3D, false };
			case SLANG_TEXTURE_BUFFER:
			// TBuffer<t> in HLSL
				return DescriptorType{ DescriptorBaseShape::SharedStorageBuffer, false };
			case SLANG_STRUCTURED_BUFFER:
				return access == SLANG_RESOURCE_ACCESS_READ_WRITE
					? DescriptorType{ DescriptorBaseShape::RWSharedStorageBuffer, false } : DescriptorType{ DescriptorBaseShape::SharedStorageBuffer, false };
			/*
			case SLANG_BYTE_ADDRESS_BUFFER:
				return access == SLANG_RESOURCE_ACCESS_READ_WRITE
					? DescriptorType{ DescriptorBaseShape::RWByteAddressBuffer, false }
				: DescriptorType{ DescriptorBaseShape::ByteAddressBuffer, false };
			*/
			// Special types
			case SLANG_ACCELERATION_STRUCTURE:
				return DescriptorType{ DescriptorBaseShape::AccelerationStructure, false };
			default:
				return UndefinedDescriptorType;
		}
	}
}