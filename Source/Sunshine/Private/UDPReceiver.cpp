#include "UDPReceiver.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"
#include "IPAddress.h"

AUDPReceiver::AUDPReceiver()
{
    PrimaryActorTick.bCanEverTick = true;
    ListenSocket = nullptr;
}

void AUDPReceiver::BeginPlay()
{
    Super::BeginPlay();

    FIPv4Endpoint Endpoint(FIPv4Address::Any, 5005);

    ListenSocket = FUdpSocketBuilder(TEXT("HandTrackerSocket"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToEndpoint(Endpoint)
        .WithReceiveBufferSize(2 * 1024 * 1024);

    UE_LOG(LogTemp, Warning, TEXT("[UDP] 소켓 열림 - 포트 5005 대기중"));
}

void AUDPReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (ListenSocket)
    {
        ListenSocket->Close();

        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);

        ListenSocket = nullptr;

        UE_LOG(LogTemp, Warning, TEXT("[UDP] 소켓 닫힘"));
    }
}

void AUDPReceiver::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ListenSocket)
    {
        return;
    }

    TSharedRef<FInternetAddr> Sender =
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

    uint32 Size;
    int32 MaxReadCount = 0;

    while (ListenSocket->HasPendingData(Size) && MaxReadCount < 10)
    {
        MaxReadCount++;

        TArray<uint8> ReceivedData;
        ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));

        int32 Read = 0;

        bool bSuccess = ListenSocket->RecvFrom(
            ReceivedData.GetData(),
            ReceivedData.Num(),
            Read,
            *Sender
        );

        if (!bSuccess || Read <= 0)
        {
            break;
        }

        ReceivedData.SetNum(Read);
        ReceivedData.Add(0);

        FString ReceivedString =
            FString(UTF8_TO_TCHAR(ReceivedData.GetData()));

        ReceivedString.TrimStartAndEndInline();

        TArray<FString> HandPackets;
        ReceivedString.ParseIntoArray(
            HandPackets,
            TEXT(";"),
            true
        );

        for (const FString& HandPacket : HandPackets)
        {
            FString TrimmedPacket = HandPacket;
            TrimmedPacket.TrimStartAndEndInline();

            TArray<FString> ParsedArray;

            TrimmedPacket.ParseIntoArray(
                ParsedArray,
                TEXT(","),
                true
            );

            if (ParsedArray.Num() == 6)
            {
                FString HandType = ParsedArray[0];

                float X = FCString::Atof(*ParsedArray[1]);
                float Y = FCString::Atof(*ParsedArray[2]);
                float Z = FCString::Atof(*ParsedArray[3]);

                FString Gesture = ParsedArray[4];

                float Angle = FCString::Atof(*ParsedArray[5]);

                FVector FingerPos(X, Y, Z);

                UE_LOG(
                    LogTemp,
                    Log,
                    TEXT("[UDP] Hand:%s Gesture:%s Angle:%.1f Pos:(%.3f %.3f %.3f)"),
                    *HandType,
                    *Gesture,
                    Angle,
                    X,
                    Y,
                    Z
                );

                if (HandType.Equals(TEXT("L")))
                {
                    OnLeftHandDataReceived(
                        FingerPos,
                        Gesture,
                        Angle
                    );
                }
                else if (HandType.Equals(TEXT("R")))
                {
                    OnRightHandDataReceived(
                        FingerPos,
                        Gesture,
                        Angle
                    );
                }
            }
            else
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("[UDP] 손 데이터 파싱 실패 | Count:%d | Packet:%s"),
                    ParsedArray.Num(),
                    *TrimmedPacket
                );
            }
        }
    }
}