// Fill out your copyright notice in the Description page of Project Settings.


#include "TCPClientSubsystem.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"

#include "UserPacket_generated.h"

bool UTCPClientSubsystem::Connect(const FString& Host, int32 Port)
{
	//Socket 만들기
	ISocketSubsystem* SocketSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	//gethostbyname
	FAddressInfoResult AddrInfo = SocketSubSystem->GetAddressInfo(*Host, nullptr, EAddressInfoFlags::Default, NAME_None);
	
	//SOCKADDR 만들기 
	TSharedRef<FInternetAddr> ServerAddr = AddrInfo.Results[0].Address;

	//Port
	ServerAddr->SetPort(Port);

	//Socket
	ServerSocket = SocketSubSystem->CreateSocket(NAME_None, TEXT("TCPClient"), ServerAddr->GetProtocolType());

	//connect()
	if (!ServerSocket)
	{
		UE_LOG(LogTemp, Warning, TEXT("Connect Failed."));
		return false;
	}

	if (!ServerSocket->Connect(*ServerAddr))
	{
		UE_LOG(LogTemp, Warning, TEXT("Connect Failed."));
		return false;
	}

	ServerSocket->SetNonBlocking(false);

	RecvBuffer.Reset();

	OnTCPConnected.Broadcast();

	return true;
}

void UTCPClientSubsystem::Disconnect()
{
	ISocketSubsystem* SocketSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	ServerSocket->Close();
	SocketSubSystem->DestroySocket(ServerSocket);

	ServerSocket = nullptr;

	OnTCPDisconnected.Broadcast();
}

bool UTCPClientSubsystem::IsConnected() const
{
	return ServerSocket != nullptr && ServerSocket->GetConnectionState() == SCS_Connected;
}

void UTCPClientSubsystem::SendLogin(const FString& UserID, const FString& Password)
{
	flatbuffers::FlatBufferBuilder Builder;

	const FTCHARToUTF8 UserUTF8(UserID);
	const FTCHARToUTF8 PasswordUTF8(Password);

	auto LoginData = UserPacket::CreateC2S_LoginDirect(
		Builder,
		UserUTF8.Get(),
		PasswordUTF8.Get()
	);

	auto PacketData = UserPacket::CreatePacketData(
		Builder,
		UserPacket::PacketType_C2S_Login,
		LoginData.Union()
	);

	Builder.Finish(PacketData);

	SendAll(Builder.GetBufferPointer(), Builder.GetSize());
}

void UTCPClientSubsystem::Deinitialize()
{
	Disconnect();

	Super::Deinitialize();
}

void UTCPClientSubsystem::RecvAll()
{
	if (!ServerSocket)
	{
		return;
	}

	uint32 Pending = 0;
	if (!ServerSocket->HasPendingData(Pending) || Pending <= 0) //나 다음에 받을거 있어?
	{
		//없다면 리턴.
		return;
	}

	//전제 - 누가 공격하지 않는다면.
	//1. 2byte 헤더 받기
	uint16 NetPacketSize = 0; //BigEndian
	uint16 PacketSize = 0; //LitteleEndian

	int32 TotalRecvBytes = 0; //총 받은 바이트 수 
	int32 RecvBytes = 0; //얼마나 받았는지

	//2바이트 받았는지 확인하기.
	while (TotalRecvBytes < (int32)sizeof(NetPacketSize))
	{
		if (!ServerSocket->Recv((uint8*)&NetPacketSize + TotalRecvBytes, sizeof(NetPacketSize) - TotalRecvBytes, RecvBytes)
			|| RecvBytes == 0)
		{
			Disconnect();
			break;
		}
		TotalRecvBytes += RecvBytes;
	}

	PacketSize = NETWORK_ORDER16(NetPacketSize);

	RecvBuffer.SetNumUninitialized(PacketSize);

	TotalRecvBytes = 0; //총 받은 바이트 수 
	RecvBytes = 0; //얼마나 받았는지

	//Body
	while (TotalRecvBytes < (int32)(PacketSize))
	{
		if (!ServerSocket->Recv(RecvBuffer.GetData() + TotalRecvBytes, PacketSize - TotalRecvBytes, RecvBytes)
			|| RecvBytes == 0)
		{
			Disconnect();
			break;
		}
		TotalRecvBytes += RecvBytes;
	}

	//예외 처리 안함. 일단 못받아도 넘긴다.
	DispatchPacket();
	RecvBuffer.Reset();

}

bool UTCPClientSubsystem::SendAll(const uint8* Body, uint32 BodyLength)
{
	TArray<uint8> Packet;
	//[][] [][][][][][][]
	Packet.Reserve(2 + BodyLength); //이전 char buffer[1024] 이런식으로 작업한 내용과 동일.
	//[][] -> Header
	FMemory::Memcpy(Packet.GetData(), &BodyLength, 2);
	Packet.Add((uint8)((BodyLength >> 8) & 0xFF));
	Packet.Add((uint8)((BodyLength) & 0xFF));
	Packet.Append(Body, BodyLength);

	int32 SentTotalBytes = 0;
	while (SentTotalBytes < Packet.Num())
	{
		int32 SentBytes = 0;
		if (!ServerSocket->Send(Packet.GetData() + SentTotalBytes, Packet.Num() - SentTotalBytes, SentBytes) || SentBytes < 0)
		{
			return false;
		}
		SentTotalBytes += SentBytes;
	}

	return true;
}

void UTCPClientSubsystem::DispatchPacket()
{
	//flatbuffer, -> Extract

	const auto UserPacketData = UserPacket::GetPacketData(RecvBuffer.GetData());
	switch (UserPacketData->data_type())
	{
	case UserPacket::PacketType_S2C_Login:
	{
		//Delegate로 바꿈
		const auto* LoginData = UserPacketData->data_as_S2C_Login();

		FString Message = UTF8_TO_TCHAR(LoginData->message()->c_str());

		UE_LOG(LogTemp, Warning, TEXT("Login %d %s"), LoginData->clientsocket_id(), *Message);
	}
	break;
	case UserPacket::PacketType_S2C_Spawn:
	{
	}
	break;
	case UserPacket::PacketType_S2C_Move:
	{

	}
	break;
	case UserPacket::PacketType_S2C_Destroy:
	{
	}
	break;

	case UserPacket::PacketType_S2C_ChangeColor:
	{
	}
	break;
	}
}

TStatId UTCPClientSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTCPClientSubsystem, STATGROUP_Tickables);
}

void UTCPClientSubsystem::Tick(float DeltaTime)
{
	RecvAll();
}