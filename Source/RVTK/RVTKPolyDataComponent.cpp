#include "RVTKPolyDataComponent.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

using namespace UE::Geometry;

URVTKPolyDataComponent::URVTKPolyDataComponent()
{
  SetMobility(EComponentMobility::Movable);

  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
}

////////////////////////////////
//~ Component

void URVTKPolyDataComponent::BeginPlay()
{
  Super::BeginPlay();
  InitializeSafe();
}

void URVTKPolyDataComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if(bDebug)
  {
    FVector2D TimeRange = GetTimeRange();
    float Duration = TimeRange.Y - TimeRange.X;

    if(Duration > 0)
    {
      UpdateTimeStep(DebugClock);

      DebugClock += DeltaTime*DebugTimeScale;
      // Wrap
      while(DebugClock > Duration) DebugClock -= Duration;
    }

    if(GEngine)
    {
      FString DebugMessage = FString::Printf(TEXT("LastTimeStep: %f"), LastTimeStep);
      GEngine->AddOnScreenDebugMessage(0,   // Key: -1 means a new message is added instead of overwriting an old one
                                       5.f, // Duration: how long (in seconds) the text stays on screen
                                       FColor::Cyan,
                                       DebugMessage);     
    }
  }
}

void URVTKPolyDataComponent::OnRegister()
{
  InitializeSafe();
  Super::OnRegister();
}

void URVTKPolyDataComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Poly = nullptr;
  ScalarTimeSteps.Reset();
  ResetMesh();

  Super::EndPlay(EndPlayReason);
}

////////////////////////////////
//~ Init

void URVTKPolyDataComponent::InitializeSafe()
{
  if(!bInitialized)
  {
    bInitialized = true;

    // Poly
    LoadPoly();

    // ScalarTimeSteps
    LoadScalarTimeSteps();

    // Mesh
    SetupMesh();

    // Lut
    BuildLut();

    // Material
    check(!bMaterialInitialized);
    bMaterialInitialized = true;
    FString MaterialPath = TEXT("/RVTK/Materials/M_FEM_Unlit_HeadLight");
    UMaterial* Mat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *MaterialPath));
    if(Mat)
    {
      SetMaterial(0, Mat);
    }
  }
}

////////////////////////////////
//~ TimeSeries Interface

FVector2D URVTKPolyDataComponent::GetTimeRange()
{
  FVector2D Ret(0,0);
  if(ScalarTimeSteps.Num() > 0)
  {
    Ret.X = ScalarTimeSteps[0].TimeStep;
    Ret.Y = ScalarTimeSteps.Last().TimeStep;
  }
  return Ret;
}

float URVTKPolyDataComponent::GetDuration()
{
  float Ret = 0;
  FVector2D TimeRange = GetTimeRange();
  Ret = TimeRange.Y - TimeRange.X;
  return Ret;
}

void URVTKPolyDataComponent::UpdateTimeStep(float TimeStep)
{
  float NewTimeStep = LastTimeStep;
  if(UpdateMeshFromTimeStep(TimeStep, NewTimeStep))
  {
    LastTimeStep = NewTimeStep;
  }
}

int32 URVTKPolyDataComponent::GetNumberOfSteps()
{
  int32 Ret = ScalarTimeSteps.Num();
  return Ret;
}

float URVTKPolyDataComponent::GetCurrentTimeStep()
{
  float Ret = LastTimeStep;
  return Ret;
}

////////////////////////////////
//~ Lut

void URVTKPolyDataComponent::BuildLut()
{
  Lut.Colors.Reset();
  Lut.Colors.SetNumUninitialized(LutResolution);

  for(int i = 0; i < LutResolution; ++i)
  {
    float T = (float)i / 255.0f;
    float TargetHue = FMath::Lerp(240.0f, 0.0f, T);
    uint8 HueByte = (uint8)FMath::Clamp(FMath::RoundToInt(TargetHue * 255.0f / 360.0f), 0, 255);
    Lut.Colors[i] = FLinearColor::MakeFromHSV8(HueByte, 255, 255);
  }
}

////////////////////////////////
//~ Mesh

// NOTE(@k): DEBUG USE ONLY!!!
void URVTKPolyDataComponent::BuildDebugMesh()
{ 
  FDynamicMesh3 Mesh;
  Mesh.EnableAttributes();
  Mesh.Attributes()->EnablePrimaryColors();

  const int32 V0 = Mesh.AppendVertex(FVector3d(0, 0, 0));
  const int32 V1 = Mesh.AppendVertex(FVector3d(100, 0, 0));
  const int32 V2 = Mesh.AppendVertex(FVector3d(0, 100, 0));
  const int32 T0 = Mesh.AppendTriangle(V0, V1, V2);

  FDynamicMeshColorOverlay* Colors = Mesh.Attributes()->PrimaryColors();
  const int32 C0 = Colors->AppendElement(FVector4f(1, 0, 0, 1));
  const int32 C1 = Colors->AppendElement(FVector4f(0, 1, 0, 1));
  const int32 C2 = Colors->AppendElement(FVector4f(0, 0, 1, 1));
  Colors->SetTriangle(T0, FIndex3i(C0, C1, C2));

  SetMesh(MoveTemp(Mesh));
}

void URVTKPolyDataComponent::LoadPoly()
{
  check(Poly == nullptr);
  if(PolyBinPath.FilePath.Len() && FPaths::FileExists(MakePathAbsoluteFromContent(PolyBinPath.FilePath)))
  {
    Poly = PolyFromBin(PolyBinPath);
  }
}

void URVTKPolyDataComponent::LoadScalarTimeSteps()
{
  check(ScalarTimeSteps.Num() == 0);
  if(ScalarBinDirectory.Path.Len() && FPaths::DirectoryExists(MakePathAbsoluteFromContent(ScalarBinDirectory.Path)))
  {
    ScalarTimeStepsFromBinDir(ScalarBinDirectory, ScalarTimeSteps);
  }
}

void URVTKPolyDataComponent::SetupMesh()
{
  check(!bMeshInitialized);

  if(Poly)
  {
    bMeshInitialized = true;
    BuildMeshFromPoly(*Poly);
  }
}

void URVTKPolyDataComponent::ResetMesh()
{
  if(UDynamicMesh *DynMesh = GetDynamicMesh())
  {
    DynMesh->Reset();
    NotifyMeshUpdated();
  }
  bMeshInitialized = false;
}

void URVTKPolyDataComponent::BuildMeshFromPoly(FRVTKPolyData &PolyData)
{
  int VertexCount = PolyData.Vertices.Num();

  FDynamicMesh3 Mesh;
  Mesh.EnableAttributes();
  Mesh.Attributes()->EnablePrimaryColors();

  FDynamicMeshNormalOverlay *Normals = Mesh.Attributes()->PrimaryNormals();
  FDynamicMeshColorOverlay *Colors = Mesh.Attributes()->PrimaryColors();

  TArray<int32> NormalElements;
  NormalElements.SetNumUninitialized(VertexCount);
  TArray<int32> ColorElements;
  ColorElements.SetNumUninitialized(VertexCount);

  // Vertices + per-vertex overlay elements(normal+color)
  for(int i = 0; i < VertexCount; ++i)
  {
    const FVector &V = PolyData.Vertices[i];
    // FIXME(@k): we need to scale them to make collision hit test work, don't know why yet
    Mesh.AppendVertex(FVector(V.X*PositionScale, -V.Y*PositionScale, V.Z*PositionScale)); // Flip Y

    FVector N = PolyData.Normals[i].GetSafeNormal();
    N.Y *= -1; // Flip Y
    NormalElements[i] = Normals->AppendElement(FVector3f(N));

    FLinearColor C = FLinearColor::White; // Default to White Color
    ColorElements[i] = Colors->AppendElement(FVector4f(C));
  }

  // Triangles + per-corner normal/color overlay bindings
  for(int TriIdx = 0; TriIdx + 2 < PolyData.Triangles.Num(); TriIdx += 3)
  {
    int I0 = PolyData.Triangles[TriIdx+0];
    int I1 = PolyData.Triangles[TriIdx+1];
    int I2 = PolyData.Triangles[TriIdx+2];

    check(Mesh.IsVertex(I0) && Mesh.IsVertex(I1) && Mesh.IsVertex(I2));
    int Tid = Mesh.AppendTriangle(I0, I1, I2);
    Normals->SetTriangle(Tid, FIndex3i(NormalElements[I0], NormalElements[I1], NormalElements[I2]));
    Colors->SetTriangle(Tid, FIndex3i(ColorElements[I0], ColorElements[I1], ColorElements[I2]));
  }

  SetMesh(MoveTemp(Mesh));
}

bool URVTKPolyDataComponent::UpdateMeshFromTimeStep(float TimeStep, float &OutTimeStep)
{
  TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);

  bool Ret = false;
  FMemMark Mark(FMemStack::Get());
  FDynamicMesh3* RawMesh = bMeshInitialized ? GetMesh() : 0;

  if(bMeshInitialized && Poly != nullptr && ScalarTimeSteps.Num() > 0 && RawMesh)
  {
    ////////////////////////////////
    //~ Sampling Scalar

    FRVTKScalarTimeStep *PrevStep = 0;
    FRVTKScalarTimeStep *NextStep = 0;

    // FIXME(@k): we could use binary search here, but for now (also I don't have that many datasets) just make it work
    {
      TRACE_CPUPROFILER_EVENT_SCOPE_STR("GetBoundingSteps");
      for(int i = 0; i < ScalarTimeSteps.Num(); ++i)
      {
        FRVTKScalarTimeStep *Curr = &ScalarTimeSteps[i];
        FRVTKScalarTimeStep *Next = (i+1) < ScalarTimeSteps.Num() ? &ScalarTimeSteps[i+1] : 0;

        if(Curr->TimeStep <= TimeStep && Next != 0 && TimeStep <= Next->TimeStep)
        {
          PrevStep = Curr;
          NextStep = Next;
          break;
        }

        if(Curr->TimeStep <= TimeStep && Next == 0)
        {
          PrevStep = NextStep = Curr;
          break;
        }
      }
    }

    int32 VertexCount = Poly->Vertices.Num();
    check(PrevStep && PrevStep->Scalars.Num() == VertexCount);

    float *Scalars = 0;
    int32 ScalarCount = VertexCount;
    float TargetTimeStep = TimeStep;

    // Do interpolation if we could
    if(PrevStep != NextStep)
    {
      TRACE_CPUPROFILER_EVENT_SCOPE_STR("ScalarInterploation");
      float Range = NextStep->TimeStep - PrevStep->TimeStep;
      check(Range > 0);

      float T = RClamp(0.0f, (TimeStep - PrevStep->TimeStep) / Range, 1.0f);
      Scalars = (float*)FMemStack::Get().Alloc(ScalarCount*sizeof(float), alignof(float));
      for(int ScalarIdx = 0; ScalarIdx < ScalarCount; ++ScalarIdx)
      {
        Scalars[ScalarIdx] = FMath::Lerp(PrevStep->Scalars[ScalarIdx], NextStep->Scalars[ScalarIdx], T);
      }
    }
    else
    {
      Scalars = PrevStep->Scalars.GetData();
      TargetTimeStep = PrevStep->TimeStep;
    }

    // TArray<FLinearColor, TMemStackAllocator<>> Colors;
    // Colors.SetNumUninitialized(VertexCount);

    double ScalarMin = LutRange[0];
    double ScalarMax = LutRange[1];
    double ScalarRange = ScalarMax - ScalarMin;
    double ScalarInvRange = !FMath::IsNearlyZero(ScalarRange) ? 1.0 / ScalarRange : 0.0;

    {
      TRACE_CPUPROFILER_EVENT_SCOPE_STR("ColorLut & UpdateColor");

      FDynamicMeshColorOverlay* DstColors = RawMesh->Attributes()->PrimaryColors();
      for(int VertexIdx = 0; VertexIdx < VertexCount; ++VertexIdx)
      {
        // FIXME(@k): we could use a pre-computed color table
        float Scalar = Scalars[VertexIdx];

        // Color Lut
        float Normalized = 0.0f;
        if(ScalarInvRange > 0.0)
        {
          Normalized = (float)((Scalar - ScalarMin) * ScalarInvRange);
        }

        Normalized = RClamp(0.0f, Normalized, 1.0f);
        int LutIndex = FMath::RoundToInt(Normalized*(Lut.Colors.Num()-1));
        FLinearColor Color = Lut.Colors[LutIndex];
        DstColors->SetElement(VertexIdx, FVector4f(Color));
      }
    }

    {
      TRACE_CPUPROFILER_EVENT_SCOPE_STR("FastNotifyColorsUpdated");
      FastNotifyColorsUpdated();
    }
    Ret = true;
    OutTimeStep = TargetTimeStep;

    // for(int VertexIdx = 0; VertexIdx < VertexCount && VertexIdx < RawMesh->MaxVertexID(); ++VertexIdx)
    // {
    //   if(RawMesh->IsVertex(VertexIdx))
    //   {
    //     DstColors->SetElement(VertexIdx, FVector4f(Colors[VertexIdx]));
    //   }
    // }
  }
  return Ret;
}

////////////////////////////////
//~ Editor

#if WITH_EDITOR
void URVTKPolyDataComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
  Super::PostEditChangeProperty(PropertyChangedEvent);

  const FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr)
    ? PropertyChangedEvent.MemberProperty->GetFName()
    : NAME_None;

  const bool bPolyBinPathChanged = PropertyName == GET_MEMBER_NAME_CHECKED(URVTKPolyDataComponent, PolyBinPath);
  const bool bScalarBinDirectoryChanged = PropertyName == GET_MEMBER_NAME_CHECKED(URVTKPolyDataComponent, ScalarBinDirectory);

  // Normalize PolyBinPath
  if(bPolyBinPathChanged)
  {
    PolyBinPath.FilePath = MakePathProjectContentRelative(PolyBinPath.FilePath);
  }

  if(bScalarBinDirectoryChanged)
  {
    ScalarBinDirectory.Path = MakePathProjectContentRelative(ScalarBinDirectory.Path);
  }

  if(bPolyBinPathChanged)
  {
    Poly = nullptr;
    LoadPoly();

    ResetMesh();
    SetupMesh();
  }

  if(bPolyBinPathChanged || bScalarBinDirectoryChanged)
  {
    ScalarTimeSteps.Reset();
    LoadScalarTimeSteps();
  }
}
#endif
