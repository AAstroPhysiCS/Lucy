#include "lypch.h"
#include "ShaderReflect.h"
#include "Shader.h"

namespace Lucy {

	//----For debugging purposes only----
	static void DebugInfo(ProgramLayout* layout) {
		Slang::ComPtr<ISlangBlob> jblob;
		layout->toJson(jblob.writeRef());
		std::string_view view = { (const char*)jblob->getBufferPointer(), jblob->getBufferSize() };

		Slang::ComPtr<ISlangBlob> jsonBlob;
		layout->toJson(jsonBlob.writeRef());

		std::string_view jsonView(
			static_cast<const char*>(jsonBlob->getBufferPointer()),
			jsonBlob->getBufferSize()
		);

		LUCY_INFO("Program Layout:\n{}", jsonView);

		// Additionally, print entry point details
		for (uint32_t i = 0; i < layout->getEntryPointCount(); i++) {
			auto entryPoint = layout->getEntryPointByIndex(i);
			LUCY_INFO("Entry Point {}: Name={}, Stage={}", i, entryPoint->getName(), ShaderStageToShaderString(SlangStageToShaderStage(entryPoint->getStage())));
		}
	}
	
	void ShaderReflect::Info(const std::filesystem::path& path, const Slang::ComPtr<IComponentType>& linkedProgram, ShaderStageType stageFlag, std::string_view entryPointName) {
		ProgramLayout* layout = linkedProgram->getLayout(0);

		DebugInfo(layout);

		EntryPointReflection* entryPointRef = nullptr;
		for (uint32_t i = 0; i < layout->getEntryPointCount(); i++) {
			EntryPointReflection* currentEntryPoint = layout->getEntryPointByIndex(i);
			if (entryPointName == currentEntryPoint->getName()) {
				entryPointRef = currentEntryPoint;
				break;
			}
		}

		LUCY_ASSERT(entryPointRef, "Failed to find reflected entry point '{0}' in shader: {1}", entryPointName, path.string());

		const auto UnwrapArrayType = [](TypeReflection* type) {
			while (type && type->getKind() == TypeReflection::Kind::Array) {
				type = type->getElementType();
			}
			return type;
		};

		if (stageFlag == ShaderStageType::Vertex) {
			for (uint32_t k = 0; k < layout->getParameterCount(); k++) {
				auto parameters = entryPointRef->getParameterByIndex(k);
				auto type = parameters->getTypeLayout();

				for (uint32_t l = 0; l < type->getFieldCount(); l++) {
					auto field = type->getFieldByIndex(l);
					auto fieldType = field->getType();

					VertexShaderLayoutElement element;
					element.Name = field->getName();
					element.Location = field->getBindingIndex();
					element.Type = SlangScalarTypeToShaderMemberType(fieldType->getElementType()->getScalarType());
					if (element.Type == ShaderMemberType::Unknown) {
						element.Type = SlangScalarTypeToShaderMemberType(fieldType->getScalarType());
					}
					element.ShaderDataSize = ShaderMemberTypeToSize(element.Type);
					element.ElementCount = fieldType->getElementCount();

					m_VertexShaderLayout.push_back(element);
				}
			}
		}

		for (uint32_t j = 0; j < layout->getParameterCount(); j++) {
			auto parameter = layout->getParameterByIndex(j);
			const char* name = parameter->getName();
			uint32_t bindingIndex = parameter->getBindingIndex();
			uint32_t setIndex = parameter->getBindingSpace();

			bool isPushConstant = parameter->getCategory() == ParameterCategory::PushConstantBuffer;
			bool isParameterBlock = parameter->getCategory() == ParameterCategory::SubElementRegisterSpace;

			TypeLayoutReflection* typeLayRef = parameter->getTypeLayout();
			VariableLayoutReflection* elementVariableLayout = typeLayRef->getElementVarLayout();
			TypeLayoutReflection* elementTypeLayout = elementVariableLayout ? elementVariableLayout->getTypeLayout() : nullptr;

			bool isSampler = UnwrapArrayType(typeLayRef->getType())->getKind() == TypeReflection::Kind::SamplerState;

			ShaderVariable variable;
			variable.Name = name;
			variable.Binding = bindingIndex;
			variable.StageFlag = ShaderStageToVkShaderStage(stageFlag);
			variable.BufferSize = elementTypeLayout ? elementTypeLayout->getSize(ParameterCategory::Uniform) : typeLayRef->getSize(ParameterCategory::Uniform);
			variable.ArraySize = typeLayRef->getElementCount();

			variable.Type = ConvertSlangResourceShapeToDescriptorBlockType(parameter->getType()->getResourceShape(), parameter->getType()->getResourceAccess());

			if (variable.Type == UndefinedDescriptorType)
				variable.Type = ConvertSlangKindToDescriptorBlockType(typeLayRef->getKind());

			if (variable.Type == UndefinedDescriptorType) {
				TypeLayoutReflection* layoutWithMostInfo = elementTypeLayout ? elementTypeLayout : typeLayRef;
				variable.Type = ConvertSlangResourceShapeToDescriptorBlockType(layoutWithMostInfo->getResourceShape(), layoutWithMostInfo->getResourceAccess());
			}

			if (isPushConstant)
				variable.Type = { DescriptorBaseShape::PushConstant, false };

			if (isParameterBlock)
				variable.Type = ConvertSlangResourceShapeToDescriptorBlockType(parameter->getType()->getElementType()->getResourceShape(), parameter->getType()->getElementType()->getResourceAccess());

			if (isSampler)
				variable.Type = { DescriptorBaseShape::Sampler, false };

			switch (variable.Type.Shape) {
				case DescriptorBaseShape::ConstantBuffer: {
					m_ShaderStageInfo.ConstantBufferCount++;
					break;
				}
				case DescriptorBaseShape::RWSharedStorageBuffer:
				case DescriptorBaseShape::SharedStorageBuffer: {
					m_ShaderStageInfo.StorageBufferCount++;
					break;
				}
				case DescriptorBaseShape::Sampler: {
					m_ShaderStageInfo.SamplerCount++;
					break;
				}
				case DescriptorBaseShape::PushConstant: {
					m_ShaderStageInfo.PushConstantCount++;
					break;
				}
				case DescriptorBaseShape::SampledImage:
				case DescriptorBaseShape::SampledImageArray:
				case DescriptorBaseShape::Texture2D:
				case DescriptorBaseShape::Texture2DArray:
				case DescriptorBaseShape::TextureCube:
				case DescriptorBaseShape::TextureCubeArray:
				case DescriptorBaseShape::Texture3D: {
					m_ShaderStageInfo.SampledImagesCount++;
					break;
				}
				case DescriptorBaseShape::RWTexture2D:
				case DescriptorBaseShape::RWTexture2DArray:
				case DescriptorBaseShape::RWTexture3D: {
					m_ShaderStageInfo.StorageImageCount++;
					break;
				}
				case DescriptorBaseShape::AccelerationStructure: {
					m_ShaderStageInfo.AccelerationStructureCount++;
					break;
				}
				default: {
					LUCY_CRITICAL("Unsupported descriptor shape in shader stage info: {0}", variable.Name);
					break;
				}
			}

			variable.Layout = ParseShaderVariableLayout(parameter, variable.DeviceAddressMembers);

			if (typeLayRef->getKind() == TypeReflection::Kind::Array && typeLayRef->getElementCount() == 0)
				variable.DynamicallyAllocated = true;

			if (isPushConstant) {
				if (CheckIfAlreadyPresent(variable, m_ShaderPushConstants))
					continue;
				m_ShaderPushConstants.push_back(variable);
				continue;
			}

			if (!m_ShaderVariableMap.contains(setIndex)) {
				std::vector<ShaderVariable> buffer;
				buffer.push_back(variable);
				m_ShaderVariableMap.emplace(setIndex, buffer);
			} else {
				const auto& it = m_ShaderVariableMap.find(setIndex);
				if (CheckIfAlreadyPresent(variable, it->second))
					continue;
				it->second.push_back(variable);
			}
		}
	}

	ShaderBlockLayoutElement ShaderReflect::ParseShaderVariableLayout(VariableLayoutReflection* variable, std::vector<ShaderDeviceAddressMember>& addressMembers) {
		TypeLayoutReflection* typeLayout = variable->getTypeLayout();
		VariableLayoutReflection* elementVariableLayout = typeLayout->getElementVarLayout();
		TypeLayoutReflection* elementTypeLayout = elementVariableLayout ? elementVariableLayout->getTypeLayout() : nullptr;

		ShaderBlockLayoutElement el;
		el.Name = variable->getName();

		const auto ParseField = [this, &addressMembers](VariableLayoutReflection* variable, const auto& SelfFunc) -> ShaderMemberVariable {
			TypeLayoutReflection* type = variable->getTypeLayout();
			TypeLayoutReflection* elementTypeLayout = type->getElementTypeLayout();

			ShaderMemberVariable memberVar;
			memberVar.Name = variable->getName();
			memberVar.Type = SlangScalarTypeToShaderMemberType(type->getScalarType());
			memberVar.Offset = variable->getOffset();

			if (type->getKind() == TypeReflection::Kind::Pointer) {
				memberVar.Type = ShaderMemberType::DeviceAddress;
				memberVar.Size = type->getSize();
				LUCY_ASSERT(memberVar.Size == sizeof(RenderDeviceBufferReference), "Unexpected device-address size reflected by Slang.");
			}

			if (type->getKind() == TypeReflection::Kind::Pointer) {
				addressMembers.push_back({
					.Name = elementTypeLayout->getName(),
					.Offset = memberVar.Offset,
					.Size = elementTypeLayout->getSize()
				});
				return memberVar;
			}

			if (memberVar.Type == ShaderMemberType::Unknown && elementTypeLayout)
				memberVar.Type = SlangScalarTypeToShaderMemberType(elementTypeLayout->getScalarType());

			if (elementTypeLayout)
				memberVar.Size = elementTypeLayout->getSize() * ShaderMemberTypeToSize(memberVar.Type);
			else
				memberVar.Size = type->getSize() * ShaderMemberTypeToSize(memberVar.Type);

			for (uint32_t k = 0; k < type->getFieldCount(); k++) {
				VariableLayoutReflection* variable = type->getFieldByIndex(k);
				memberVar.Children.push_back(SelfFunc(variable, SelfFunc));
			}

			return memberVar;
		};

		const auto ParseChildren = [this, ParseField, &addressMembers](auto* reflection, ShaderBlockLayoutElement& el) {
			TypeLayoutReflection* typeLayout = reflection->getTypeLayout();
			TypeLayoutReflection* elementTypeLayout = typeLayout->getElementTypeLayout();

			TypeReflection::Kind kindToCompare = TypeReflection::Kind::None;

			if (elementTypeLayout)
				kindToCompare = elementTypeLayout->getKind();
			else
				kindToCompare = typeLayout->getKind();

			if (typeLayout->getKind() == TypeReflection::Kind::Pointer) {
				addressMembers.push_back({
					.Name = elementTypeLayout->getName(),
					.Offset = reflection->getOffset(),
					.Size = elementTypeLayout->getSize()
				});
				return;
			}

			switch (SlangKindToShaderBlockType(kindToCompare)) {
				case ShaderBlockType::Struct:
				case ShaderBlockType::ParameterBlock:
				case ShaderBlockType::Array:
					el.Children.push_back(ParseShaderVariableLayout(reflection, addressMembers));
					break;
				default:
					//work with "type" not "typelayout"
					//means we are dealing with a variable that is not a struct or array, but a basic type e.g. "u_ShadowMap"
					el.Members.push_back(ParseField(reflection, ParseField));
					break;
			}
		};

		const auto GetContainedTypeLayout = [](TypeLayoutReflection* typeLayout) {
			switch (typeLayout->getKind()) {
				case TypeReflection::Kind::ConstantBuffer:
				case TypeReflection::Kind::ParameterBlock:
				case TypeReflection::Kind::ShaderStorageBuffer: {
					VariableLayoutReflection* elementVariableLayout = typeLayout->getElementVarLayout();
					if (elementVariableLayout)
						return elementVariableLayout->getTypeLayout();
					break;
				}
				default:
					break;
			}

			return typeLayout;
		};

		TypeLayoutReflection* layoutWithMostInfo = GetContainedTypeLayout(typeLayout);
		el.BufferSize = layoutWithMostInfo->getSize(ParameterCategory::Uniform);
		el.Type = SlangKindToShaderBlockType(layoutWithMostInfo->getKind());
		el.Offset = variable->getOffset(ParameterCategory::Uniform);

		for (uint32_t k = 0; k < layoutWithMostInfo->getFieldCount(); k++) {
			VariableLayoutReflection* child = layoutWithMostInfo->getFieldByIndex(k);
			ParseChildren(child, el);
		}
		
		return el;
	}

	//if there are multiple occurences between the 2 shader stages, dont add a another one but combine them together
	bool ShaderReflect::CheckIfAlreadyPresent(const ShaderVariable& variable, std::vector<ShaderVariable>& buffer) {
		auto result = std::find_if(buffer.begin(), buffer.end(), [&variable](const ShaderVariable& element) {
			return variable.Name == element.Name;
		});

		if (result != buffer.end()) {
			result->StageFlag |= variable.StageFlag;

			if (variable.BufferSize > result->BufferSize) {
				result->BufferSize = variable.BufferSize;
				result->Layout = variable.Layout;
			}

			return true;
		}
		return false;
	}

	void ShaderReflect::DestroyCachedData() {
		m_ShaderPushConstants.clear();
		m_ShaderVariableMap.clear();
		m_VertexShaderLayout.clear();
		m_ShaderStageInfo = {};
	}
}
