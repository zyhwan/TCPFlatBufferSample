// Fill out your copyright notice in the Description page of Project Settings.


#include "TCPTestActor.h"
#include "TCPClientSubsystem.h"

// Sets default values
ATCPTestActor::ATCPTestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATCPTestActor::BeginPlay()
{
	Super::BeginPlay();
	
	UTCPClientSubsystem* TCP = GetTCP();

	TCP->OnTCPConnected.AddDynamic(this, &ATCPTestActor::HandleConnected);
	TCP->OnTCPDisconnected.AddDynamic(this, &ATCPTestActor::HandleDisconnected);

	TCP->Connect(TEXT("127.0.0.1"), 35000);
}

void ATCPTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	UTCPClientSubsystem* TCP = GetTCP();

	TCP->OnTCPConnected.RemoveDynamic(this, &ATCPTestActor::HandleConnected);
	TCP->OnTCPDisconnected.RemoveDynamic(this, &ATCPTestActor::HandleDisconnected);

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ATCPTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

UTCPClientSubsystem* ATCPTestActor::GetTCP()
{
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		return GI->GetSubsystem<UTCPClientSubsystem>();
	}

	return nullptr;
}

void ATCPTestActor::HandleConnected()
{
	UE_LOG(LogTemp, Warning, TEXT("Connected Server"));

	UTCPClientSubsystem* TCP = GetTCP();

	TCP->SendLogin(TEXT("zyhwan"), TEXT("a"));
}

void ATCPTestActor::HandleDisconnected()
{
	UE_LOG(LogTemp, Warning, TEXT("Disonnected Server"));
}

