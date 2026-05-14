import cv2
import mediapipe as mp
import socket
import time
import math

# 언리얼 엔진이 받을 통신 주소와 포트 (로컬호스트)
UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 제스처 판별 함수 (주먹/오픈/검지)
def get_gesture(lm):
    # 각 손가락 끝(8,12,16,20)이 관절(6,10,14,18)보다 위에 있으면 펴진 것
    index_up  = lm[8].y  < lm[6].y
    middle_up = lm[12].y < lm[10].y
    ring_up   = lm[16].y < lm[14].y
    pinky_up  = lm[20].y < lm[18].y

    # 네 손가락 모두 접혀있으면 주먹
    if not index_up and not middle_up and not ring_up and not pinky_up:
        return "FIST"
    # 네 손가락 모두 펴져있으면 오픈
    elif index_up and middle_up and ring_up and pinky_up:
        return "OPEN"
    # 그 외 (검지만 펴기 등)
    else:
        return "POINT"

# 손목 각도 계산 함수 (주먹 쥐고 돌릴 때 테마 속도 조절용)
def get_wrist_angle(lm):
    # 손목(0)과 중지 뿌리(9) 사이의 각도를 계산
    dx = lm[9].x - lm[0].x
    dy = lm[9].y - lm[0].y
    return round(math.degrees(math.atan2(dy, dx)), 1)

# MediaPipe 손 인식 초기화
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(
    static_image_mode=False,      # 실시간 영상 모드
    max_num_hands=2,              # 최대 양손 인식
    min_detection_confidence=0.7  # 인식 정확도 70% 이상만 허용
)
mp_draw = mp.solutions.drawing_utils

# 기본 웹캠(0번) 켜기
cap = cv2.VideoCapture(0)
print("카메라를 켜는 중입니다...")
print("3초 후 전송 시작! 언리얼을 재생하세요!")
time.sleep(3)  # 언리얼 소켓이 열릴 때까지 대기
print("전송 시작!")

while cap.isOpened():
    success, image = cap.read()
    if not success:
        print("카메라를 찾을 수 없습니다.")
        break

    # 이미지를 좌우 반전(거울 모드)하고 RGB로 변환
    image = cv2.flip(image, 1)
    rgb_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    # 손가락 관절 추적
    results = hands.process(rgb_image)

    # 손가락 뼈대와 왼손/오른손 정보를 같이 가져옵니다
    if results.multi_hand_landmarks and results.multi_handedness:
        for hand_landmarks, handedness in zip(results.multi_hand_landmarks, results.multi_handedness):

            # 'Left' 또는 'Right'의 첫 글자(L 또는 R) 따오기
            hand_label = handedness.classification[0].label[0]
            lm = hand_landmarks.landmark

            # 검지 손가락 끝(8번 랜드마크) 좌표 추출 → 나이아가라 햇빛 위치용
            idx_finger = lm[8]

            # 현재 제스처 판별 (FIST / OPEN / POINT)
            gesture = get_gesture(lm)

            # 주먹일 때만 손목 각도 계산, 아니면 0 (테마 속도 조절용) (-67기준으로 0으로 만들기)
            angle = get_wrist_angle(lm)-(-67) if gesture == "FIST" else 0.0

            # 6조각 데이터 조립 후 언리얼로 전송
            # 형식: L/R, X, Y, Z, 제스처, 각도
            data_string = f"{hand_label},{idx_finger.x},{idx_finger.y},{idx_finger.z},{gesture},{angle}"
            sock.sendto(data_string.encode(), (UDP_IP, UDP_PORT))

            # 파이썬 화면에 손가락 뼈대 그리기
            mp_draw.draw_landmarks(image, hand_landmarks, mp_hands.HAND_CONNECTIONS)

            # 화면 좌측 상단에 손 구분 및 제스처 표시
            # 왼손(L)은 위쪽, 오른손(R)은 아래쪽에 표시
            cv2.putText(image, f"{hand_label}: {gesture}",
                        (10, 30 if hand_label == "L" else 60),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

    # 화면 출력
    cv2.imshow("Python Hand Tracker", image)

    # ESC 키를 누르면 종료
    if cv2.waitKey(5) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()