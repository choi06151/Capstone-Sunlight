#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UDPReceiver.generated.h"

UCLASS()
class SUNSHINE_API AUDPReceiver : public AActor
{
    GENERATED_BODY()

public:
    AUDPReceiver();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    class FSocket* ListenSocket;

    UFUNCTION(BlueprintImplementableEvent, Category = "UDP")
    void OnLeftHandDataReceived(FVector FingerPos, const FString& Gesture, float Angle);

    UFUNCTION(BlueprintImplementableEvent, Category = "UDP")
    void OnRightHandDataReceived(FVector FingerPos, const FString& Gesture, float Angle);
};