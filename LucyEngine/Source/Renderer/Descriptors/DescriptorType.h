#pragma once

#include "slang/slang.h"

namespace Lucy {

	enum class DescriptorBaseShape {
		Undefined,
		Array,
		SharedStorageBuffer,
		ConstantBuffer,
		PushConstant,
		Sampler,
		Texture2D,
		Texture2DArray,
		TextureCube,
		TextureCubeArray,
		Texture3D,
		SampledImage,
		SampledImageArray,
		RWTexture2D,
		RWTexture2DArray,
		RWTexture3D,
		RWSharedStorageBuffer,
		UniformTexelBuffer,
		StorageTexelBuffer,
		InputAttachment,
		AccelerationStructure,
		RayTracingScene,
	};

	struct DescriptorType {
		DescriptorBaseShape Shape = DescriptorBaseShape::Undefined;
		bool isDynamic = false;

		inline bool operator==(const DescriptorType& other) const { return Shape == other.Shape && isDynamic == other.isDynamic; }
	};

	static constexpr DescriptorType UndefinedDescriptorType = { DescriptorBaseShape::Undefined, false };

	//DescriptorType ConvertDescriptorType(uint32_t type);
	uint32_t ConvertDescriptorType(DescriptorType type);

	DescriptorType ConvertSlangKindToDescriptorBlockType(slang::TypeReflection::Kind kind);
	DescriptorType ConvertSlangResourceShapeToDescriptorBlockType(SlangResourceShape shape, SlangResourceAccess access);
}