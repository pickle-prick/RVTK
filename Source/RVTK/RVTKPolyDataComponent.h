#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"
#include "Core.h"
#include "RVTKPolyDataComponent.generated.h"

UCLASS(BlueprintType, ClassGroup=(Rendering, Common), HideCategories=(Object, Activation, "Components|Activation"), ShowCategories=(Mobility), Meta = (BlueprintSpawnableComponent))
class RVTK_API URVTKPolyDataComponent : public UDynamicMeshComponent
{
  GENERATED_BODY()

public:
  URVTKPolyDataComponent();

  ////////////////////////////////
  //~ Component Hook

  void BeginPlay() override;
  void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
  // void OnComponentCreated() override;
  void OnRegister() override;
  void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  ////////////////////////////////
  //~ DataSource

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DataSource", meta=(FilePathFilter="bin"))
  FFilePath PolyBinPath;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DataSource")
  FDirectoryPath ScalarBinDirectory;

  ////////////////////////////////
  //~ Init

  bool bInitialized = false;
  void InitializeSafe(); // Can be called multiple times

  ////////////////////////////////
  //~ TimeSeries Interface (Main API)

  float LastTimeStep = 0.0f;

  UFUNCTION(BlueprintCallable, Category = "Time Series")
  FVector2D GetTimeRange();

  UFUNCTION(BlueprintCallable, Category = "Time Series")
  float GetDuration();

  UFUNCTION(BlueprintCallable, Category = "Time Series")
  void UpdateTimeStep(float TimeStep);

  UFUNCTION(BlueprintCallable, Category = "Time Series")
  int32 GetNumberOfSteps();

  UFUNCTION(BlueprintCallable, Category = "Time Series")
  float GetCurrentTimeStep();

  ////////////////////////////////
  //~ Lut

  // Lut Value Range 
  UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category="Lut")
  FVector2D LutRange = FVector2D(0.0, 16.0f);

  // Lut Settings (palette, resolution ...etc)
  UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category="Lut")
  int32 LutResolution = 256;

  FRVTKLut Lut;

  void BuildLut();

  ////////////////////////////////
  //~ Mesh

  // Poly & ScalarSteps
  TUniquePtr<FRVTKPolyData> Poly;
  TArray<FRVTKScalarTimeStep> ScalarTimeSteps;

  void LoadPoly();
  void LoadScalarTimeSteps();

  // Settings
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mesh")
  double PositionScale = 100.0;

  // DEBUG ONLY !!!
  void BuildDebugMesh();

  // If we already uploaded mesh to gpu by calling SetMesh
  bool bMeshInitialized = false;
  bool bMaterialInitialized = false;
  // Build initial mesh from case and apply mesh settings (Material ...etc)
  void SetupMesh();
  void ResetMesh();

  void BuildMeshFromPoly(FRVTKPolyData &PolyData);
  bool UpdateMeshFromTimeStep(float TimeStep, float &OutTimeStep);

  ////////////////////////////////
  //~ DEBUG

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
  bool bDebug = false;
  float DebugClock = 0.0f;
  float DebugTimeScale = 0.5f;

  ////////////////////////////////
  //~ Editor

#if WITH_EDITOR
  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
