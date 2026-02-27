from picamera2 import Picamera2
import time
import cv2
import numpy as np
import RPi.GPIO as GPIO

# Configuration
ENABLE_DISPLAY = False  # Set to False for headless operation (faster!)
CAMERA_WIDTH = 640
CAMERA_HEIGHT = 360

# GPIO pins for Arduino communication (BCM numbering)
CMD_BIT0 = 17  # Physical pin 11 → Arduino Pin 4
CMD_BIT1 = 27  # Physical pin 13 → Arduino Pin 6
CMD_BIT2 = 22  # Physical pin 15 → Arduino Pin 7

# Command encoding (3-bit binary)
CMD_STOP = 0     # 000
CMD_FORWARD = 1  # 001
CMD_BACKWARD = 2 # 010
CMD_LEFT = 3     # 011
CMD_RIGHT = 4    # 100

GPIO.setmode(GPIO.BCM)
GPIO.setup(CMD_BIT0, GPIO.OUT)
GPIO.setup(CMD_BIT1, GPIO.OUT)
GPIO.setup(CMD_BIT2, GPIO.OUT)

# Initialize - all low
GPIO.output(CMD_BIT0, False)
GPIO.output(CMD_BIT1, False)
GPIO.output(CMD_BIT2, False)

def send_command(cmd):
    """Send 3-bit command to Arduino via GPIO"""
    GPIO.output(CMD_BIT0, bool(cmd & 0b001))
    GPIO.output(CMD_BIT1, bool(cmd & 0b010))
    GPIO.output(CMD_BIT2, bool(cmd & 0b100))
    print(f"Sent command: {cmd}")

# Setup camera with picamera2
camera = Picamera2()
camera_config = camera.create_preview_configuration(
    main={"size": (CAMERA_WIDTH, CAMERA_HEIGHT), "format": "RGB888"},
    transform={"vflip": True, "hflip": True}  # Rotation 180
)
camera.configure(camera_config)
camera.start()
time.sleep(1)  # Camera warm-up

center_x = CAMERA_WIDTH // 2  # Center of image

print("Starting line following with GPIO control...")
send_command(CMD_STOP)  # Start with motors stopped

frame_count = 0
start_time = time.time()
last_status_time = start_time

try:
    while True:
        # Capture frame
        image = camera.capture_array()
        roi = image[200:250, 0:639]
        Blackline = cv2.inRange(roi, (0, 0, 0), (50, 50, 50))
        kernel = np.ones((3, 3), np.uint8)
        Blackline = cv2.erode(Blackline, kernel, iterations=5)
        Blackline = cv2.dilate(Blackline, kernel, iterations=9)	
        contours, hierarchy = cv2.findContours(Blackline.copy(), cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)	
        
        command = CMD_STOP  # Default: Stop
        command_text = "STOP"
        
        if len(contours) > 0:
            x, y, w, h = cv2.boundingRect(contours[0])
            line_center = x + (w / 2)
            
            # Calculate where line is relative to center
            error = line_center - center_x
            
            # Send commands based on line position
            if abs(error) < 40:
                command = CMD_FORWARD
                command_text = "FORWARD"
            elif error < -40:
                command = CMD_LEFT
                command_text = "LEFT"
            else:
                command = CMD_RIGHT
                command_text = "RIGHT"
            
            # Send to Arduino via GPIO
            send_command(command)
            
            # Draw line on image (for display mode)
            if ENABLE_DISPLAY:
                cv2.line(image, (int(line_center), 200), (int(line_center), 250), (255, 0, 0), 3)
        
        # Update frame counter
        frame_count += 1
        
        # Print status every 2 seconds (headless mode)
        if not ENABLE_DISPLAY:
            current_time = time.time()
            if current_time - last_status_time >= 2.0:
                fps = frame_count / (current_time - start_time)
                print(f"[{current_time - start_time:.1f}s] FPS: {fps:.1f} | CMD: {command_text} | Contours: {len(contours)}")
                last_status_time = current_time
            
        # Display command on screen (display mode only)
        if ENABLE_DISPLAY:
            cv2.putText(image, f"CMD: {command_text}", (10, 30), 
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            cv2.imshow("Line Following", image)	
            key = cv2.waitKey(1) & 0xFF	
            if key == ord("q"):
                break
        else:
            # Minimal delay for headless mode (maximize FPS)
            time.sleep(0.001)

except KeyboardInterrupt:
    print("\nStopping...")

finally:
    # Cleanup
    print("Cleaning up...")
    send_command(CMD_STOP)  # Stop motors
    camera.stop()
    GPIO.cleanup()
    if ENABLE_DISPLAY:
        cv2.destroyAllWindows()
    print("Done!")
    # Print final statistics
    total_time = time.time() - start_time
    avg_fps = frame_count / total_time if total_time > 0 else 0
    print(f"Session complete: {frame_count} frames in {total_time:.1f}s (avg {avg_fps:.1f} FPS)")
    