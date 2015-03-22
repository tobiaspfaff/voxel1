// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GeneratedMeshComponent.generated.h"

/** Component that allows you to specify Generated triangle mesh geometry */
UCLASS(hidecategories=(Object,LOD, Physics, Collision), editinlinenew, meta=(BlueprintSpawnableComponent), ClassGroup=Rendering)
class UGeneratedMeshComponent : public UMeshComponent
{
    GENERATED_UCLASS_BODY()

    /** Set the geometry to use on this triangle mesh */
    bool SetTriangleData(float* Data, int32 Num);

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

    // raw triangle data
    float* FaceData;
    int32 NumFaces;

    friend class FGeneratedMeshSceneProxy;
};


