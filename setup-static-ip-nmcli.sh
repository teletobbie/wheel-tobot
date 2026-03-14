#!/bin/bash
#
# Static IP Configuration Script for Raspberry Pi (NetworkManager)
# Sets wlan0 to 192.168.1.178 with gateway 192.168.1.1
#
# Usage: 
#   chmod +x setup-static-ip-nmcli.sh
#   ./setup-static-ip-nmcli.sh
#

echo "====================================="
echo "Raspberry Pi Static IP Setup"
echo "NetworkManager Version"
echo "====================================="
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then 
   echo "Error: Do not run as root. Run as regular user (script will use sudo)."
   exit 1
fi

# Get current WiFi connection name
echo "[1/4] Detecting WiFi connection..."
WIFI_CONN=$(nmcli -t -f NAME,TYPE connection show --active | grep 802-11-wireless | cut -d: -f1)

if [ -z "$WIFI_CONN" ]; then
    echo "✗ No active WiFi connection found!"
    echo "Available connections:"
    nmcli connection show
    exit 1
fi

echo "✓ Found active WiFi: $WIFI_CONN"

# Show current IP
echo ""
echo "[2/4] Current IP configuration:"
ip -4 addr show wlan0 | grep inet

# Configure static IP
echo ""
echo "[3/4] Configuring static IP..."
sudo nmcli connection modify "$WIFI_CONN" ipv4.addresses 192.168.1.178/24
sudo nmcli connection modify "$WIFI_CONN" ipv4.gateway 192.168.1.1
sudo nmcli connection modify "$WIFI_CONN" ipv4.dns "192.168.1.1 8.8.8.8"
sudo nmcli connection modify "$WIFI_CONN" ipv4.method manual

if [ $? -eq 0 ]; then
    echo "✓ Static IP configuration applied"
else
    echo "✗ Configuration failed!"
    exit 1
fi

# Restart connection
echo ""
echo "[4/4] Restarting network connection..."
sudo nmcli connection down "$WIFI_CONN" && sudo nmcli connection up "$WIFI_CONN"

if [ $? -eq 0 ]; then
    echo "✓ Connection restarted successfully"
else
    echo "✗ Connection restart failed!"
    exit 1
fi

echo ""
echo "====================================="
echo "✓ Static IP Setup Complete!"
echo "====================================="
echo ""
echo "Configuration:"
echo "  IP Address: 192.168.1.178"
echo "  Gateway:    192.168.1.1"
echo "  DNS:        192.168.1.1, 8.8.8.8"
echo ""
echo "New IP configuration:"
ip -4 addr show wlan0 | grep inet
echo ""
echo "To verify connection: ping 192.168.1.1"
echo ""
