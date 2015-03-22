// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. 

#include "voxel1.h"
//#include "GeneratedMeshComponentPluginPrivatePCH.h"
#include "GeneratedMeshComponent.h"
#include "DynamicMeshBuilder.h"

struct FGeneratedVertex {
    FVector Pos;
    FPackedNormal T1, T2;

    FGeneratedVertex() {}
    FGeneratedVertex(const FVector& P, const FVector& TX, const FVector& TY, const FVector& TZ) : 
        Pos(P), T1(TX), T2(TZ)
    {
        T2.Vector.W = GetBasisDeterminantSign(TX,TY,TZ) < 0.0f ? 0 : 255;
    }
};

/** Vertex Buffer */
class FGeneratedMeshVertexBuffer : public FVertexBuffer 
{
public:
    TArray<FGeneratedVertex> Vertices;

    virtual void InitRHI() override
    {
        FRHIResourceCreateInfo CreateInfo;
        VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FGeneratedVertex),BUF_Static,CreateInfo);

        // Copy the vertex data into the vertex buffer.
        void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI,0,Vertices.Num() * sizeof(FGeneratedVertex), RLM_WriteOnly);
        FMemory::Memcpy(VertexBufferData,Vertices.GetData(),Vertices.Num() * sizeof(FGeneratedVertex));
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
            NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FGeneratedVertex,Pos,VET_Float3);
            NewData.TextureCoordinates.Add(STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FGeneratedVertex,Pos,VET_Float2));
 
            NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FGeneratedVertex,T1,VET_PackedNormal);
            NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FGeneratedVertex,T2,VET_PackedNormal);
            //NewData.ColorComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FGeneratedVertex, X, VET_Color);
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
        float* FacePtr = Component->FaceData;
        for(int32 TriIdx=0; TriIdx<Component->NumFaces; TriIdx++)
        {
            FVector V[3];
            for (int32 VertIdx=0; VertIdx<3; VertIdx++)
            for (int32 Comp=0; Comp<3; Comp++)
                V[VertIdx][Comp] = *(FacePtr++);

            const FVector Edge01 = (V[1] - V[0]);
            const FVector Edge02 = (V[2] - V[0]);

            const FVector TangentX = Edge01.SafeNormal();
            const FVector TangentZ = (Edge02 ^ Edge01).SafeNormal();
            const FVector TangentY = (TangentX ^ TangentZ).SafeNormal();

            FGeneratedVertex Vert0(V[0], TangentX, TangentY, TangentZ);
            int32 VIndex = VertexBuffer.Vertices.Add(Vert0);
            IndexBuffer.Indices.Add(VIndex);

            FGeneratedVertex Vert1(V[1], TangentX, TangentY, TangentZ);
            VIndex = VertexBuffer.Vertices.Add(Vert1);
            IndexBuffer.Indices.Add(VIndex);

            FGeneratedVertex Vert2(V[2], TangentX, TangentY, TangentZ);
            VIndex = VertexBuffer.Vertices.Add(Vert2);
            IndexBuffer.Indices.Add(VIndex);
        }
        FElement& Element = *new(Elements)FElement;
        Element.FirstIndex = 0;
        Element.NumPrimitives = IndexBuffer.Indices.Num() / 3;
        
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

    virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View) override
    {
        // Set up the wireframe material Face.
        FColoredMaterialRenderProxy WireframeMaterialFace(
            WITH_EDITOR ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
            FLinearColor(0, 0.5f, 1.f)
            );

        // Draw the mesh elements.
        for(int32 ElementIndex = 0; ElementIndex < Elements.Num(); ++ElementIndex)
        {
            PDI->DrawMesh(GetMeshBatch(ElementIndex, View->Family->EngineShowFlags.Wireframe ? &WireframeMaterialFace : NULL));
        }
    }

    virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override
    {
        for(int32 ElementIndex = 0; ElementIndex < Elements.Num(); ++ElementIndex)
        {
            PDI->DrawMesh(GetMeshBatch(ElementIndex,NULL),FLT_MAX);
        }
    }

    FMeshBatch GetMeshBatch(int32 ElementIndex,FMaterialRenderProxy* WireframeMaterialFace)
    {
        const FElement& Element = Elements[ElementIndex];
        FMeshBatch Mesh;
        Mesh.bWireframe = WireframeMaterialFace != NULL;
        Mesh.VertexFactory = &VertexFactory;
        Mesh.MaterialRenderProxy = WireframeMaterialFace ? WireframeMaterialFace : Material->GetRenderProxy(IsSelected());
        Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
        Mesh.Type = PT_TriangleList;
        Mesh.DepthPriorityGroup = SDPG_World;
        Mesh.CastShadow = true;
        Mesh.Elements[0].FirstIndex = Element.FirstIndex;
        Mesh.Elements[0].NumPrimitives = Element.NumPrimitives;
        Mesh.Elements[0].MinVertexIndex = 0;
        Mesh.Elements[0].MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
        Mesh.Elements[0].IndexBuffer = &IndexBuffer;
        Mesh.Elements[0].PrimitiveUniformBuffer = PrimitiveUniformBuffer;
        return Mesh;
    }

    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
    {
        FPrimitiveViewRelevance Result;
        Result.bDrawRelevance = IsShown(View);
        Result.bShadowRelevance = IsShadowCast(View);
        Result.bDynamicRelevance = View->Family->EngineShowFlags.Wireframe || IsSelected();
        Result.bStaticRelevance = !Result.bDynamicRelevance;
        MaterialRelevance.SetPrimitiveViewRelevance(Result);
        return Result;
    }

    virtual bool CanBeOccluded() const override
    {
        return !MaterialRelevance.bDisableDepthTest;
    }

    virtual void OnTransformChanged() override
    {
        // Create a uniform buffer with the transform for the chunk.
        PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
    }

    virtual uint32 GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }

    uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

private:
    struct FElement
    {
        uint32 FirstIndex;
        uint32 NumPrimitives;
        uint32 MaterialIndex;
        uint32 FaceIndex;
    };
    TArray<FElement> Elements;

    UMaterialInterface* Material;
    FGeneratedMeshVertexBuffer VertexBuffer;
    FGeneratedMeshIndexBuffer IndexBuffer;
    FGeneratedMeshVertexFactory VertexFactory;

    FMaterialRelevance MaterialRelevance;

    TUniformBufferRef<FPrimitiveUniformShaderParameters> PrimitiveUniformBuffer;

};

//////////////////////////////////////////////////////////////////////////

UGeneratedMeshComponent::UGeneratedMeshComponent( const FObjectInitializer& ObjectInitializer )
    : Super( ObjectInitializer ), FaceData(nullptr), NumFaces(0)
{
    PrimaryComponentTick.bCanEverTick = false;
    CastShadow = true;
    bUseAsOccluder = true;
    bCanEverAffectNavigation = true;    
    //bAutoRegister = false;
    SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
}

bool UGeneratedMeshComponent::SetTriangleData(float* Data, int32 Num)
{
    FaceData = Data;
    NumFaces = Num;
    return true;
}


FPrimitiveSceneProxy* UGeneratedMeshComponent::CreateSceneProxy()
{
    FPrimitiveSceneProxy* Proxy = NULL;
    if(NumFaces > 0)
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

