// Copyright 2015 Tobias Pfaff. All rights reserved
 
#include "voxel1.h"
#include "GameGeneratedActor.h"

AGameGeneratedActor::AGameGeneratedActor(const class FPostConstructInitializeProperties& PCIP)
    : Super(PCIP)
{
 
    TSubobjectPtr<UGeneratedMeshComponent> Mesh = PCIP.CreateDefaultSubobject<UGeneratedMeshComponent>(this, TEXT("GeneratedMesh"));
 
    TArray<FGeneratedMeshTriangle> Triangles;
    ConstructVoxels(Triangles);
    Mesh->SetGeneratedMeshTriangles(Triangles);
 
    RootComponent = Mesh;
}
 
inline void AddQuad(const FVector& Pos, int32 Dim, bool bCCW, float Size, TArray<FGeneratedMeshTriangle>& Triangles) 
{
    FVector V0(0);
    FVector V1(0);
    FVector C = Pos;
    C[Dim] += bCCW ? Size/2 : -Size/2;
    V0[(Dim+1) % 3] += bCCW ? -Size/2 : Size/2; 
    V1[(Dim+2) % 3] += Size / 2;
    
    FGeneratedMeshTriangle Tri;
    Tri.Vertex0 = C+V0+V1;
    Tri.Vertex1 = C-V0+V1;
    Tri.Vertex2 = C+V0-V1;
    Triangles.Add(Tri);

    Tri.Vertex0 = C-V0+V1;
    Tri.Vertex1 = C-V0-V1;
    Tri.Vertex2 = C+V0-V1;
    Triangles.Add(Tri);
}

inline int32 Index(const FIntVector& P, int FieldBits) {
    return P.X | (P.Y<<FieldBits) | (P.Z<<(FieldBits<<1));
}

void AGameGeneratedActor::ConstructVoxels(TArray<FGeneratedMeshTriangle>& Triangles) 
{
    //UE_LOG(LogClass, Log, TEXT("AGameGeneratedActor::Lathe POINTS %d"), points.Num());
 
    // generate voxel field
    const int32 FieldBits = 6, FieldSize = 1<<FieldBits;
    uint8 Voxels[FieldSize*FieldSize*FieldSize];

    for (int32 i=0,idx=0; i<FieldSize; i++)
    for (int32 j=0; j<FieldSize; j++)
    for (int32 k=0; k<FieldSize; k++,idx++) 
    {
        float Freq = M_PI / 20;
        Voxels[idx] = sin(Freq*i) * sin(Freq*j) * sin(Freq*k) > 0.4 ? 1 : 0;
    }

    float Scale = 10;
    
    // generate cube geometry
    for (int32 i=1; i<FieldSize-1; i++)
    for (int32 j=1; j<FieldSize-1; j++)
    for (int32 k=1; k<FieldSize-1; k++) 
    {
        FIntVector Pos0 (i, j, k);
        if (!Voxels[Index(Pos0, FieldBits)])
            continue;

        for (int32 d=0; d<3; d++) 
        {
            FIntVector Pos = Pos0;
            Pos(d) += 1;
            if (!Voxels[Index(Pos, FieldBits)])
                AddQuad(FVector(Pos0)*Scale, d, true, Scale, Triangles);
            Pos(d) -= 2;
            if (!Voxels[Index(Pos, FieldBits)])
                AddQuad(FVector(Pos0)*Scale, d, false, Scale, Triangles);               
        }
    }
}