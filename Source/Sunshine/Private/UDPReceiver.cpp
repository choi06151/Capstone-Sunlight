// Fill out your copyright notice in the Description page of Project Settings.


#include "UDPReceiver.h"
// 통신 필수 라이브러리 3개 추가
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"

// Sets default values
AUDPReceiver::AUDPReceiver()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    ListenSocket = nullptr;
}

// Called when the game starts or when spawned
void AUDPReceiver::BeginPlay()
{
    Super::BeginPlay();

    // 내 컴퓨터(Any)의 5000번 포트를 열고 파이썬을 기다립니다.
    FIPv4Endpoint Endpoint(FIPv4Address::Any, 5005);
    ListenSocket = FUdpSocketBuilder(TEXT("HandTrackerSocket"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToEndpoint(Endpoint)
        .WithReceiveBufferSize(2 * 1024 * 1024);
}

void AUDPReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    // 에디터 플레이(PIE)를 끌 때 소켓이 찌꺼기로 남지 않게 깔끔하게 폭파
    if (ListenSocket)
    {
        ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);

        ListenSocket = nullptr;
    }
}

// Called every frame
void AUDPReceiver::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ListenSocket) return;

    TSharedRef<FInternetAddr> Sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    uint32 Size;

    // 💡 [안전장치 1] 한 프레임에 최대 10번만 읽도록 제한 카운터 생성
    int32 MaxReadCount = 0;

    // 파이썬에서 데이터가 날아왔는지 매 프레임 확인
    while (ListenSocket->HasPendingData(Size) && MaxReadCount < 10)
    {
        MaxReadCount++; // 한 바퀴 돌 때마다 카운트 1 증가

        TArray<uint8> ReceivedData;
        ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));
        int32 Read = 0;

        bool bSuccess = ListenSocket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), Read, *Sender);

        if (!bSuccess || Read <= 0)
        {
            break;
        }
        // 받은 데이터를 언리얼이 읽을 수 있는 문자열로 변환
        ReceivedData.SetNum(Read);
        ReceivedData.Add(0); // 문자열 끝맺음
        FString ReceivedString = FString(UTF8_TO_TCHAR(ReceivedData.GetData()));

        // 💡 [수진님 추가 코드] 언리얼 화면에 귀에 들리는 모든 것을 노란색으로 무조건 띄워라!
        if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("Raw Data: %s"), *ReceivedString)); }

        // 1. 파이썬이 보낸 문자열 끝에 묻어있는 '엔터(줄바꿈)'나 '공백'을 깔끔하게 지우기 (★핵심)
        ReceivedString.TrimStartAndEndInline();

        // "L,0.5,0.4,0.1" 같은 문자열을 쉼표(,) 기준으로 4조각으로 자르기
        TArray<FString> ParsedArray;

        // 2. 쉼표를 기준으로 자르기
        ReceivedString.ParseIntoArray(ParsedArray, TEXT(","), true);

        // 💡 2. [수진님 추가 코드] 언리얼이 이 데이터를 몇 조각으로 잘랐는지 초록색으로 띄워보기!
        if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, FString::Printf(TEXT("조각 개수: %d"), ParsedArray.Num())); }

        if (ParsedArray.Num() == 6)  // ← 4에서 6으로
        {
            FString HandType = ParsedArray[0];
            float X = FCString::Atof(*ParsedArray[1]);
            float Y = FCString::Atof(*ParsedArray[2]);
            float Z = FCString::Atof(*ParsedArray[3]);
            FString Gesture = ParsedArray[4];        // ← 추가
            float Angle = FCString::Atof(*ParsedArray[5]); // ← 추가
            FVector FingerPos(X, Y, Z);

            if (GEngine) {
                GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Cyan,
                    FString::Printf(TEXT("HandType: %s | Gesture: %s | Angle: %.1f"), *HandType, *Gesture, Angle));
            }

            OnHandDataReceived(HandType, FingerPos, Gesture, Angle); // ← 인자 4개
        }
    }
}

