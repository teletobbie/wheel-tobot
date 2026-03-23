#!/usr/bin/env python3
"""
Arduino PID Logger
Reads and logs PID control data from Arduino via UART serial communication
"""

import serial
import time
import sys
import csv
from datetime import datetime

# Serial configuration
SERIAL_PORT = '/dev/ttyS0'  # Raspberry Pi hardware UART
BAUD_RATE = 57600

class ArduinoLogger:
    """Logger for Arduino PID control data"""
    
    def __init__(self, serial_port=SERIAL_PORT, baud_rate=BAUD_RATE, log_to_file=True):
        """Initialize logger
        
        Args:
            serial_port: Serial port path
            baud_rate: Serial communication baud rate
            log_to_file: Whether to save logs to CSV file
        """
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.log_to_file = log_to_file
        self.csv_writer = None
        self.csv_file = None
        
        if self.log_to_file:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"pid_log_{timestamp}.csv"
            self.csv_file = open(filename, 'w', newline='')
            self.csv_writer = csv.writer(self.csv_file)
            # Write header
            self.csv_writer.writerow([
                'timestamp', 'Kp', 'Ki', 'Kd', 'error', 
                'error_derivative', 'adjustment', 'left_speed', 'right_speed'
            ])
            print(f"Logging to file: {filename}")
    
    def parse_log_line(self, line):
        """Parse a LOG line from Arduino
        
        Args:
            line: String in format "LOG,Kp,Ki,Kd,error,error_derivative,adjustment,left_speed,right_speed"
        
        Returns:
            Dictionary with parsed values or None if invalid
        """
        try:
            if not line.startswith('LOG,'):
                return None
            
            parts = line.strip().split(',')
            if len(parts) != 9:
                return None
            
            data = {
                'timestamp': time.time(),
                'Kp': float(parts[1]),
                'Ki': float(parts[2]),
                'Kd': float(parts[3]),
                'error': int(parts[4]),
                'error_derivative': int(parts[5]),
                'adjustment': float(parts[6]),
                'left_speed': int(parts[7]),
                'right_speed': int(parts[8])
            }
            return data
        except (ValueError, IndexError) as e:
            print(f"Parse error: {e} - Line: {line}")
            return None
    
    def log_data(self, data):
        """Log parsed data to console and file
        
        Args:
            data: Dictionary with PID data
        """
        if data is None:
            return
        
        # Print to console
        print(f"[{data['timestamp']:.2f}] Kp={data['Kp']:.2f} Ki={data['Ki']:.2f} Kd={data['Kd']:.2f} | "
              f"err={data['error']:+4d} d_err={data['error_derivative']:+4d} | "
              f"adj={data['adjustment']:+6.2f} | L={data['left_speed']:3d} R={data['right_speed']:3d}")
        
        # Write to CSV file
        if self.log_to_file and self.csv_writer:
            self.csv_writer.writerow([
                data['timestamp'],
                data['Kp'],
                data['Ki'],
                data['Kd'],
                data['error'],
                data['error_derivative'],
                data['adjustment'],
                data['left_speed'],
                data['right_speed']
            ])
            self.csv_file.flush()  # Ensure data is written
    
    def close(self):
        """Close log file"""
        if self.csv_file:
            self.csv_file.close()
            print("Log file closed")


def main():
    """Standalone mode: Monitor and log Arduino data"""
    print(f"Arduino PID Logger")
    print(f"Connecting to {SERIAL_PORT} at {BAUD_RATE} baud...")
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        time.sleep(2)  # Wait for Arduino reset
        ser.reset_input_buffer()
        print("Connected!")
    except Exception as e:
        print(f"Failed to open serial port: {e}")
        sys.exit(1)
    
    logger = ArduinoLogger(log_to_file=True)
    
    print("\nMonitoring Arduino output (Ctrl+C to stop)...")
    print("-" * 80)
    
    try:
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Parse and log if it's a LOG line
                        if line.startswith('LOG,'):
                            data = logger.parse_log_line(line)
                            logger.log_data(data)
                        else:
                            # Print other Arduino output
                            print(f"Arduino: {line}")
                except UnicodeDecodeError:
                    pass  # Ignore decoding errors
            
            time.sleep(0.01)  # Small delay
            
    except KeyboardInterrupt:
        print("\n\nShutting down...")
    finally:
        logger.close()
        ser.close()
        print("Done!")


if __name__ == '__main__':
    main()
