#include "LucyResource.slang"

struct DDGIProbeSelectionData {
    uint* ProbePriorities;

    uint* ProbeUpdateList;
    uint* ProbeUpdateMask;

    uint* SelectionCounter;
    uint* LastUpdateFrames;

    // x = probe count
    // y = update budget
    // z = priority level
    // w = frame number
    uint4 SelectionData;
};

[[vk::push_constant]]
PushConstantGlobal<DDGIProbeSelectionData> PushConstants;

[numthreads(64, 1, 1)]
void DDGIProbeSelectionMain(uint3 dispatchThreadID: SV_DispatchThreadID) {
    uint candidateIndex = dispatchThreadID.x;

    uint probeCount = PushConstants.Data.SelectionData.x;

    if (candidateIndex >= probeCount)
        return;

    uint updateBudget = PushConstants.Data.SelectionData.y;
    uint priorityLevel = PushConstants.Data.SelectionData.z;
    uint frameNumber = PushConstants.Data.SelectionData.w;

    uint probeIndex = (candidateIndex + frameNumber * 977u) % probeCount;

    if (PushConstants.Data.ProbePriorities[probeIndex] != priorityLevel)
        return;

    uint updateIndex = 0;
    InterlockedAdd(PushConstants.Data.SelectionCounter[0], 1, updateIndex);

    if (updateIndex >= updateBudget)
        return;

    PushConstants.Data.ProbeUpdateList[updateIndex] = probeIndex;
    PushConstants.Data.ProbeUpdateMask[probeIndex] = 1;
    PushConstants.Data.LastUpdateFrames[probeIndex] = frameNumber;
}