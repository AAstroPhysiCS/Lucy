#pragma once

namespace Lucy {

	struct PipelineSpecification {
		//shaderlayout
	};

	class Pipeline
	{
	public:
		Pipeline(PipelineSpecification& specs);
	private:
		PipelineSpecification m_Specs;
	};
}

