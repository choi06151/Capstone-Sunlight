import cv2
import mediapipe as mp
import socket
import time
import math
from collections import deque

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

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

# flip 때문에 좌우가 반대로 나오면 여기만 바꾸면 됨
def label_to_short(label):
    return "L" if label == "Left" else "R"
    # return "R" if label == "Left" else "L"

hands_left = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=2,
    min_detection_confidence=0.7,
    min_tracking_confidence=0.7
)

hands_under = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=2,
    min_detection_confidence=0.7,
    min_tracking_confidence=0.7
)

cap_left = cv2.VideoCapture(1)
cap_under = cv2.VideoCapture(0)

hand_data = {
    "L": {
        "x": 0.5, "y": 0.5, "z": 0.5,
        "gesture": "NONE",
        "angle": 0.0,
        "sx": deque(maxlen=5),
        "sy": deque(maxlen=5),
        "sz": deque(maxlen=5)
    },
    "R": {
        "x": 0.5, "y": 0.5, "z": 0.5,
        "gesture": "NONE",
        "angle": 0.0,
        "sx": deque(maxlen=5),
        "sy": deque(maxlen=5),
        "sz": deque(maxlen=5)
    }
}

print("카메라를 켜는 중입니다...")
print("3초 후 전송 시작! 언리얼을 재생하세요!")
time.sleep(3)
print("전송 시작!")

while cap_left.isOpened() and cap_under.isOpened():

    success_left, image_left = cap_left.read()
    success_under, image_under = cap_under.read()

    if not success_left or not success_under:
        print("카메라를 찾을 수 없습니다.")
        break

    image_left = cv2.flip(image_left, 1)
    rgb_left = cv2.cvtColor(image_left, cv2.COLOR_BGR2RGB)
    results_left = hands_left.process(rgb_left)

    if results_left.multi_hand_landmarks and results_left.multi_handedness:
        for hand_landmarks, handedness in zip(
            results_left.multi_hand_landmarks,
            results_left.multi_handedness
        ):
            label = label_to_short(handedness.classification[0].label)
            lm = hand_landmarks.landmark

            hand_data[label]["z"] = lm[8].y
            hand_data[label]["gesture"] = get_gesture(lm)

            if hand_data[label]["gesture"] == "FIST":
                hand_data[label]["angle"] = get_wrist_angle(lm) - (-67) + 1
            else:
                hand_data[label]["angle"] = 1.0

            mp_draw.draw_landmarks(image_left, hand_landmarks, mp_hands.HAND_CONNECTIONS)

            cv2.putText(
                image_left,
                f"{label} {hand_data[label]['gesture']}",
                (10, 30 if label == "L" else 60),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2
            )

    image_under = cv2.flip(image_under, 1)
    rgb_under = cv2.cvtColor(image_under, cv2.COLOR_BGR2RGB)
    results_under = hands_under.process(rgb_under)

    if results_under.multi_hand_landmarks and results_under.multi_handedness:
        for hand_landmarks, handedness in zip(
            results_under.multi_hand_landmarks,
            results_under.multi_handedness
        ):
            label = label_to_short(handedness.classification[0].label)
            lm = hand_landmarks.landmark

            hand_data[label]["x"] = lm[8].x
            hand_data[label]["y"] = lm[8].y

            mp_draw.draw_landmarks(image_under, hand_landmarks, mp_hands.HAND_CONNECTIONS)

            cv2.putText(
                image_under,
                f"{label}",
                (10, 30 if label == "L" else 60),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2
            )

    for label in ["L", "R"]:
        hand_data[label]["sx"].append(hand_data[label]["x"])
        hand_data[label]["sy"].append(hand_data[label]["y"])
        hand_data[label]["sz"].append(hand_data[label]["z"])

        hand_data[label]["avg_x"] = sum(hand_data[label]["sx"]) / len(hand_data[label]["sx"])
        hand_data[label]["avg_y"] = sum(hand_data[label]["sy"]) / len(hand_data[label]["sy"])
        hand_data[label]["avg_z"] = sum(hand_data[label]["sz"]) / len(hand_data[label]["sz"])

    data_string = (
        f"L,{hand_data['L']['avg_x']:.4f},{hand_data['L']['avg_y']:.4f},{hand_data['L']['avg_z']:.4f},"
        f"{hand_data['L']['gesture']},{hand_data['L']['angle']:.1f};"
        f"R,{hand_data['R']['avg_x']:.4f},{hand_data['R']['avg_y']:.4f},{hand_data['R']['avg_z']:.4f},"
        f"{hand_data['R']['gesture']},{hand_data['R']['angle']:.1f}"
    )

    print(data_string)

    sock.sendto(data_string.encode(), (UDP_IP, UDP_PORT))

    cv2.imshow("Left Camera", image_left)
    cv2.imshow("Under Camera", image_under)

    if cv2.waitKey(5) & 0xFF == 27:
        break

cap_left.release()
cap_under.release()
cv2.destroyAllWindows()