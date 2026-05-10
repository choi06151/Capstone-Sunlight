// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UDPReceiver.generated.h"

UCLASS()
class SUNSHINE_API AUDPReceiver : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUDPReceiver();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 언리얼이 꺼질 때 소켓을 닫아줄 함수 추가
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// UDP 통신을 위한 소켓 포인터
	class FSocket* ListenSocket;

	// 블루프린트에서 노드로 튀어나올 이벤트 (구현은 C++이 아니라 블루프린트에서 합니다!)
	UFUNCTION(BlueprintImplementableEvent, Category = "UDP")
	void OnHandDataReceived(const FString& HandType, FVector FingerPos, const FString& Gesture, float Angle);
};

