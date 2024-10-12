#pragma once

#include "RenderCommand.h"

#include "CommandPool.h"
#include "Renderer/Memory/Buffer/Buffer.h"

namespace Lucy {

	class Image;
	class Shader;

	class RenderCommandList final {
	public:
		RenderCommandList(const Ref<RenderDevice>& renderDevice);
		~RenderCommandList() = default;

		void SetImageLayout(Ref<Image> image, uint32_t newLayout, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount);
		void CopyImageToImage(Ref<Image> srcImage, Ref<Image> destImage, const std::vector<VkImageCopy>& regions);
		void CopyBufferToImage(Ref<ByteBuffer> srcBuffer, Ref<Image> destImage);
		void CopyImageToBuffer(Ref<Image> srcImage, Ref<ByteBuffer> destBuffer);

		RenderCommand& BeginRenderCommand(const std::string& nameOfDraw);
		void EndRenderCommand() const;

		inline Ref<CommandPool> GetPrimaryCommandPool() const { return m_PrimaryCommandPool; }
	private:
		void Recreate();
		void Destroy();
		
		std::unordered_map<std::string, RenderCommand> m_RenderCommands;

		Ref<RenderDevice> m_RenderDevice = nullptr;
		Ref<CommandPool> m_PrimaryCommandPool = nullptr;
		//Ref<CommandPool> m_SecondaryCommandPool = nullptr;

		friend class CommandQueue; //for Destroy/Recreate
	};
}