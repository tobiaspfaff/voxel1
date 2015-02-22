// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GeneratedMeshComponent.generated.h"

USTRUCT(BlueprintType)
struct FGeneratedMeshTriangle
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, Category=Triangle)
    FVector Vertex0;

    UPROPERTY(EditAnywhere, Category=Triangle)
    FVector Vertex1;

    UPROPERTY(EditAnywhere, Category=Triangle)
    FVector Vertex2;
};

/** Component that allows you to specify Generated triangle mesh geometry */
UCLASS(hidecategories=(Object,LOD, Physics, Collision), editinlinenew, meta=(BlueprintSpawnableComponent), ClassGroup=Rendering)
class UGeneratedMeshComponent : public UMeshComponent
{
    GENERATED_UCLASS_BODY()

    /** Set the geometry to use on this triangle mesh */
    UFUNCTION(BlueprintCallable, Category="Components|GeneratedMesh")
    bool SetGeneratedMeshTriangles(const TArray<FGeneratedMeshTriangle>& Triangles);

private:

    // Begin UPrimitiveComponent interface.
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    // End UPrimitiveComponent interface.

    // Begin UMeshComponent interface.
    virtual int32 GetNumMaterials() const override;
    // End UMeshComponent interface.

    // Begin USceneComponent interface.
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    // Begin USceneComponent interface.

    /** */
    TArray<FGeneratedMeshTriangle> GeneratedMeshTris;

    friend class FGeneratedMeshSceneProxy;
};


