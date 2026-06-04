#pragma once

#include "CoreMinimal.h"

////////////////////////////////
//~ Poly & Scalar

struct FRVTKScalarTimeStep
{
  int StepIdx;
  float TimeStep;
  TArray<float> Scalars;
  FString Path;
};

struct FRVTKPolyData
{
  TArray<FVector> Vertices;
  TArray<uint32> Triangles;
  TArray<FVector> Normals;
};

////////////////////////////////
//~ LUT

struct FRVTKLut
{
  TArray<FLinearColor> Colors;
};

////////////////////////////////
//~ Poly & Scalar Functions

//- Loader
//

TUniquePtr<FRVTKPolyData> PolyFromBin(FFilePath &BinPath);
void ScalarTimeStepsFromBinDir(FDirectoryPath &SearchDirectory, TArray<FRVTKScalarTimeStep> &OutArray);

//- Geometry
//

void ComputeNormalsFromPoly(TArray<FVector> &Vertices, TArray<uint32> &Triangles, TArray<FVector> &OutNormals);

////////////////////////////////
//~ Path Helpers

FString MakePathProjectContentRelative(FString &InPath);
FString MakePathAbsoluteFromContent(FString& InOutPath);


////////////////////////////////
//~ Clamps, Mins, Maxes

#define RMin(A,B) (((A)<(B))?(A):(B))
#define RMax(A,B) (((A)>(B))?(A):(B))
#define RClampTop(A,X) RMin(A,X)
#define RClampBot(X,B) RMax(X,B)
#define RClamp(A,X,B) (((X)<(A))?(A):((X)>(B))?(B):(X))
