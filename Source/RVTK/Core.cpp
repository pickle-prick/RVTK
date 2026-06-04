#include "Core.h"

////////////////////////////////
//~ Poly & Scalar Functions

//- Loader
//

TUniquePtr<FRVTKPolyData> PolyFromBin(FFilePath &BinPath)
{
  TUniquePtr<FRVTKPolyData> Ret = nullptr;
  FString PathStr = MakePathAbsoluteFromContent(BinPath.FilePath);
  TArray<uint8> FileData;
  if(PathStr.Len() > 0 && FFileHelper::LoadFileToArray(FileData, *PathStr))
  {
    check(FileData.Num() > 8);
    Ret = MakeUnique<FRVTKPolyData>();
    uint8 *Ptr = FileData.GetData();

    // Header (8 bytes total: 2 x uint32)
    uint32 VertexCount = *((uint32*)(Ptr+0));
    uint32 IndexCount =  *((uint32*)(Ptr+4));
    size_t VertexDataSize = VertexCount*sizeof(FVector);
    size_t IndexDataSize = IndexCount*sizeof(uint32);
    check(VertexCount > 0 && IndexCount > 0);
    size_t DataSize = FileData.Num();
    check(FileData.Num() >= 8+VertexDataSize+IndexDataSize);
    Ptr += 8; // Skip the 8-byte header
    
    Ret->Vertices.SetNumUninitialized(VertexCount);
    FMemory::Memcpy(Ret->Vertices.GetData(), Ptr, VertexDataSize); // f64*3

    Ret->Triangles.SetNumUninitialized(IndexCount);
    FMemory::Memcpy(Ret->Triangles.GetData(), Ptr+VertexDataSize, IndexDataSize); // uint32

    // Generate normals
    ComputeNormalsFromPoly(Ret->Vertices, Ret->Triangles, Ret->Normals);
  }

  return MoveTemp(Ret);
}

void ScalarTimeStepsFromBinDir(FDirectoryPath &SearchDirectory, TArray<FRVTKScalarTimeStep> &OutArray)
{
  FString SearchPathStr = MakePathAbsoluteFromContent(SearchDirectory.Path);
  // FPaths::NormalizeDirectoryName(SearchPathStr);
  FString SearchPattern = FPaths::Combine(SearchPathStr, TEXT("*.bin"));

  TArray<FString> FoundFileNames;
  // Parameters: (OutputArray, SearchPattern, SearchForFiles, SearchForDirectories)
  IFileManager::Get().FindFiles(FoundFileNames, *SearchPattern, true, false);

  // Sort the file names numerically
  FoundFileNames.Sort([](const FString& A, const FString& B)
  {
    int32 NumA = FCString::Atoi(*FPaths::GetBaseFilename(A));
    int32 NumB = FCString::Atoi(*FPaths::GetBaseFilename(B));
    return NumA < NumB;
  });

  OutArray.Reset();
  OutArray.Reserve(FoundFileNames.Num());

  for(int i = 0; i < FoundFileNames.Num(); ++i)
  {
    const FString& FileName = FoundFileNames[i];
    // FindFiles returns only the filenames (e.g., "data.bin"), not full paths.
    // We combine the base folder path with the filename to get the full absolute path.
    FString FullFilePath = FPaths::Combine(SearchPathStr, FileName);

    TArray<uint8> FileData;
    if(FFileHelper::LoadFileToArray(FileData, *FullFilePath))
    {
      uint8 *Ptr = FileData.GetData();
      FRVTKScalarTimeStep& NewTimeStep = OutArray.AddDefaulted_GetRef();

      // Header (4 bytes total: uint32)
      uint32 Count = *((uint32*)(Ptr+0));
      Ptr += 4; // Skip Header Bytes
      
      // Data
      TArray<float> Scalars;
      Scalars.SetNumUninitialized(Count);
      FMemory::Memcpy(Scalars.GetData(), Ptr, Count*sizeof(float)); // f32

      // Fill
      NewTimeStep.StepIdx  = i;
      NewTimeStep.TimeStep = i*0.1f;
      NewTimeStep.Scalars  = MoveTemp(Scalars);
      NewTimeStep.Path     = MoveTemp(FullFilePath);
    }
  }
}

//- Geometry
//

void ComputeNormalsFromPoly(TArray<FVector> &Vertices, TArray<uint32> &Triangles, TArray<FVector> &OutNormals)
{
  const int32 VertexCount = Vertices.Num();
  const int32 TriangleCount = Triangles.Num() / 3;
  OutNormals.SetNumZeroed(VertexCount);

  for(int32 TriIdx = 0; TriIdx + 2 < Triangles.Num(); TriIdx += 3)
  {
    const int32 I0 = Triangles[TriIdx];
    const int32 I1 = Triangles[TriIdx + 1];
    const int32 I2 = Triangles[TriIdx + 2];
    if(!Vertices.IsValidIndex(I0) || !Vertices.IsValidIndex(I1) || !Vertices.IsValidIndex(I2))
    {
      continue;
    }

    const FVector& V0 = Vertices[I0];
    const FVector& V1 = Vertices[I1];
    const FVector& V2 = Vertices[I2];
    const FVector FaceNormal = FVector::CrossProduct(V1 - V0, V2 - V0);
    OutNormals[I0] += FaceNormal;
    OutNormals[I1] += FaceNormal;
    OutNormals[I2] += FaceNormal;
  }

  for(int32 i = 0; i < OutNormals.Num(); ++i)
  {
    OutNormals[i].Normalize();
  }
}

////////////////////////////////
//~ Path Helpers

FString MakePathProjectContentRelative(FString &InPath)
{
  FString Ret;
  if(InPath.Len())
  {
    FString FullPath = FPaths::ConvertRelativePathToFull(InPath);
    FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    // Either FullPath is outside of ContentDir or inside

    FPaths::MakePathRelativeTo(FullPath, *ContentDir);
    Ret = FullPath;
  }
  return Ret;
}

FString MakePathAbsoluteFromContent(FString& InPath)
{
  FString Ret;
  if(InPath.Len())
  {
    // Combine the relative path with the project's content directory
    FString CombinedPath = FPaths::Combine(FPaths::ProjectContentDir(), InPath);
    Ret = FPaths::ConvertRelativePathToFull(CombinedPath);
  }
  return Ret;
}
