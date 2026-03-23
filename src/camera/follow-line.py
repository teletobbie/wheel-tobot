from picamera2 import Picamera2
import time
import cv2
import numpy as np
import serial
import argparse
import signal
import sys
from logger import ArduinoLogger

# Configuration
CAMERA_WIDTH = 640
CAMERA_HEIGHT = 360
SEARCH_TIMEOUT_FRAMES = 40  # Stop after losing line for 40 frames (~1.5 seconds)
SEARCH_ERROR_THRESHOLD = 20  # Only search if last error was significant
REVERSE_FRAMES = 15  # Try to reverse/turn back for first 15 frames

# Serial communication setup
SERIAL_PORT = '/dev/ttyS0'  # Raspberry Pi hardware UART (TX=GPIO14, RX=GPIO15)
BAUD_RATE = 57600

# Direction codes for Arduino protocol
DIR_FORWARD = 0       # Normal line following
DIR_WAITING = 1       # Stopped, no line yet
DIR_REVERSE_LEFT = 2  # Just lost line, turning back left
DIR_REVERSE_RIGHT = 3 # Just lost line, turning back right
DIR_SEARCH_LEFT = 4   # Gentle sweep searching left
DIR_SEARCH_RIGHT = 5  # Gentle sweep searching right
# Parse command line arguments
parser = argparse.ArgumentParser(description='Line following robot with optional web streaming')
parser.add_argument('--stream', action='store_true', help='Enable web streaming on port 5000')
args = parser.parse_args()

# Import streaming module if enabled
if args.stream:
    import streaming_server    

# Initialize serial connection to Arduino
try:
    arduino_serial = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.01)
    time.sleep(2)  # Wait for Arduino to reset after serial connection
    # Flush any garbage data from initial connection
    arduino_serial.reset_input_buffer()
    arduino_serial.reset_output_buffer()
    arduino_logger = ArduinoLogger(log_to_file=False)
    print(f"Serial connection established on {SERIAL_PORT} at {BAUD_RATE} baud")
except Exception as e:
    print(f"Failed to open serial port: {e}")
    print("Make sure UART is enabled in raspi-config and Arduino is connected")
    sys.exit(1)

def send_error_value(error, direction=DIR_FORWARD):
    """Send proportional error value and direction to Arduino via UART
    
    Args:
        error: Line position error in pixels (-320 to +320)
               Negative = line on left, Positive = line on right
        direction: Command byte (default: DIR_FORWARD)
                   DIR_FORWARD (0) = Normal line following
                   DIR_WAITING (1) = Stopped, no line yet
                   DIR_REVERSE_LEFT (2) = Just lost line, turning back left
                   DIR_REVERSE_RIGHT (3) = Just lost line, turning back right
                   DIR_SEARCH_LEFT (4) = Gentle sweep searching left
                   DIR_SEARCH_RIGHT (5) = Gentle sweep searching right
    
    Protocol: [SYNC][ERROR_HIGH][ERROR_LOW][DIRECTION]
    """
    
    # Convert to signed 16-bit bytes by deviding into two 8-bit values
    error_signed = error if error >= 0 else (0x10000 + error)
    error_high = (error_signed >> 8) & 0xFF
    error_low = error_signed & 0xFF
    
    # Send packet with direction byte
    packet = bytes([0xFF, error_high, error_low, direction])
    try:
        arduino_serial.write(packet)
    except Exception as e:
        print(f"Serial write error: {e}")

def read_arduino_logs():
    """Read and log any incoming data from Arduino"""
    while arduino_serial.in_waiting > 0:
        try:
            line = arduino_serial.readline().decode('utf-8', errors='ignore').strip()
            if line and line.startswith('LOG,'):
                data = arduino_logger.parse_log_line(line)
                arduino_logger.log_data(data)
        except Exception as e:
            pass  # Ignore read errors

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

# Wait longer for Arduino to fully boot and clear bootloader garbage
time.sleep(1)
arduino_serial.reset_input_buffer()
time.sleep(0.5)

send_error_value(0, direction=DIR_WAITING)  # Start in WAITING state

frame_count = 0
start_time = time.time()
last_status_time = start_time
last_error = 0  # Track last error for line search
frames_without_line = 0  # Count frames where line is lost
line_found = False  # Has the line ever been found?
running = True  # Control flag for main loop

# Signal handler for graceful shutdown
def signal_handler(sig, frame):
    global running
    print("\nShutdown signal received...")
    running = False

signal.signal(signal.SIGINT, signal_handler)

if args.stream:
    streaming_server.start_streaming()

while running:
        # Capture frame
        image = camera.capture_array()
        roi = image[200:250, 0:639]
        Blackline = cv2.inRange(roi, (0, 0, 0), (50, 50, 50))
        kernel = np.ones((3, 3), np.uint8)
        Blackline = cv2.erode(Blackline, kernel, iterations=3)
        Blackline = cv2.dilate(Blackline, kernel, iterations=5)	
        contours, hierarchy = cv2.findContours(Blackline.copy(), cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)	
        
        error = 0  # Default: No error (centered)
        command_text = "CENTERED"
        direction = DIR_FORWARD  # Default: FORWARD
        line_center = None
        
        if len(contours) > 0:
            # Line detected!
            frames_without_line = 0
            line_found = True
            
            # Find the largest contour (most likely the line)
            largest_contour = max(contours, key=cv2.contourArea)
            x, y, w, h = cv2.boundingRect(largest_contour)
            line_center = x + (w / 2)
            
            # Calculate proportional error (pixels from center)
            error = int(line_center - center_x)
            
            # Clamp error to reasonable range to prevent extreme corrections
            if error > 200:
                error = 200
            elif error < -200:
                error = -200
                
            last_error = error  # Remember for line search
            
            # Generate status text based on error magnitude
            direction = DIR_FORWARD  # FORWARD (normal line following)
            if abs(error) < 30:
                command_text = "CENTERED"
            elif error < 0:
                command_text = "TURN_LEFT"
            else:
                command_text = "TURN_RIGHT"
        else:
            # No line detected
            frames_without_line += 1
            
            if not line_found:
                # Never found line - stop and wait
                error = 0
                command_text = "WAITING"
                direction = DIR_WAITING  # WAITING
            elif frames_without_line <= REVERSE_FRAMES:
                # Just lost line - turn back in opposite direction
                # Use stronger correction for larger errors
                if abs(last_error) > 100:
                    reverse_strength = 0.4  # 40% for large errors
                elif abs(last_error) > 50:
                    reverse_strength = 0.3  # 30% for medium errors
                else:
                    reverse_strength = 0.2  # 20% for small errors
                
                error = -int(last_error * reverse_strength)
                if error > 0:
                    command_text = "REVERSE_RIGHT"
                    direction = DIR_REVERSE_RIGHT  # REVERSE_RIGHT
                else:
                    command_text = "REVERSE_LEFT"
                    direction = DIR_REVERSE_LEFT  # REVERSE_LEFT
            else:
                # Still no line after reversing - gentle sweep search
                error = int(last_error * 0.15)
                if last_error < 0:
                    command_text = "SEARCH_LEFT"
                    direction = DIR_SEARCH_LEFT  # SEARCH_LEFT
                else:
                    command_text = "SEARCH_RIGHT"
                    direction = DIR_SEARCH_RIGHT  # SEARCH_RIGHT
        
        # Send proportional error and direction to Arduino
        send_error_value(error, direction)
        
        # Update frame counter
        frame_count += 1

        # Draw visualization for streaming and debug frames
        debug_img = image.copy()
        cv2.rectangle(debug_img, (0, 200), (639, 250), (0, 165, 255), 2)
        if line_center is not None:
            cv2.line(debug_img, (int(line_center), 200), (int(line_center), 250), (255, 0, 0), 3)
            error_val = int(line_center - center_x)
            cv2.putText(debug_img, f"Error: {error_val}px", (10, 70), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)
        cv2.line(debug_img, (center_x, 200), (center_x, 250), (0, 255, 0), 2)
        cv2.putText(debug_img, f"CMD: {command_text}", (10, 30), 
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        
        # Update stream frame if streaming is enabled
        if args.stream:
            streaming_server.update_frame(debug_img)
        
        # Print status every 2 seconds
        current_time = time.time()
        if current_time - last_status_time >= 2.0:
            fps = frame_count / (current_time - start_time)
            status_msg = f"[{current_time - start_time:.1f}s] FPS: {fps:.1f} | CMD: {command_text} | ERR: {error:+4d} | CNT: {len(contours)}"
            if frames_without_line > 0:
                status_msg += f" | LOST: {frames_without_line}"
            print(status_msg)
            last_status_time = current_time
        
        # Read and log any incoming data from Arduino
        read_arduino_logs()

        # Minimal delay for headless mode (maximize FPS)
        time.sleep(0.001)

# Cleanup
print("\nCleaning up...")
send_error_value(0, direction=DIR_WAITING)  # Send zero error with WAITING state (stop motors)
time.sleep(0.1)
arduino_serial.close()
camera.stop()
arduino_logger.close()
print("Done!")

# Print final statistics
total_time = time.time() - start_time
avg_fps = frame_count / total_time if total_time > 0 else 0
print(f"Session complete: {frame_count} frames in {total_time:.1f}s (avg {avg_fps:.1f} FPS)")