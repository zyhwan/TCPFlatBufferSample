// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TCPTestActor.generated.h"

class UTCPClientSubsystem;

UCLASS()
class TCPFLATBUFFERSAMPLE_API ATCPTestActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATCPTestActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	UTCPClientSubsystem* GetTCP();

	UFUNCTION()
	void HandleConnected();

	UFUNCTION()
	void HandleDisconnected();

};
