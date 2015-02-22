// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. 

#include "voxel1.h"
//#include "GeneratedMeshComponentPluginPrivatePCH.h"
#include "GeneratedMeshComponent.h"
#include "DynamicMeshBuilder.h"

/** Vertex Buffer */
class FGeneratedMeshVertexBuffer : public FVertexBuffer 
{
public:
    TArray<FDynamicMeshVertex> Vertices;

    virtual void InitRHI() override
    {
        FRHIResourceCreateInfo CreateInfo;
        VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FDynamicMeshVertex),BUF_Static,CreateInfo);

        // Copy the vertex data into the vertex buffer.
        void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI,0,Vertices.Num() * sizeof(FDynamicMeshVertex), RLM_WriteOnly);
        FMemory::Memcpy(VertexBufferData,Vertices.GetData(),Vertices.Num() * sizeof(FDynamicMeshVertex));
        RHIUnlockVertexBuffer(VertexBufferRHI);
    }

};

/** Index Buffer */
class FGeneratedMeshIndexBuffer : public FIndexBuffer 
{
public:
    TArray<int32> Indices;

    virtual void InitRHI() override
    {
        FRHIResourceCreateInfo CreateInfo;
        IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32),Indices.Num() * sizeof(int32),BUF_Static, CreateInfo);

        // Write the indices to the index buffer.
        void* Buffer = RHILockIndexBuffer(IndexBufferRHI,0,Indices.Num() * sizeof(int32),RLM_WriteOnly);
        FMemory::Memcpy(Buffer,Indices.GetData(),Indices.Num() * sizeof(int32));
        RHIUnlockIndexBuffer(IndexBufferRHI);
    }
};

/** Vertex Factory */
class FGeneratedMeshVertexFactory : public FLocalVertexFactory
{
public:

    FGeneratedMeshVertexFactory()
    {}


    /** Initialization */
    void Init(const FGeneratedMeshVertexBuffer* VertexBuffer)
    {
        check(!IsInRenderingThread());

        ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
            InitGeneratedMeshVertexFactory,
            FGeneratedMeshVertexFactory*,VertexFactory,this,
            const FGeneratedMeshVertexBuffer*,VertexBuffer,VertexBuffer,
        {
            // Initialize the vertex factory's stream components.
            DataType NewData;
            NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,Position,VET_Float3);
            NewData.TextureCoordinates.Add(
                FVertexStreamComponent(VertexBuffer,STRUCT_OFFSET(FDynamicMeshVertex,TextureCoordinate),sizeof(FDynamicMeshVertex),VET_Float2)
                );
            NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentX,VET_PackedNormal);
            NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentZ,VET_PackedNormal);
            NewData.ColorComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Color, VET_Color);
            VertexFactory->SetData(NewData);
        });
    }
};

/** Scene proxy */
class FGeneratedMeshSceneProxy : public FPrimitiveSceneProxy
{
public:

    FGeneratedMeshSceneProxy(UGeneratedMeshComponent* Component)
        : FPrimitiveSceneProxy(Component)
        , MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
    {
        const FColor VertexColor(255,255,255);

        // Add each triangle to the vertex/index buffer
        for(int TriIdx=0; TriIdx<Component->GeneratedMeshTris.Num(); TriIdx++)
        {
            FGeneratedMeshTriangle& Tri = Component->GeneratedMeshTris[TriIdx];

            const FVector Edge01 = (Tri.Vertex1 - Tri.Vertex0);
            const FVector Edge02 = (Tri.Vertex2 - Tri.Vertex0);

            const FVector TangentX = Edge01.SafeNormal();
            const FVector TangentZ = (Edge02 ^ Edge01).SafeNormal();
            const FVector TangentY = (TangentX ^ TangentZ).SafeNormal();

            FDynamicMeshVertex Vert0;
            Vert0.Position = Tri.Vertex0;
            Vert0.Color = VertexColor;
            Vert0.SetTangents(TangentX, TangentY, TangentZ);
            int32 VIndex = VertexBuffer.Vertices.Add(Vert0);
            IndexBuffer.Indices.Add(VIndex);

            FDynamicMeshVertex Vert1;
            Vert1.Position = Tri.Vertex1;
            Vert1.Color = VertexColor;
            Vert1.SetTangents(TangentX, TangentY, TangentZ);
            VIndex = VertexBuffer.Vertices.Add(Vert1);
            IndexBuffer.Indices.Add(VIndex);

            FDynamicMeshVertex Vert2;
            Vert2.Position = Tri.Vertex2;
            Vert2.Color = VertexColor;
            Vert2.SetTangents(TangentX, TangentY, TangentZ);
            VIndex = VertexBuffer.Vertices.Add(Vert2);
            IndexBuffer.Indices.Add(VIndex);
        }

        // Init vertex factory
        VertexFactory.Init(&VertexBuffer);

        // Enqueue initialization of render resource
        BeginInitResource(&VertexBuffer);
        BeginInitResource(&IndexBuffer);
        BeginInitResource(&VertexFactory);

        // Grab material
        Material = Component->GetMaterial(0);
        if(Material == NULL)
        {
            Material = UMaterial::GetDefaultMaterial(MD_Surface);
        }
    }

    virtual ~FGeneratedMeshSceneProxy()
    {
        VertexBuffer.ReleaseResource();
        IndexBuffer.ReleaseResource();
        VertexFactory.ReleaseResource();
    }

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
    {
        QUICK_SCOPE_CYCLE_COUNTER( STAT_GeneratedMeshSceneProxy_GetDynamicMeshElements );

        const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

        auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
            GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
            FLinearColor(0, 0.5f, 1.f)
            );

        Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

        FMaterialRenderProxy* MaterialProxy = NULL;
        if(bWireframe)
        {
            MaterialProxy = WireframeMaterialInstance;
        }
        else
        {
            MaterialProxy = Material->GetRenderProxy(IsSelected());
        }

        for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
        {
            if (VisibilityMap & (1 << ViewIndex))
            {
                const FSceneView* View = Views[ViewIndex];
                // Draw the mesh.
                FMeshBatch& Mesh = Collector.AllocateMesh();
                FMeshBatchElement& BatchElement = Mesh.Elements[0];
                BatchElement.IndexBuffer = &IndexBuffer;
                Mesh.bWireframe = bWireframe;
                Mesh.VertexFactory = &VertexFactory;
                Mesh.MaterialRenderProxy = MaterialProxy;
                BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
                BatchElement.FirstIndex = 0;
                BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
                BatchElement.MinVertexIndex = 0;
                BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
                Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
                Mesh.Type = PT_TriangleList;
                Mesh.DepthPriorityGroup = SDPG_World;
                Mesh.bCanApplyViewModeOverrides = false;
                Collector.AddMesh(ViewIndex, Mesh);
            }
        }
    }

    virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
    {
        QUICK_SCOPE_CYCLE_COUNTER( STAT_GeneratedMeshSceneProxy_DrawDynamicElements );

        const bool bWireframe = AllowDebugViewmodes() && View->Family->EngineShowFlags.Wireframe;

        FColoredMaterialRenderProxy WireframeMaterialInstance(
            GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
            FLinearColor(0, 0.5f, 1.f)
            );

        FMaterialRenderProxy* MaterialProxy = NULL;
        if(bWireframe)
        {
            MaterialProxy = &WireframeMaterialInstance;
        }
        else
        {
            MaterialProxy = Material->GetRenderProxy(IsSelected());
        }

        // Draw the mesh.
        FMeshBatch Mesh;
        FMeshBatchElement& BatchElement = Mesh.Elements[0];
        BatchElement.IndexBuffer = &IndexBuffer;
        Mesh.bWireframe = bWireframe;
        Mesh.VertexFactory = &VertexFactory;
        Mesh.MaterialRenderProxy = MaterialProxy;
        BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
        BatchElement.FirstIndex = 0;
        BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
        BatchElement.MinVertexIndex = 0;
        BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
        Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
        Mesh.Type = PT_TriangleList;
        Mesh.DepthPriorityGroup = SDPG_World;
        PDI->DrawMesh(Mesh);
    }

    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
    {
        FPrimitiveViewRelevance Result;
        Result.bDrawRelevance = IsShown(View);
        Result.bShadowRelevance = IsShadowCast(View);
        Result.bDynamicRelevance = true;
        MaterialRelevance.SetPrimitiveViewRelevance(Result);
        return Result;
    }

    virtual bool CanBeOccluded() const override
    {
        return !MaterialRelevance.bDisableDepthTest;
    }

    virtual uint32 GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }

    uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

private:

    UMaterialInterface* Material;
    FGeneratedMeshVertexBuffer VertexBuffer;
    FGeneratedMeshIndexBuffer IndexBuffer;
    FGeneratedMeshVertexFactory VertexFactory;

    FMaterialRelevance MaterialRelevance;
};

//////////////////////////////////////////////////////////////////////////

UGeneratedMeshComponent::UGeneratedMeshComponent( const FObjectInitializer& ObjectInitializer )
    : Super( ObjectInitializer )
{
    PrimaryComponentTick.bCanEverTick = false;

    SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
}

bool UGeneratedMeshComponent::SetGeneratedMeshTriangles(const TArray<FGeneratedMeshTriangle>& Triangles)
{
    GeneratedMeshTris = Triangles;
    // Need to recreate scene proxy to send it over
    MarkRenderStateDirty();

    return true;
}


FPrimitiveSceneProxy* UGeneratedMeshComponent::CreateSceneProxy()
{
    FPrimitiveSceneProxy* Proxy = NULL;
    if(GeneratedMeshTris.Num() > 0)
    {
        Proxy = new FGeneratedMeshSceneProxy(this);
    }
    return Proxy;
}

int32 UGeneratedMeshComponent::GetNumMaterials() const
{
    return 1;
}


FBoxSphereBounds UGeneratedMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    FBoxSphereBounds NewBounds;
    NewBounds.Origin = FVector::ZeroVector;
    NewBounds.BoxExtent = FVector(HALF_WORLD_MAX,HALF_WORLD_MAX,HALF_WORLD_MAX);
    NewBounds.SphereRadius = FMath::Sqrt(3.0f * FMath::Square(HALF_WORLD_MAX));
    return NewBounds;
}

