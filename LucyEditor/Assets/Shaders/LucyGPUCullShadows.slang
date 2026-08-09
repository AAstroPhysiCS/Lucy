#include "LucyResource.slang"

struct GPUShadowCullData {
    GPUVisibleObject* VisibleObjects;
    uint* VisibleObjectCounts;
    GPUDispatchIndirectCommand* MeshletDispatches;

    uint4 ViewIndices;
    uint4 Data;
};

[[vk::push_constant]]
PushConstantGlobal<GPUShadowCullData> PushConstants;

[shader("compute")]
[numthreads(64, 1, 1)]
void CullShadowObjects(uint3 dispatchID: SV_DispatchThreadID) {
    const uint objectIndex = dispatchID.x;
    const uint cascadeIndex = dispatchID.y;

    const uint objectCapacity = PushConstants.Data.Data.x;
    const uint cascadeCount = PushConstants.Data.Data.z;

    if (objectIndex >= objectCapacity || cascadeIndex >= cascadeCount)
        return;

    GPUObject object = PushConstants.Root->Addresses.Objects[objectIndex];
    const uint objectFlags = object.Data.y;
    const uint meshIndex = object.Data.x;

    bool visible = (objectFlags & OBJECT_FLAG_ALIVE) != 0;
    visible = visible && meshIndex != INVALID_BINDLESS_INDEX;

    if (visible) {
        GPUMesh mesh = PushConstants.Root->Addresses.Meshes[object.Data.x];
        GPUCullView view = PushConstants.Root->Addresses.CullViews[PushConstants.Data.ViewIndices[cascadeIndex]];

        float3 worldCenter = (object.Transform * float4(mesh.BoundingSphere.xyz, 1.0f)).xyz;
        float worldRadius = mesh.BoundingSphere.w * GetMaximumScale(object.Transform);

        visible = IsSphereInsideFrustumShadow(worldCenter, worldRadius, view);
    }

    const uint waveVisibleCount = WaveActiveCountBits(visible);
    uint waveBaseIndex = 0;

    if (WaveIsFirstLane() && waveVisibleCount > 0) {
        InterlockedAdd(PushConstants.Data.VisibleObjectCounts[cascadeIndex], waveVisibleCount, waveBaseIndex);
    }

    waveBaseIndex = WaveReadLaneFirst(waveBaseIndex);

    const uint waveOffset = WavePrefixCountBits(visible);

    if (!visible)
        return;

    const uint outputIndex = cascadeIndex * objectCapacity + waveBaseIndex + waveOffset;

    GPUVisibleObject visibleObject;
    visibleObject.ObjectIndex = objectIndex;

    PushConstants.Data.VisibleObjects[outputIndex] = visibleObject;
}

[shader("compute")]
[numthreads(4, 1, 1)]
void BuildShadowMeshletDispatches(uint3 dispatchID: SV_DispatchThreadID) {
    const uint cascadeIndex = dispatchID.x;
    const uint cascadeCount = PushConstants.Data.Data.z;

    if (cascadeIndex >= cascadeCount)
        return;

    GPUDispatchIndirectCommand command;
    command.GroupCountX = PushConstants.Data.VisibleObjectCounts[cascadeIndex];
    command.GroupCountY = 1;
    command.GroupCountZ = 1;

    PushConstants.Data.MeshletDispatches[cascadeIndex] = command;
}
