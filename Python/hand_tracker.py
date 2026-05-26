import cv2
import mediapipe as mp
import socket
import time
import math
from collections import deque

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

smooth_x = deque(maxlen=5)
smooth_y = deque(maxlen=5)
smooth_z = deque(maxlen=5)

def get_gesture(lm):
    index_up  = lm[8].y  < lm[6].y
    middle_up = lm[12].y < lm[10].y
    ring_up   = lm[16].y < lm[14].y
    pinky_up  = lm[20].y < lm[18].y
    if not index_up and not middle_up and not ring_up and not pinky_up:
        return "FIST"
    elif index_up and middle_up and ring_up and pinky_up:
        return "OPEN"
    else:
        return "POINT"

def get_wrist_angle(lm):
    dx = lm[9].x - lm[0].x
    dy = lm[9].y - lm[0].y
    return round(math.degrees(math.atan2(dy, dx)), 1)

mp_hands = mp.solutions.hands
hands_front = mp_hands.Hands(static_image_mode=False, max_num_hands=1, min_detection_confidence=0.7)
hands_top = mp_hands.Hands(static_image_mode=False, max_num_hands=1, min_detection_confidence=0.7)
mp_draw = mp.solutions.drawing_utils

cap_front = cv2.VideoCapture(1)  # 정면 웹캠
cap_top = cv2.VideoCapture(0)    # iVCam (위에서 아래)

print("카메라를 켜는 중입니다...")
print("3초 후 전송 시작! 언리얼을 재생하세요!")
time.sleep(3)
print("전송 시작!")

# 이전 값 저장용 (한쪽 카메라 인식 안될 때 유지)
x_val, y_val, z_val = 0.5, 0.2, 0.5
gesture = "POINT"
angle = 0.0
hand_label = "R"

while cap_front.isOpened() and cap_top.isOpened():
    success_front, image_front = cap_front.read()
    success_top, image_top = cap_top.read()

    if not success_front or not success_top:
        print("카메라를 찾을 수 없습니다.")
        break

    # 정면 카메라 처리 (좌우, 상하)
    image_front = cv2.flip(image_front, 1)
    rgb_front = cv2.cvtColor(image_front, cv2.COLOR_BGR2RGB)
    results_front = hands_front.process(rgb_front)

    if results_front.multi_hand_landmarks and results_front.multi_handedness:
        for hand_landmarks, handedness in zip(results_front.multi_hand_landmarks, results_front.multi_handedness):
            hand_label = handedness.classification[0].label[0]
            lm = hand_landmarks.landmark
            idx_finger = lm[8]
            x_val = idx_finger.x  # 좌우
            z_val = idx_finger.y  # 상하
            gesture = get_gesture(lm)
            angle = get_wrist_angle(lm)-(-67)+1 if gesture == "FIST" else 1.0
            mp_draw.draw_landmarks(image_front, hand_landmarks, mp_hands.HAND_CONNECTIONS)
            cv2.putText(image_front, f"{hand_label}: {gesture}",
                        (10, 30 if hand_label == "L" else 60),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

    # 위 카메라 처리 (앞뒤)
    image_top = cv2.flip(image_top, 1)
    rgb_top = cv2.cvtColor(image_top, cv2.COLOR_BGR2RGB)
    results_top = hands_top.process(rgb_top)

    if results_top.multi_hand_landmarks:
        for hand_landmarks in results_top.multi_hand_landmarks:
            lm = hand_landmarks.landmark
            # 위에서 아래로 찍을 때 Y값이 앞뒤
            y_val = lm[8].y
            print(f"y_val(앞뒤): {y_val:.4f}")  # 추가
            mp_draw.draw_landmarks(image_top, hand_landmarks, mp_hands.HAND_CONNECTIONS)

    # 스무딩
    smooth_x.append(x_val)
    smooth_y.append(y_val)
    smooth_z.append(z_val)

    x_val = sum(smooth_x) / len(smooth_x)
    y_val = sum(smooth_y) / len(smooth_y)
    z_val = sum(smooth_z) / len(smooth_z)

    # UDP 전송
    data_string = f"{hand_label},{x_val},{y_val},{z_val},{gesture},{angle}"
    sock.sendto(data_string.encode(), (UDP_IP, UDP_PORT))

    cv2.imshow("Front Camera", image_front)
    cv2.imshow("Top Camera", image_top)

    if cv2.waitKey(5) & 0xFF == 27:
        break

cap_front.release()
cap_top.release()
cv2.destroyAllWindows()