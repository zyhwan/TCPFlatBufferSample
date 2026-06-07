// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Tickable.h"

#include "TCPClientSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTCPConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTCPDisconnected);

class FSocket;

/**
 * 
 */
UCLASS()
class TCPFLATBUFFERSAMPLE_API UTCPClientSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "TCP")
	bool Connect(const FString& Host, int32 Port);

	UFUNCTION(BlueprintCallable, Category = "TCP")
	void Disconnect();

	UFUNCTION(BlueprintCallable, Category = "TCP")
	bool IsConnected() const;

	UPROPERTY(BlueprintAssignable, Category = "TCP")
	FOnTCPConnected OnTCPConnected;

	UPROPERTY(BlueprintAssignable, Category = "TCP")
	FOnTCPDisconnected OnTCPDisconnected;

	UFUNCTION(BlueprintCallable, Category = "TCP")
	void SendLogin(const FString& UserID, const FString& Password);

	virtual void Deinitialize() override;
private:
	TArray<uint8> RecvBuffer;

	void RecvAll();

	bool SendAll(const uint8* Body, uint32 BodyLength);

	//언리얼 자체에서 제공해주는 자체 소켓.
	FSocket* ServerSocket = nullptr;

	void DispatchPacket();

	// Inherited via FTickableGameObject
	TStatId GetStatId() const override;

	virtual void Tick(float DeltaTime) override;
};
