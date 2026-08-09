#include "LucyResource.slang"

struct CullData {
    GPUVisibleObject* VisibleObjects;
    uint* VisibleObjectCount;

    GPUDispatchIndirectCommand* MeshletDispatchIndirect;

    GPUVisibleDraw* VisibleDraws;
    GPUIndexedIndirectCommand* IndirectCommands;
    uint* DrawCounts;

    uint ObjectCapacity;
    uint CommandCapacityPerBin;
    uint ViewIndex;
    uint RenderBinCount;
};

[[vk::push_constant]]
PushConstantGlobal<CullData> PushConstants;

// how much a object appears on screen... we are dealing here with perspective projection, so we can obtain
// the approximate projected radius by getting the Pyy and multiply that by Viewport.y pixels / 2
float GetProjectedRadiusPixels(float3 center, float radius, GPUCullView view) {
    const float projectionScale = abs(view.Projection[1][1]) * view.Viewport.y * 0.5f;

    if ((view.Data.w & VIEW_FLAG_ORTHOGRAPHIC) != 0)
        return radius * projectionScale;

    const float distanceToCamera = max(length(view.CameraPosition.xyz - center), 0.0001f);
    return radius * projectionScale / distanceToCamera;
}

uint SelectLOD(GPUSubmesh submesh, float projectedRadiusPixels, uint objectFlags) {
    const uint firstLOD = submesh.LODs.x;
    const uint lodCount = submesh.LODs.y;

    if ((objectFlags & OBJECT_FLAG_FORCE_HIGHEST_LOD) != 0)
        return firstLOD;

    if ((objectFlags & OBJECT_FLAG_FORCE_LOWEST_LOD) != 0)
        return firstLOD + lodCount - 1;

    uint selectedLOD = firstLOD + lodCount - 1;

    for (uint lodOffset = 0; lodOffset < lodCount; lodOffset++) {
        const uint lodIndex = firstLOD + lodOffset;
        const GPUMeshLOD lod = PushConstants.Root->Addresses.MeshLODs[lodIndex];

        if (projectedRadiusPixels >= lod.LODData.x) {
            selectedLOD = lodIndex;
            return selectedLOD;
        }
    }

    return selectedLOD;
}

bool IsConeBackFacing(GPUMeshlet meshlet, float4x4 modelMatrix, float4x4 normalMatrix, float3 worldCenter, float worldRadius, float3 cameraPosition) {
    const float coneCutoff = meshlet.NormalCone.w;

    if (coneCutoff >= 1.0f)
        return false;

    const float3 coneAxis = normalize(float3x3(normalMatrix) * meshlet.NormalCone.xyz);

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

    const GPUObject object = PushConstants.Root->Addresses.Objects[objectIndex];
    const uint objectFlags = object.Data.y;
    const uint meshIndex = object.Data.x;

    bool visible = (objectFlags & OBJECT_FLAG_ALIVE) != 0;
    visible = visible && meshIndex != INVALID_BINDLESS_INDEX;

    GPUMesh mesh;
    float3 worldCenter = 0.0f;
    float worldRadius = 0.0f;

    if (visible) {
        mesh = PushConstants.Root->Addresses.Meshes[meshIndex];

        worldCenter = (object.Transform * float4(mesh.BoundingSphere.xyz, 1.0f)).xyz;
        worldRadius = mesh.BoundingSphere.w * GetMaximumScale(object.Transform);
        
        printf("World Center: (%f, %f, %f), World Radius: %f\n", worldCenter.x, worldCenter.y, worldCenter.z, worldRadius);

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
void BuildMeshletDispatch() {
    const uint visibleObjectCount = PushConstants.Data.VisibleObjectCount[0];

    GPUDispatchIndirectCommand command;
    command.GroupCountX = visibleObjectCount;
    command.GroupCountY = 1;
    command.GroupCountZ = 1;

    PushConstants.Data.MeshletDispatchIndirect[0] = command;
}
[shader("compute")]
[numthreads(64, 1, 1)]
void CullMeshletsShadow(
	uint3 groupID: SV_GroupID,
	uint3 groupThreadID: SV_GroupThreadID
) {
    const uint visibleObjectIndex = groupID.x;
    const uint visibleObjectCount = PushConstants.Data.VisibleObjectCount[0];

    if (visibleObjectIndex >= visibleObjectCount)
        return;

    GPUVisibleObject visibleObject =
		PushConstants.Data.VisibleObjects[visibleObjectIndex];

    if (visibleObject.ObjectIndex >= PushConstants.Data.ObjectCapacity)
        return;

    GPUObject object =
		PushConstants.Root->Addresses.Objects[
			visibleObject.ObjectIndex
		];

    if ((object.Data.y & OBJECT_FLAG_ALIVE) == 0)
        return;

    const uint meshIndex = object.Data.x;

    if (meshIndex == INVALID_BINDLESS_INDEX)
        return;

    GPUMesh mesh =
		PushConstants.Root->Addresses.Meshes[meshIndex];

    for (
		uint submeshOffset = 0;
		submeshOffset < mesh.Data.w;
		submeshOffset++
	) {
        const uint submeshIndex =
			mesh.Data.z + submeshOffset;

        GPUSubmesh submesh =
			PushConstants.Root->Addresses.Submeshes[submeshIndex];

        // force highest / first LOD for this test
        const uint lodIndex = submesh.LODs.x;

        GPUMeshLOD lod =
			PushConstants.Root->Addresses.MeshLODs[lodIndex];

        for (
			uint localMeshletIndex = groupThreadID.x;
			localMeshletIndex < lod.Meshlets.y;
			localMeshletIndex += 64
		) {
            const uint meshletIndex =
				lod.Meshlets.x + localMeshletIndex;

            GPUMeshlet meshlet =
				PushConstants.Root->Addresses.Meshlets[meshletIndex];

            uint outputIndex = 0;

            InterlockedAdd(
				PushConstants.Data.DrawCounts[0],
				1,
				outputIndex
			);

            if (outputIndex >= PushConstants.Data.CommandCapacityPerBin)
                continue;

            GPUVisibleDraw visibleDraw;
            visibleDraw.ObjectIndex = visibleObject.ObjectIndex;
            visibleDraw.SubmeshIndex = meshlet.Draw.w;
            visibleDraw.MeshletIndex = meshletIndex;

            PushConstants.Data.VisibleDraws[outputIndex] =
				visibleDraw;

            GPUIndexedIndirectCommand command;
            command.IndexCount = meshlet.Draw.y;
            command.InstanceCount = 1;
            command.FirstIndex = meshlet.Draw.x;
            command.VertexOffset = asint(meshlet.Draw.z);
            command.FirstInstance = outputIndex;

            PushConstants.Data.IndirectCommands[outputIndex] =
				command;
        }
    }
}

[shader("compute")]
[numthreads(64, 1, 1)]
void CullMeshlets(uint3 groupID: SV_GroupID, uint3 groupThreadID: SV_GroupThreadID) {
    const uint visibleObjectIndex = groupID.x;
    const uint visibleObjectCount = PushConstants.Data.VisibleObjectCount[0];

    if (visibleObjectIndex >= visibleObjectCount)
        return;

    GPUVisibleObject visibleObject = PushConstants.Data.VisibleObjects[visibleObjectIndex];
    GPUObject object = PushConstants.Root->Addresses.Objects[visibleObject.ObjectIndex];
    GPUCullView view = PushConstants.Root->Addresses.CullViews[PushConstants.Data.ViewIndex];
    GPUMesh mesh = PushConstants.Root->Addresses.Meshes[object.Data.x];

    for (uint submeshOffset = 0; submeshOffset < mesh.Data.w; submeshOffset++) {
        const uint submeshIndex = mesh.Data.z + submeshOffset;
        GPUSubmesh submesh = PushConstants.Root->Addresses.Submeshes[submeshIndex];

        float4x4 modelMatrix = object.Transform * submesh.Transform;
        float4x4 normalMatrix = object.TransformInversedTransposed * submesh.TransformInversedTransposed;

        float3 submeshWorldCenter = (modelMatrix * float4(submesh.BoundingSphere.xyz, 1.0f)).xyz;
        float submeshWorldRadius = submesh.BoundingSphere.w * GetMaximumScale(modelMatrix);

        if ((view.Data.w & VIEW_FLAG_FRUSTUM) != 0 && !IsSphereInsideFrustum(submeshWorldCenter, submeshWorldRadius, view))
            continue;

        float projectedRadius = GetProjectedRadiusPixels(submeshWorldCenter, submeshWorldRadius, view);
        uint lodIndex = SelectLOD(submesh, projectedRadius, object.Data.y);
        GPUMeshLOD lod = PushConstants.Root->Addresses.MeshLODs[lodIndex];

        for (uint localMeshletIndex = groupThreadID.x; localMeshletIndex < lod.Meshlets.y; localMeshletIndex += 64) {
            uint meshletIndex = lod.Meshlets.x + localMeshletIndex;
            GPUMeshlet meshlet = PushConstants.Root->Addresses.Meshlets[meshletIndex];

            float3 worldCenter = (modelMatrix * float4(meshlet.BoundingSphere.xyz, 1.0f)).xyz;
            float worldRadius = meshlet.BoundingSphere.w * GetMaximumScale(modelMatrix);

            bool visible = true;

            if ((view.Data.w & VIEW_FLAG_FRUSTUM) != 0)
                visible = IsSphereInsideFrustum(worldCenter, worldRadius, view);

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
            visibleDraw.ObjectIndex = visibleObject.ObjectIndex;
            visibleDraw.SubmeshIndex = submeshIndex;
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
}