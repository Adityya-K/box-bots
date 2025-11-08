import cv2 
from ultralytics import YOLO
import math
import mediapipe as mp
import time

stream = cv2.VideoCapture(0)
stream.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M', 'J', 'P', 'G'))
stream.set(3, 640)
stream.set(4, 480)

model = YOLO("yolo11m.pt")

mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles
mp_hands = mp.solutions.hands

def print_result(result: mp.tasks.vision.GestureRecognizerResult, unused_output_image: mp.Image, timestamp_ms: int):
    if len(result.gestures) == 2:
        print(result.gestures[0])
        text = str(result.gestures[0][0].category_name)
        org = (50, 50) # Bottom-left corner of the text (x, y)
        font = cv2.FONT_HERSHEY_SIMPLEX
        font_scale = 1
        color = (0, 255, 0) # Green color in BGR format
        thickness = 2
        text2 = str(result.gestures[1][0].category_name)
        org2 = (300, 50) # bottom-left corner of the text (x, y)
        cv2.putText(frame, text, org, font, font_scale, color, thickness)
        cv2.putText(frame, text2, org2, font, font_scale, color, thickness)
    elif len(result.gestures) == 1:
        text = str(result.gestures[0][0].category_name)
        org = (50, 50) # Bottom-left corner of the text (x, y)
        font = cv2.FONT_HERSHEY_SIMPLEX
        font_scale = 1
        color = (0, 255, 0) # Green color in BGR format
        thickness = 2
        cv2.putText(frame, text, org, font, font_scale, color, thickness)





base_options = mp.tasks.BaseOptions(model_asset_path="gesture_recognizer.task")
options = mp.tasks.vision.GestureRecognizerOptions(base_options=base_options,
                                        running_mode=mp.tasks.vision.RunningMode.LIVE_STREAM,
                                        num_hands=2,
                                        result_callback=print_result)
recognizer = mp.tasks.vision.GestureRecognizer.create_from_options(options)


if not stream.isOpened():
    print("No Stream :(")
    exit() 

classNames = ["person", "bicycle", "car", "motorbike", "aeroplane", "bus", "train", "truck", "boat",
              "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
              "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella",
              "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat",
              "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
              "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli",
              "carrot", "hot dog", "pizza", "donut", "cake", "chair", "sofa", "pottedplant", "bed",
              "diningtable", "toilet", "tvmonitor", "laptop", "mouse", "remote", "keyboard", "cell phone",
              "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors",
              "teddy bear", "hair drier", "toothbrush"
              ]

time_stamp = time.time_ns() // 1000000
while (True):
    ret, frame = stream.read()
    if not ret:
        print("No more stream")
        exit()

    results = model(frame, stream=True)

    for r in results:
        boxes = r.boxes

        for box in boxes:
            # class name
            cls = int(box.cls[0])
            print("Class name -->", classNames[cls])

            if classNames[cls] != "person":
                continue

            # bounding box
            x1, y1, x2, y2 = box.xyxy[0]
            x1, y1, x2, y2 = int(x1), int(y1), int(x2), int(y2) # convert to int values

            # put box in cam
            cv2.rectangle(frame, (x1, y1), (x2, y2), (255, 0, 255), 3)

            # confidence
            confidence = math.ceil((box.conf[0]*100))/100
            print("Confidence --->",confidence)

            # object details
            org = [x1, y1]
            font = cv2.FONT_HERSHEY_SIMPLEX
            fontScale = 1
            color = (255, 0, 0)
            thickness = 2

            cv2.putText(frame, classNames[cls], org, font, fontScale, color, thickness)

    with mp_hands.Hands(
        model_complexity=0,
        min_detection_confidence=0.5,
        min_tracking_confidence=0.5) as hands:
        if not ret:
            print("Ignoring empty camera frame.")
        # If loading a video, use 'break' instead of 'continue'.
            continue

        # To improve performance, optionally mark the image as not writeable to
        # pass by reference.
        frame.flags.writeable = False
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = hands.process(frame)

        # Draw the hand annotations on the image.
        frame.flags.writeable = True
        frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=frame)
        if results.multi_hand_landmarks:
            for hand_landmarks in results.multi_hand_landmarks:
                mp_drawing.draw_landmarks(
                    frame,
                    hand_landmarks,
                    mp_hands.HAND_CONNECTIONS,
                    mp_drawing_styles.get_default_hand_landmarks_style(),
                    mp_drawing_styles.get_default_hand_connections_style())

        recognizer.recognize_async(mp_image, time_stamp)
        time_stamp += 100


    cv2.imshow("Webcam", frame)
    if cv2.waitKey(1) == ord('q'):
        break 

stream.release()
cv2.destroyAllWindows()
