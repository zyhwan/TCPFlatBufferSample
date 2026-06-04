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

void UTCPClientSubsystem::RecvAll()
{
	if (!ServerSocket)
	{
		return;
	}

	//1. 2byte 헤더 받기
	uint32 Pending = 0;

	int32 RecvBytes = 0;
	uint16 PacketSize = 0;
	while (ServerSocket->HasPendingData(Pending))
	{
		if (ServerSocket->Recv((uint8*)&PacketSize, sizeof(PacketSize), RecvBytes) || RecvBytes == 0)
		{
			Disconnect();
			break;
		}

		if (RecvBytes == 2)
		{
			break;
		}
	}



	//Body
	while (ServerSocket->HasPendingData(Pending))
	{
		if (ServerSocket->Recv(RecvBuffer.GetData(), PacketSize, RecvBytes) || RecvBytes == 0)
		{
			Disconnect();
			break;
		}

		if (RecvBytes == PacketSize)
		{
			break;
		}
	}

	if (RecvBytes > 0)
	{
		RecvBuffer.SetNum(RecvBytes);
		DispatchPacket();
		RecvBuffer.Reset();
	}


}

bool UTCPClientSubsystem::SendAll(const uint8* Body, uint32 BodyLength)
{
	TArray<uint8> Packet;
	//[][] [][][][][][][]
	Packet.Reserve(2 + BodyLength);
	//[][] -> Header
	FMemory::Memcpy(Packet.GetData(), &BodyLength, 2);
	//Packet.Add((uint8)((BodyLength >> 8) & 0xFF));
	//Packet.Add((uint8)((BodyLength) & 0xFF));
	Packet.SetNum(2);
	Packet.Append(Body, BodyLength);

	int32 SentTotalBytes = 0;
	while (SentTotalBytes < Packet.Num())
	{
		int32 SentBytes = 0;
		if (ServerSocket->Send(Packet.GetData() + SentTotalBytes, Packet.Num() - SentTotalBytes, SentBytes) || SentBytes < 0)
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

		FString Message = LoginData->message() ? UTF8_TO_TCHAR(LoginData->message()) : FString();

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