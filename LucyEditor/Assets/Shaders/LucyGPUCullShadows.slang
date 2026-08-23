#include "LucyResource.slang"

struct GPUShadowCullData {
    GPUVisibleObject* VisibleObjects;
    uint* VisibleObjectCount;

    GPUVisibleSubmesh* VisibleSubmeshes;
    uint* VisibleSubmeshCount;

    GPUDispatchIndirectCommand* SubmeshDispatch;
    GPUDispatchIndirectCommand* MeshletDispatch;

    GPUVisibleDraw* VisibleDraws;
    GPUIndexedIndirectCommand* IndirectCommands;
    uint* DrawCount;

    uint4 ViewIndices;
    uint4 Data;
};

[[vk::push_constant]]
PushConstantGlobal<GPUShadowCullData> PushConstants;

[shader("compute")]
[numthreads(64, 1, 1)]
void CullShadowObjects(uint3 dispatchID: SV_DispatchThreadID) {
    const uint objectIndex = dispatchID.x;
    const uint objectCapacity = PushConstants.Data.Data.x;
    const uint cascadeCount = PushConstants.Data.Data.w;

    if (objectIndex >= objectCapacity)
        return;

    GPUObject object = PushConstants.Root->Addresses.Objects[objectIndex];

    const uint objectFlags = object.Data.y;
    const uint meshIndex = object.Data.x;

    bool visible = (objectFlags & OBJECT_FLAG_ALIVE) != 0;
    visible = visible && meshIndex != INVALID_BINDLESS_INDEX;

    if (visible) {
        GPUMesh mesh = PushConstants.Root->Addresses.Meshes[meshIndex];

        float3 aabbCenter = mul(object.Transform, float4(mesh.AABBCenter, 1.0f)).xyz;
        float3 aabbExtents = mul(abs(float3x3(object.Transform)), mesh.AABBExtents);

        visible = false;

        for (uint cascadeIndex = 0; cascadeIndex < cascadeCount; cascadeIndex++) {
            GPUCullView view = PushConstants.Root->Addresses.CullViews[PushConstants.Data.ViewIndices[cascadeIndex]];

            if (IsAABBInsideFrustum(aabbCenter, aabbExtents, view)) {
                visible = true;
                break;
            }
        }
    }

    const uint waveVisibleCount = WaveActiveCountBits(visible);
    uint waveBaseIndex = 0;

    if (WaveIsFirstLane() && waveVisibleCount > 0) {
        InterlockedAdd(PushConstants.Data.VisibleObjectCount[0], waveVisibleCount, waveBaseIndex);
    }

    waveBaseIndex = WaveReadLaneFirst(waveBaseIndex);

    if (!visible)
        return;

    const uint waveOffset = WavePrefixCountBits(visible);
    const uint outputIndex = waveBaseIndex + waveOffset;

    if (outputIndex >= objectCapacity)
        return;

    GPUVisibleObject visibleObject;
    visibleObject.ObjectIndex = objectIndex;

    PushConstants.Data.VisibleObjects[outputIndex] = visibleObject;
}

[shader("compute")]
[numthreads(1, 1, 1)]
void BuildShadowSubmeshDispatch() {
    GPUDispatchIndirectCommand command;

    command.GroupCountX = min(PushConstants.Data.VisibleObjectCount[0], PushConstants.Data.Data.x);
    command.GroupCountY = 1;
    command.GroupCountZ = 1;

    PushConstants.Data.SubmeshDispatch[0] = command;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void CullShadowSubmeshes(uint3 groupID: SV_GroupID, uint3 groupThreadID: SV_GroupThreadID) {
    const uint visibleObjectIndex = groupID.x;
    const uint visibleObjectCount = PushConstants.Data.VisibleObjectCount[0];

    if (visibleObjectIndex >= visibleObjectCount)
        return;

    GPUVisibleObject visibleObject = PushConstants.Data.VisibleObjects[visibleObjectIndex];
    GPUObject object = PushConstants.Root->Addresses.Objects[visibleObject.ObjectIndex];
    GPUMesh mesh = PushConstants.Root->Addresses.Meshes[object.Data.x];

    const uint cascadeCount = PushConstants.Data.Data.w;

    for (uint submeshOffset = groupThreadID.x; submeshOffset < mesh.Data.w; submeshOffset += 64) {
        const uint submeshIndex = mesh.Data.z + submeshOffset;

        GPUSubmesh submesh = PushConstants.Root->Addresses.Submeshes[submeshIndex];

        float4x4 modelMatrix = mul(object.Transform, submesh.Transform);

        float worldScale = GetMaximumScale(modelMatrix);
        // float worldRadius = submesh.BoundingSphere.w * worldScale;
        float3 worldCenter = mul(modelMatrix, float4(submesh.BoundingSphere.xyz, 1.0f)).xyz;
        float3 aabbCenter = mul(modelMatrix, float4(submesh.AABBCenter, 1.0f)).xyz;
        float3 aabbExtents = mul(abs(float3x3(modelMatrix)), submesh.AABBExtents);

        bool visible = false;

        for (uint cascadeIndex = 0; cascadeIndex < cascadeCount; cascadeIndex++) {
            GPUCullView view = PushConstants.Root->Addresses.CullViews[PushConstants.Data.ViewIndices[cascadeIndex]];

            if (IsAABBInsideFrustum(aabbCenter, aabbExtents, view)) {
                visible = true;
                break;
            }
        }

        uint lodIndex = INVALID_BINDLESS_INDEX;

        if (visible) {
            lodIndex = SelectLOD(submesh, worldCenter, worldScale, PushConstants.Root->Addresses.CullViews[PushConstants.Data.ViewIndices[0]], object.Data.y, PushConstants.Root->Addresses.MeshLODs);
            visible = lodIndex != INVALID_BINDLESS_INDEX;
        }

        const uint waveVisibleCount = WaveActiveCountBits(visible);
        uint waveBaseIndex = 0;

        if (WaveIsFirstLane() && waveVisibleCount > 0) {
            InterlockedAdd(PushConstants.Data.VisibleSubmeshCount[0], waveVisibleCount, waveBaseIndex);
        }

        waveBaseIndex = WaveReadLaneFirst(waveBaseIndex);

        if (!visible)
            continue;

        const uint waveOffset = WavePrefixCountBits(visible);
        const uint outputIndex = waveBaseIndex + waveOffset;

        if (outputIndex >= PushConstants.Data.Data.y)
            continue;

        GPUVisibleSubmesh visibleSubmesh;
        visibleSubmesh.ObjectIndex = visibleObject.ObjectIndex;
        visibleSubmesh.SubmeshIndex = submeshIndex;
        visibleSubmesh.LODIndex = lodIndex;

        PushConstants.Data.VisibleSubmeshes[outputIndex] = visibleSubmesh;
    }
}

[shader("compute")]
[numthreads(1, 1, 1)]
void BuildShadowMeshletDispatch() {
    GPUDispatchIndirectCommand command;

    command.GroupCountX = min(PushConstants.Data.VisibleSubmeshCount[0], PushConstants.Data.Data.y);
    command.GroupCountY = 1;
    command.GroupCountZ = 1;

    PushConstants.Data.MeshletDispatch[0] = command;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void CullShadowMeshlets(uint3 groupID: SV_GroupID, uint3 groupThreadID: SV_GroupThreadID) {
    const uint visibleSubmeshIndex = groupID.x;
    const uint visibleSubmeshCount = PushConstants.Data.VisibleSubmeshCount[0];

    if (visibleSubmeshIndex >= visibleSubmeshCount)
        return;

    GPUVisibleSubmesh visibleSubmesh = PushConstants.Data.VisibleSubmeshes[visibleSubmeshIndex];

    GPUObject object = PushConstants.Root->Addresses.Objects[visibleSubmesh.ObjectIndex];
    GPUSubmesh submesh = PushConstants.Root->Addresses.Submeshes[visibleSubmesh.SubmeshIndex];
    GPUMeshLOD lod = PushConstants.Root->Addresses.MeshLODs[visibleSubmesh.LODIndex];

    float4x4 modelMatrix = mul(object.Transform, submesh.Transform);

    const uint cascadeCount = PushConstants.Data.Data.w;

    for (uint localMeshletIndex = groupThreadID.x; localMeshletIndex < lod.Meshlets.y; localMeshletIndex += 64) {
        const uint meshletIndex = lod.Meshlets.x + localMeshletIndex;

        GPUMeshlet meshlet = PushConstants.Root->Addresses.Meshlets[meshletIndex];

        float3 aabbCenter = mul(modelMatrix, float4(meshlet.AABBCenter.xyz, 1.0f)).xyz;
        float3 aabbExtents = mul(abs(float3x3(modelMatrix)), meshlet.AABBExtents.xyz);

        bool visible = false;

        for (uint cascadeIndex = 0; cascadeIndex < cascadeCount; cascadeIndex++) {
            GPUCullView view = PushConstants.Root->Addresses.CullViews[PushConstants.Data.ViewIndices[cascadeIndex]];

            if (IsAABBInsideFrustum(aabbCenter, aabbExtents, view)) {
                visible = true;
                break;
            }
        }

        const uint waveVisibleCount = WaveActiveCountBits(visible);
        uint waveBaseIndex = 0;

        if (WaveIsFirstLane() && waveVisibleCount > 0) {
            InterlockedAdd(PushConstants.Data.DrawCount[0], waveVisibleCount, waveBaseIndex);
        }

        waveBaseIndex = WaveReadLaneFirst(waveBaseIndex);

        if (!visible)
            continue;

        const uint waveOffset = WavePrefixCountBits(visible);
        const uint outputIndex = waveBaseIndex + waveOffset;

        if (outputIndex >= PushConstants.Data.Data.z)
            continue;

        GPUVisibleDraw visibleDraw;
        visibleDraw.ObjectIndex = visibleSubmesh.ObjectIndex;
        visibleDraw.SubmeshIndex = visibleSubmesh.SubmeshIndex;
        visibleDraw.MeshletIndex = meshletIndex;

        PushConstants.Data.VisibleDraws[outputIndex] = visibleDraw;

        GPUIndexedIndirectCommand command;
        command.IndexCount = meshlet.Draw.y;
        command.InstanceCount = 1;
        command.FirstIndex = meshlet.Draw.x;
        command.VertexOffset = asint(meshlet.Draw.z);
        command.FirstInstance = outputIndex;

        PushConstants.Data.IndirectCommands[outputIndex] = command;
    }
}