#include "LucyResource.slang"

struct CullData {
    GPUVisibleObject* VisibleObjects;
    uint* VisibleObjectCount;

    GPUVisibleSubmesh* VisibleSubmeshes;
    uint* VisibleSubmeshCount;

    GPUDispatchIndirectCommand* SubmeshDispatchIndirect;
    GPUDispatchIndirectCommand* MeshletDispatchIndirect;

    GPUVisibleDraw* VisibleDraws;
    GPUIndexedIndirectCommand* IndirectCommands;
    uint* DrawCounts;

    uint ObjectCapacity;
    uint SubmeshCapacity;
    uint CommandCapacityPerBin;
    uint ViewIndex;
    uint RenderBinCount;
};

[[vk::push_constant]]
PushConstantGlobal<CullData> PushConstants;

// how much a object appears on screen... we are dealing here with perspective projection, so we can obtain
// the approximate projected radius by getting the Pyy and multiply that by Viewport.y pixels / 2

bool IsConeBackFacing(GPUMeshlet meshlet, float4x4 modelMatrix, float4x4 normalMatrix, float3 worldCenter, float worldRadius, float3 cameraPosition) {
    const float coneCutoff = meshlet.NormalCone.w;

    if (coneCutoff >= 1.0f)
        return false;

    const float3 coneAxis = normalize(mul(float3x3(normalMatrix), meshlet.NormalCone.xyz));

    const float3 centerToCamera = worldCenter - cameraPosition;
    return dot(centerToCamera, coneAxis) >= coneCutoff * length(centerToCamera) + worldRadius;
}

bool IsHiZOccluded(float3 worldCenter, float worldRadius, GPUCullView view) {
    const uint hiZTextureIndex = view.Data.x;
    const uint hiZSamplerIndex = view.Data.y;
    const uint hiZMipCount = view.Data.z;
    const uint viewFlags = view.Data.w;

    if ((viewFlags & VIEW_FLAG_OCCLUSION) == 0)
        return false;
    if (hiZTextureIndex == INVALID_BINDLESS_INDEX || hiZSamplerIndex == INVALID_BINDLESS_INDEX || hiZMipCount == 0)
        return false;

    float4 viewCenter = view.View * float4(worldCenter, 1.0f);

    if (viewCenter.z + worldRadius >= -0.0001f)
        return false;

    float4 clipCenter = view.Projection * viewCenter;
    float2 centerNDC = clipCenter.xy / clipCenter.w;

    float distanceToCamera = max(abs(viewCenter.z), 0.0001f);

    float2 radiusNDC = float2(abs(view.Projection[0][0]), abs(view.Projection[1][1])) * worldRadius / distanceToCamera;

    float2 centerUV = centerNDC * 0.5f + 0.5f;
    float2 radiusUV = radiusNDC * 0.5f;

    float2 minUV = clamp(centerUV - radiusUV, 0.0f, 1.0f);
    float2 maxUV = clamp(centerUV + radiusUV, 0.0f, 1.0f);

    float2 rectanglePixels = (maxUV - minUV) * view.Viewport.xy;

    float maximumExtent = max(rectanglePixels.x, rectanglePixels.y);
    float mip = clamp(floor(log2(max(maximumExtent, 1.0f))), 0.0f, float(hiZMipCount - 1u));

    Texture2D<float> hiZ = BindlessTable::Textures2D_Float[NonUniformResourceIndex(hiZTextureIndex)];
    SamplerState sampler = BindlessTable::Samplers[NonUniformResourceIndex(hiZSamplerIndex)];

    const float depth0 = hiZ.SampleLevel(sampler, float2(minUV.x, minUV.y), mip);
    const float depth1 = hiZ.SampleLevel(sampler, float2(maxUV.x, minUV.y), mip);
    const float depth2 = hiZ.SampleLevel(sampler, float2(minUV.x, maxUV.y), mip);
    const float depth3 = hiZ.SampleLevel(sampler, float2(maxUV.x, maxUV.y), mip);

    float hierarchyDepth = min(min(depth0, depth1), min(depth2, depth3));
    float closestViewZ = viewCenter.z + worldRadius;

    float4 closestClip = view.Projection * float4(0.0f, 0.0f, closestViewZ, 1.0f);
    float closestDepth = closestClip.z / closestClip.w;

    return closestDepth < hierarchyDepth - 0.0005f;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void CullObjects(uint3 dispatchID : SV_DispatchThreadID) {
    const uint objectIndex = dispatchID.x;
    if (objectIndex >= PushConstants.Data.ObjectCapacity)
        return;

    GPUObject object = PushConstants.Root->Addresses.Objects[objectIndex];
    uint objectFlags = object.Data.y;
    uint meshIndex = object.Data.x;

    bool visible = (objectFlags & OBJECT_FLAG_ALIVE) != 0;
    visible = visible && meshIndex != INVALID_BINDLESS_INDEX;

    GPUMesh mesh;
    float3 worldCenter = 0.0f;
    float worldRadius = 0.0f;

    if (visible) {
        mesh = PushConstants.Root->Addresses.Meshes[meshIndex];

        worldCenter = worldCenter = mul(object.Transform, float4(mesh.BoundingSphere.xyz, 1.0f)).xyz;
        worldRadius = mesh.BoundingSphere.w * GetMaximumScale(object.Transform);
        
        //printf("World Center: (%f, %f, %f), World Radius: %f\n", worldCenter.x, worldCenter.y, worldCenter.z, worldRadius);

        GPUCullView view = PushConstants.Root->Addresses.CullViews[PushConstants.Data.ViewIndex];
        if ((view.Data.w & VIEW_FLAG_FRUSTUM) != 0) {
            visible = IsSphereInsideFrustum(worldCenter, worldRadius, view);
        }
    }

    const uint waveVisibleCount = WaveActiveCountBits(visible);
    uint waveOutputBase = 0;

    if (WaveIsFirstLane() && waveVisibleCount > 0) {
        InterlockedAdd(PushConstants.Data.VisibleObjectCount[0], waveVisibleCount, waveOutputBase);
    }

    waveOutputBase = WaveReadLaneFirst(waveOutputBase);

    if (!visible)
        return;

    const uint waveOffset = WavePrefixCountBits(visible);
    const uint outputIndex = waveOutputBase + waveOffset;

    GPUVisibleObject output;
    output.ObjectIndex = objectIndex;

    PushConstants.Data.VisibleObjects[outputIndex] = output;
}

[shader("compute")]
[numthreads(1, 1, 1)]
void BuildSubmeshDispatch() {
    const uint visibleObjectCount = PushConstants.Data.VisibleObjectCount[0];

    GPUDispatchIndirectCommand command;
    command.GroupCountX = visibleObjectCount;
    command.GroupCountY = 1;
    command.GroupCountZ = 1;

    PushConstants.Data.SubmeshDispatchIndirect[0] = command;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void CullSubmeshes(uint3 groupID: SV_GroupID, uint3 groupThreadID: SV_GroupThreadID) {
    const uint visibleObjectIndex = groupID.x;
    const uint visibleObjectCount = PushConstants.Data.VisibleObjectCount[0];

    if (visibleObjectIndex >= visibleObjectCount)
        return;

    GPUVisibleObject visibleObject = PushConstants.Data.VisibleObjects[visibleObjectIndex];
    GPUObject object = PushConstants.Root->Addresses.Objects[visibleObject.ObjectIndex];
    GPUMesh mesh = PushConstants.Root->Addresses.Meshes[object.Data.x];
    GPUCullView view = PushConstants.Root->Addresses.CullViews[PushConstants.Data.ViewIndex];

    for (uint submeshOffset = groupThreadID.x; submeshOffset < mesh.Data.w; submeshOffset += 64) {
        uint submeshIndex = mesh.Data.z + submeshOffset;

        GPUSubmesh submesh = PushConstants.Root->Addresses.Submeshes[submeshIndex];
        float4x4 modelMatrix = mul(object.Transform, submesh.Transform);

        float worldScale = GetMaximumScale(modelMatrix);
        float3 worldCenter = mul(modelMatrix, float4(submesh.BoundingSphere.xyz, 1.0f)).xyz;
        float worldRadius = submesh.BoundingSphere.w * worldScale;

        bool visible = true;

        if ((view.Data.w & VIEW_FLAG_FRUSTUM) != 0)
            visible = IsSphereInsideFrustum(worldCenter, worldRadius, view);

        uint lodIndex = INVALID_BINDLESS_INDEX;

        if (visible) {
            lodIndex = SelectLOD(submesh, worldCenter, worldScale, view, object.Data.y, PushConstants.Root->Addresses.MeshLODs);
            visible = lodIndex != INVALID_BINDLESS_INDEX;
        }

        uint waveVisibleCount = WaveActiveCountBits(visible);
        uint waveOutputBase = 0;

        if (WaveIsFirstLane() && waveVisibleCount > 0)
            InterlockedAdd(PushConstants.Data.VisibleSubmeshCount[0], waveVisibleCount, waveOutputBase);

        waveOutputBase = WaveReadLaneFirst(waveOutputBase);

        if (!visible)
            continue;

        uint waveOffset = WavePrefixCountBits(visible);
        uint outputIndex = waveOutputBase + waveOffset;

        if (outputIndex >= PushConstants.Data.SubmeshCapacity)
            continue;

        GPUVisibleSubmesh output;
        output.ObjectIndex = visibleObject.ObjectIndex;
        output.SubmeshIndex = submeshIndex;
        output.LODIndex = lodIndex;

        PushConstants.Data.VisibleSubmeshes[outputIndex] = output;
    }
}

[shader("compute")]
[numthreads(1, 1, 1)]
void BuildMeshletDispatch() {
    const uint visibleSubmeshCount = PushConstants.Data.VisibleSubmeshCount[0];

    GPUDispatchIndirectCommand command;
    command.GroupCountX = visibleSubmeshCount;
    command.GroupCountY = 1;
    command.GroupCountZ = 1;

    PushConstants.Data.MeshletDispatchIndirect[0] = command;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void CullMeshlets(uint3 groupID: SV_GroupID, uint3 groupThreadID: SV_GroupThreadID) {
    const uint visibleSubmeshIndex = groupID.x;
    const uint visibleSubmeshCount = PushConstants.Data.VisibleSubmeshCount[0];

    if (visibleSubmeshIndex >= visibleSubmeshCount)
        return;

    GPUVisibleSubmesh visibleSubmesh = PushConstants.Data.VisibleSubmeshes[visibleSubmeshIndex];
    GPUObject object = PushConstants.Root->Addresses.Objects[visibleSubmesh.ObjectIndex];
    GPUSubmesh submesh = PushConstants.Root->Addresses.Submeshes[visibleSubmesh.SubmeshIndex];
    GPUMeshLOD lod = PushConstants.Root->Addresses.MeshLODs[visibleSubmesh.LODIndex];
    GPUCullView view = PushConstants.Root->Addresses.CullViews[PushConstants.Data.ViewIndex];

    float4x4 modelMatrix = mul(object.Transform, submesh.Transform);
    float4x4 normalMatrix = mul(object.TransformInversedTransposed, submesh.TransformInversedTransposed);

    float worldScale = GetMaximumScale(modelMatrix);
    uint renderBin = min(submesh.Meshlets.z, PushConstants.Data.RenderBinCount - 1);

    for (uint localMeshletIndex = groupThreadID.x; localMeshletIndex < lod.Meshlets.y; localMeshletIndex += 64) {
        uint meshletIndex = lod.Meshlets.x + localMeshletIndex;
        GPUMeshlet meshlet = PushConstants.Root->Addresses.Meshlets[meshletIndex];

        float3 worldCenter = mul(modelMatrix, float4(meshlet.BoundingSphere.xyz, 1.0f)).xyz;
        float worldRadius = meshlet.BoundingSphere.w * worldScale;

        float3 aabbCenter = mul(modelMatrix, float4(meshlet.AABBCenter.xyz, 1.0f)).xyz;
        float3x3 linearTransform = float3x3(modelMatrix);
        float3 aabbExtents = mul(abs(float3x3(modelMatrix)), meshlet.AABBExtents.xyz);

        bool visible = true;

        if ((view.Data.w & VIEW_FLAG_FRUSTUM) != 0)
            visible = IsAABBInsideFrustum(aabbCenter, aabbExtents, view);

        if (visible && (view.Data.w & VIEW_FLAG_CONE) != 0)
            visible = !IsConeBackFacing(meshlet, modelMatrix, normalMatrix, worldCenter, worldRadius, view.CameraPosition.xyz);

        if (visible && (view.Data.w & VIEW_FLAG_OCCLUSION) != 0)
            visible = !IsHiZOccluded(worldCenter, worldRadius, view);

        uint renderBin = min(submesh.Meshlets.z, PushConstants.Data.RenderBinCount - 1);
        uint waveVisibleCount = WaveActiveCountBits(visible);

        uint waveBinBase = 0;

        if (WaveIsFirstLane() && waveVisibleCount > 0)
            InterlockedAdd(PushConstants.Data.DrawCounts[renderBin], waveVisibleCount, waveBinBase);

        waveBinBase = WaveReadLaneFirst(waveBinBase);

        if (!visible)
            continue;

        uint waveOffset = WavePrefixCountBits(visible);
        uint binLocalIndex = waveBinBase + waveOffset;

        if (binLocalIndex >= PushConstants.Data.CommandCapacityPerBin)
            continue;

        uint outputIndex = renderBin * PushConstants.Data.CommandCapacityPerBin + binLocalIndex;

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