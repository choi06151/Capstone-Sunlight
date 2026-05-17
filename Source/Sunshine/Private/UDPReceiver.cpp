// Fill out your copyright notice in the Description page of Project Settings.

#include "UDPReceiver.h"
// UDP 통신을 위한 소켓 관련 헤더들
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"

AUDPReceiver::AUDPReceiver()
{
    // 매 프레임 Tick()을 호출하도록 설정
    PrimaryActorTick.bCanEverTick = true;
    ListenSocket = nullptr;
}

void AUDPReceiver::BeginPlay()
{
    Super::BeginPlay();

    // 포트 5005를 열고 Python에서 보내는 데이터를 기다림
    FIPv4Endpoint Endpoint(FIPv4Address::Any, 5005);
    ListenSocket = FUdpSocketBuilder(TEXT("HandTrackerSocket"))
        .AsNonBlocking()            // 데이터 없어도 멈추지 않고 바로 넘어감
        .AsReusable()               // 포트 재사용 허용
        .BoundToEndpoint(Endpoint)  // 5005 포트에 바인딩
        .WithReceiveBufferSize(2 * 1024 * 1024); // 수신 버퍼 2MB

    UE_LOG(LogTemp, Warning, TEXT("[UDP] 소켓 열림 - 포트 5005 대기중"));
}

void AUDPReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    // 플레이 종료 시 소켓을 닫아 포트 찌꺼기가 남지 않도록 정리
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

    if (!ListenSocket) return;

    TSharedRef<FInternetAddr> Sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    uint32 Size;

    // 한 프레임에 최대 10번만 읽도록 제한 (무한루프 방지)
    int32 MaxReadCount = 0;

    while (ListenSocket->HasPendingData(Size) && MaxReadCount < 10)
    {
        MaxReadCount++;

        // 수신 버퍼 준비 (최대 UDP 패킷 크기 65507바이트로 제한)
        TArray<uint8> ReceivedData;
        ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));
        int32 Read = 0;

        bool bSuccess = ListenSocket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), Read, *Sender);
        if (!bSuccess || Read <= 0) break;

        // 받은 바이트 수만큼 자르고 문자열 끝에 null 문자 추가
        ReceivedData.SetNum(Read);
        ReceivedData.Add(0);

        // 바이트 배열을 언리얼 FString으로 변환
        FString ReceivedString = FString(UTF8_TO_TCHAR(ReceivedData.GetData()));

        // 문자열 앞뒤 공백 및 줄바꿈 제거 (파싱 오류 방지)
        ReceivedString.TrimStartAndEndInline();

        // 쉼표(,) 기준으로 문자열 분리
        // 예: "R,0.5,0.4,0.1,OPEN,45.0" → 6개 조각
        TArray<FString> ParsedArray;
        ReceivedString.ParseIntoArray(ParsedArray, TEXT(","), true);

        // 6개 값이 정상적으로 들어왔을 때만 처리
        // 형식: HandType, X, Y, Z, Gesture, Angle
        if (ParsedArray.Num() == 6)
        {
            FString HandType = ParsedArray[0];          // L 또는 R
            float X = FCString::Atof(*ParsedArray[1]); // 손 위치 X (0~1)
            float Y = FCString::Atof(*ParsedArray[2]); // 손 위치 Y (0~1)
            float Z = FCString::Atof(*ParsedArray[3]); // 손 위치 Z (깊이)
            FString Gesture = ParsedArray[4];           // OPEN / FIST / POINT
            float Angle = FCString::Atof(*ParsedArray[5]); // 손목 각도 (FIST일 때만 유효)
            FVector FingerPos(X, Y, Z);

            UE_LOG(LogTemp, Log, TEXT("[UDP] Hand: %s | Gesture: %s | Angle: %.1f | Pos: (%.2f, %.2f, %.2f)"),
                *HandType, *Gesture, Angle, X, Y, Z);

            // Blueprint로 데이터 전달 (OnHandDataReceived는 Blueprint에서 구현)
            OnHandDataReceived(HandType, FingerPos, Gesture, Angle);
        }
        else
        {
            // 파싱 실패 시 원본 데이터와 조각 수를 로그로 확인
            UE_LOG(LogTemp, Warning, TEXT("[UDP] 파싱 실패 - 조각 개수: %d | 원본: %s"),
                ParsedArray.Num(), *ReceivedString);
        }
    }
}