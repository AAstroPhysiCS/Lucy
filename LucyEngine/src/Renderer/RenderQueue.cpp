#include "lypch.h"
#include "RenderQueue.h"

namespace Lucy {

	//add threading, i did this way, so that i can support multithreading easier
	//this function in my mind should contain or "record" the state of the render command.
	//if it's finished, it should signal it to the thread 
	//after all threads have finished, the main thread should be called and presented
	RenderCommand RenderCommand::BeginNew() {
		if (s_CurrentRenderCommand) {
			LUCY_CRITICAL("Forgot to call RenderCommandEnd?");
			LUCY_ASSERT(false);
		}
		RenderCommand cmd;
		s_CurrentRenderCommand = &cmd;
		return cmd;
	}

	void RenderCommand::End() {
		if (!s_CurrentRenderCommand) {
			LUCY_CRITICAL("Calling RenderCommandEnd without beginning it!");
			LUCY_ASSERT(false);
		}
		s_CurrentRenderCommand = nullptr;
	}

	void RenderCommandQueue::SubmitToQueue(const RenderCommand& cmd) {
		m_CommandQueue.push_back(cmd);
	}

	void RenderCommandQueue::Clear() {
		m_CommandQueue.clear();
	}
}