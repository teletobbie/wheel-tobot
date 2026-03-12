import os
from picamera2 import Picamera2
import time
from datetime import datetime
import cv2
import numpy as np
import serial

# Configuration
MAX_DEBUG_FRAMES = 50  # Limit number of saved frames to prevent memory issues
CAMERA_WIDTH = 640
CAMERA_HEIGHT = 360

# Serial communication setup
SERIAL_PORT = '/dev/ttyS0'  # Raspberry Pi hardware UART (TX=GPIO14, RX=GPIO15)
BAUD_RATE = 115200

# Initialize serial connection to Arduino
try:
    arduino_serial = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.01)
    time.sleep(2)  # Wait for Arduino to reset after serial connection
    print(f"Serial connection established on {SERIAL_PORT} at {BAUD_RATE} baud")
except Exception as e:
    print(f"Failed to open serial port: {e}")
    print("Make sure UART is enabled in raspi-config and Arduino is connected")
    exit(1)

def send_error_value(error):
    """Send proportional error value to Arduino via UART
    
    Args:
        error: Line position error in pixels (-320 to +320)
               Negative = line on left, Positive = line on right
    
    Protocol: [SYNC][ERROR_HIGH][ERROR_LOW][CHECKSUM]
    """
    # Clamp error to valid range
    error = max(-320, min(320, int(error)))
    
    # Convert to signed 16-bit bytes
    error_signed = error if error >= 0 else (0x10000 + error)
    error_high = (error_signed >> 8) & 0xFF
    error_low = error_signed & 0xFF
    
    # Simple checksum
    checksum = (0xFF + error_high + error_low) & 0xFF
    
    # Send packet
    packet = bytes([0xFF, error_high, error_low, checksum])
    try:
        arduino_serial.write(packet)
    except Exception as e:
        print(f"Serial write error: {e}")

# Setup camera with picamera2
camera = Picamera2()
camera_config = camera.create_preview_configuration(
    main={"size": (CAMERA_WIDTH, CAMERA_HEIGHT), "format": "RGB888"}
)
camera.configure(camera_config)
camera.start()
time.sleep(1)  # Camera warm-up

center_x = CAMERA_WIDTH // 2  # Center of image

print("Starting line following with UART serial control...")
send_error_value(0)  # Start with no error (motors will be controlled by Arduino)

frame_count = 0
start_time = time.time()
last_status_time = start_time
# add frame and time counters for debugging
images = []
last_error = 0  # Track last error for line search

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
        
        error = 0  # Default: No error (centered)
        command_text = "CENTERED"
        line_center = None
        
        if len(contours) > 0:
            # Find the largest contour (most likely the line)
            largest_contour = max(contours, key=cv2.contourArea)
            x, y, w, h = cv2.boundingRect(largest_contour)
            line_center = x + (w / 2)
            
            # Calculate proportional error (pixels from center)
            error = int(line_center - center_x)
            last_error = error  # Remember for line search
            
            # Generate status text based on error magnitude
            if abs(error) < 20:
                command_text = f"CENTERED (err:{error:+d})"
            elif error < 0:
                command_text = f"TURN LEFT (err:{error:+d})"
            else:
                command_text = f"TURN RIGHT (err:{error:+d})"
        else:
            # No line detected - continue in last known direction
            if abs(last_error) < 20:
                error = 0  # Was centered, stop
                command_text = "STOP (No line)"
            else:
                # Search in the direction line was last seen
                error = last_error  # Maintain last correction
                command_text = f"SEARCH (err:{error:+d})"
        
        # Send proportional error to Arduino
        send_error_value(error)
        
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
    send_error_value(0)  # Send zero error (stop motors)
    time.sleep(0.1)
    arduino_serial.close()
    camera.stop()
    print("Done!")
    # Print final statistics
    total_time = time.time() - start_time
    avg_fps = frame_count / total_time if total_time > 0 else 0
    print(f"Session complete: {frame_count} frames in {total_time:.1f}s (avg {avg_fps:.1f} FPS)")
    