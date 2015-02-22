// Copyright 2015 Tobias Pfaff. All rights reserved
 
#pragma once
 
#include "GameFramework/Actor.h"
#include "GeneratedMeshComponent.h"
#include "GameGeneratedActor.generated.h"
 
/**
 * 
 */
UCLASS()
class AGameGeneratedActor : public AActor
{
    GENERATED_UCLASS_BODY()
 
    void ConstructVoxels(TArray<FGeneratedMeshTriangle>& triangles);
};