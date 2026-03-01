import os
from picamera2 import Picamera2
import time
from datetime import datetime
import cv2
import numpy as np
import RPi.GPIO as GPIO

# Configuration
MAX_DEBUG_FRAMES = 50  # Limit number of saved frames to prevent memory issues
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

# Setup camera with picamera2
camera = Picamera2()
camera_config = camera.create_preview_configuration(
    main={"size": (CAMERA_WIDTH, CAMERA_HEIGHT), "format": "RGB888"}
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
# add frame and time counters for debugging
images = []
last_turn_command = CMD_STOP  # Track last turn direction for line search

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
        line_center = None
        
        if len(contours) > 0:
            # Find the largest contour (most likely the line)
            largest_contour = max(contours, key=cv2.contourArea)
            x, y, w, h = cv2.boundingRect(largest_contour)
            line_center = x + (w / 2)
            
            # Calculate where line is relative to center
            error = line_center - center_x
            
            # Send commands based on line position (wider forward threshold)
            if abs(error) < 80:  # Increased from 40 to 80 for smoother forward motion
                command = CMD_FORWARD
                command_text = "FORWARD"
                last_turn_command = CMD_FORWARD  # Reset search direction
            elif error < -80:
                command = CMD_LEFT
                command_text = "LEFT"
                last_turn_command = CMD_LEFT  # Remember we were turning left
            else:
                command = CMD_RIGHT
                command_text = "RIGHT"
                last_turn_command = CMD_RIGHT  # Remember we were turning right
        
            # Send to Arduino via GPIO 
            send_command(command)
        else:
            # No line detected - search based on last known direction
            if last_turn_command == CMD_LEFT:
                command = CMD_LEFT
                command_text = "SEARCH LEFT"
                send_command(command)
            elif last_turn_command == CMD_RIGHT:
                command = CMD_RIGHT
                command_text = "SEARCH RIGHT"
                send_command(command)
            else:
                # Was going forward, stop to avoid running away
                send_command(CMD_STOP)
                command_text = "STOP (No line)"
        
        # Update frame counter
        frame_count += 1

        # Print status every 2 seconds
        current_time = time.time()
        if current_time - last_status_time >= 2.0:
            fps = frame_count / (current_time - start_time)
            print(f"[{current_time - start_time:.1f}s] FPS: {fps:.1f} | CMD: {command_text} | Contours: {len(contours)}")
            last_status_time = current_time

            # Draw visualization only on saved frames
            debug_img = image.copy()
            # Draw ROI rectangle
            cv2.rectangle(debug_img, (0, 200), (639, 250), (0, 165, 255), 2)
            if line_center is not None:
                cv2.line(debug_img, (int(line_center), 200), (int(line_center), 250), (255, 0, 0), 3)
                # Add error value
                error_val = int(line_center - center_x)
                cv2.putText(debug_img, f"Error: {error_val}px", (10, 70), 
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)
            cv2.line(debug_img, (center_x, 200), (center_x, 250), (0, 255, 0), 2)
            cv2.putText(debug_img, f"CMD: {command_text}", (10, 30), 
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            
            # Save image with metadata for filename
            images.append((debug_img, Blackline.copy(), last_status_time, command_text, len(contours)))
            if len(images) > MAX_DEBUG_FRAMES:
                images.pop(0)  # Remove oldest            

        # Minimal delay for headless mode (maximize FPS)
        time.sleep(0.001)
            

except KeyboardInterrupt:
    print("\nStopping...")

finally:
    # save all captured images for debugging into a folder
    print(f"Saving {len(images)} captured frames for debugging...")
    os.makedirs("debug_frames", exist_ok=True)
    os.makedirs("debug_frames/binary_mask", exist_ok=True)
    # clear old debug frames before saving new ones
    for file in os.listdir("debug_frames"):
        file_path = os.path.join("debug_frames", file)
        if os.path.isfile(file_path):
            os.remove(file_path)  # Clear old debug frames
    # Clear old binary masks
    for file in os.listdir("debug_frames/binary_mask"):
        file_path = os.path.join("debug_frames/binary_mask", file)
        if os.path.isfile(file_path):
            os.remove(file_path)
    
    for idx, (img, binary_mask, timestamp, cmd_txt, contour_count) in enumerate(images):
        # Convert timestamp to readable format
        time_str = datetime.fromtimestamp(timestamp).strftime("%H-%M-%S")
        # Save main debug frame
        filename = f"debug_frames/{idx:04d}_{time_str}_CMD{cmd_txt}_CNT{contour_count}.jpg"
        cv2.imwrite(filename, img)
        # Save binary mask
        mask_filename = f"debug_frames/binary_mask/{idx:04d}_{time_str}_CMD{cmd_txt}_CNT{contour_count}.jpg"
        cv2.imwrite(mask_filename, binary_mask)

    # Cleanup
    print("Cleaning up...")
    send_command(CMD_STOP)  # Stop motors
    camera.stop()
    GPIO.cleanup()
    print("Done!")
    # Print final statistics
    total_time = time.time() - start_time
    avg_fps = frame_count / total_time if total_time > 0 else 0
    print(f"Session complete: {frame_count} frames in {total_time:.1f}s (avg {avg_fps:.1f} FPS)")
    