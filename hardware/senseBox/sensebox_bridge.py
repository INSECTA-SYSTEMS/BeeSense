#!/usr/bin/env python3
"""
BeeSense OpenSenseMap Bridge
=============================
Dieses Script liest Sensordaten vom Arduino über die serielle Schnittstelle
und sendet sie zur OpenSenseMap.

Voraussetzungen:
    pip install pyserial requests

Verwendung:
    python3 sensebox_bridge.py [PORT]
    
    Beispiele:
    - macOS: python3 sensebox_bridge.py /dev/cu.usbserial-*
    - Linux: python3 sensebox_bridge.py /dev/ttyUSB0
    - Windows: python3 sensebox_bridge.py COM3
    
    Wenn kein PORT angegeben wird, werden verfügbare Ports aufgelistet.
"""

import serial
import serial.tools.list_ports
import requests
import json
import time
import sys
from datetime import datetime

# OpenSenseMap Konfiguration
OSEM_API_URL = "https://ingress.opensensemap.org"
OSEM_AUTH_TOKEN = "9b7caacbf7b53af8a05d93423e5df5b630d0e67fb7abce2499420004a8bc9aa5"

# BeeSense Dashboard Konfiguration
DASHBOARD_API_URL = "http://3.75.94.127:8080"

# Mapping: Sensor-Type → Dashboard-Endpoint + JSON-Feld
# Die Keys entsprechen dem "type"-Feld aus den DATA|-Zeilen des ESP32
DASHBOARD_TYPE_MAP = {
    "Beehive_Temperature": ("/api/sensors/water", "waterTemperature"),
    "Temperature":         ("/api/sensors",       "temperature"),
    "Humidity":            ("/api/sensors",       "humidity"),
    "UV":                  ("/api/sensors/light", "uv"),
    "Lux":                 ("/api/sensors/light", "lux"),
}

# Serial Konfiguration
BAUD_RATE = 115200
TIMEOUT = 2

# Farben für Terminal-Ausgabe
class Colors:
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def print_colored(text, color):
    """Gibt farbigen Text aus"""
    print(f"{color}{text}{Colors.RESET}")

def print_info(text):
    print_colored(f"ℹ {text}", Colors.BLUE)

def print_success(text):
    print_colored(f"✓ {text}", Colors.GREEN)

def print_warning(text):
    print_colored(f"⚠ {text}", Colors.YELLOW)

def print_error(text):
    print_colored(f"✗ {text}", Colors.RED)

def list_serial_ports():
    """Listet alle verfügbaren seriellen Ports auf"""
    ports = serial.tools.list_ports.comports()
    
    if not ports:
        print_warning("Keine seriellen Ports gefunden!")
        return []
    
    print_info("Verfügbare serielle Ports:")
    for i, port in enumerate(ports, 1):
        print(f"  {i}. {port.device} - {port.description}")
    
    return ports

def send_to_opensensemap(box_id, sensor_id, value):
    """Sendet einen Messwert zur OpenSenseMap"""
    url = f"{OSEM_API_URL}/boxes/{box_id}/{sensor_id}"
    
    headers = {
        "Content-Type": "application/json",
        "Authorization": OSEM_AUTH_TOKEN
    }
    
    data = {
        "value": value
    }
    
    try:
        response = requests.post(url, json=data, headers=headers, timeout=10)
        
        if response.status_code == 201:
            print_success(f"Daten gesendet: Sensor {sensor_id[-8:]}... = {value}")
            return True
        else:
            print_error(f"Fehler beim Senden (HTTP {response.status_code}): {response.text}")
            return False
            
    except requests.exceptions.RequestException as e:
        print_error(f"Netzwerkfehler beim Senden: {e}")
        return False

def send_to_dashboard(sensor_type, value):
    """Sendet einen Messwert an das BeeSense Dashboard"""
    mapping = DASHBOARD_TYPE_MAP.get(sensor_type)
    if not mapping:
        print_warning(f"Kein Dashboard-Mapping für Typ '{sensor_type}'")
        return False

    endpoint, field = mapping
    url = f"{DASHBOARD_API_URL}{endpoint}"

    data = {
        field: value,
        "timestamp": int(time.time() * 1000)  # ms seit Epoch
    }

    try:
        response = requests.post(url, json=data, timeout=10)

        if response.status_code == 200:
            print_success(f"Dashboard: {field}={value} → {endpoint}")
            return True
        else:
            print_error(f"Dashboard-Fehler (HTTP {response.status_code}): {response.text}")
            return False

    except requests.exceptions.RequestException as e:
        print_error(f"Dashboard-Netzwerkfehler: {e}")
        return False


def parse_data_line(line):
    """Parst eine DATA-Zeile vom Arduino"""
    try:
        # Format: DATA|{"boxId":"xxx","sensorId":"xxx","value":12.34,"type":"Temperature"}
        if not line.startswith("DATA|"):
            return None
        
        json_str = line[5:]  # Entferne "DATA|" Prefix
        data = json.loads(json_str)
        
        return {
            'boxId': data.get('boxId'),
            'sensorId': data.get('sensorId'),
            'value': data.get('value'),
            'type': data.get('type')
        }
    except (json.JSONDecodeError, KeyError) as e:
        print_warning(f"Fehler beim Parsen der Daten: {e}")
        return None

def main():
    """Hauptfunktion"""
    print_colored("\n" + "="*60, Colors.BOLD)
    print_colored("   BeeSense → OpenSenseMap Bridge", Colors.BOLD)
    print_colored("="*60 + "\n", Colors.BOLD)
    
    # Port auswählen
    if len(sys.argv) > 1:
        port_name = sys.argv[1]
    else:
        ports = list_serial_ports()
        if not ports:
            print_error("Keine Ports verfügbar. Beende.")
            sys.exit(1)
        
        print("\nBitte Port-Nummer eingeben (oder 'q' zum Beenden): ", end='')
        choice = input()
        
        if choice.lower() == 'q':
            sys.exit(0)
        
        try:
            port_index = int(choice) - 1
            port_name = ports[port_index].device
        except (ValueError, IndexError):
            print_error("Ungültige Auswahl!")
            sys.exit(1)
    
    print_info(f"Verwende Port: {port_name}")
    print_info(f"Baud Rate: {BAUD_RATE}")

    def open_serial(port):
        """Öffnet die serielle Verbindung zum ESP32"""
        ser = serial.Serial(
            port=port,
            baudrate=BAUD_RATE,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=TIMEOUT,
            xonxoff=False,
            rtscts=False,
            dsrdtr=False
        )
        # Buffer leeren
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        return ser

    # Äußere Reconnect-Schleife
    max_reconnects = 50
    reconnect_count = 0
    ser = None

    try:
        while reconnect_count < max_reconnects:
            # Verbindung herstellen (oder wiederherstellen)
            try:
                if ser is not None and ser.is_open:
                    ser.close()
                ser = open_serial(port_name)
                if reconnect_count == 0:
                    print_success(f"Verbunden mit {port_name}")
                    print_info("Warte auf Daten vom Arduino...\n")
                else:
                    print_success(f"Reconnect #{reconnect_count} erfolgreich")
                # Kurz warten, damit Arduino bereit ist
                time.sleep(3)
            except serial.SerialException:
                reconnect_count += 1
                print_warning(f"Port nicht verfügbar, warte... (Versuch {reconnect_count}/{max_reconnects})")
                time.sleep(2)
                continue

            # Hauptschleife - lese Daten vom ESP32
            consecutive_errors = 0
            max_consecutive_errors = 10

            while True:
                try:
                    # Lese Zeile von Serial - timeout verhindert Blockierung
                    line = ser.readline().decode('utf-8', errors='ignore').strip()

                    # Reset error counter bei erfolgreicher Lesung
                    consecutive_errors = 0

                    if not line:
                        continue

                    # Info-Nachrichten vom Arduino anzeigen
                    if line.startswith("INFO:"):
                        print_colored(f"[Arduino] {line[6:]}", Colors.BLUE)
                        continue

                    # Daten-Zeilen verarbeiten und zur OpenSenseMap senden
                    if line.startswith("DATA|"):
                        timestamp = datetime.now().strftime("%H:%M:%S")
                        print(f"\n[{timestamp}] Empfange Daten...")

                        parsed = parse_data_line(line)
                        if parsed:
                            print(f"  Sensor: {parsed['type']}")
                            print(f"  Wert: {parsed['value']}")

                            # Zur OpenSenseMap senden
                            send_to_opensensemap(
                                parsed['boxId'],
                                parsed['sensorId'],
                                parsed['value']
                            )

                            # Zum BeeSense Dashboard senden
                            send_to_dashboard(
                                parsed['type'],
                                parsed['value']
                            )
                        else:
                            print_warning("Konnte Daten nicht parsen")

                except KeyboardInterrupt:
                    print_warning("\n\nBeende durch Benutzer...")
                    return
                except (OSError, IOError, serial.SerialException) as e:
                    consecutive_errors += 1

                    # Bei macOS ESP32 "Device not configured" - kurzfristig ignorieren
                    if "[Errno 6]" in str(e) or "Device not configured" in str(e):
                        if consecutive_errors <= 3:
                            print_error(f"macOS ESP32 Fehler (wird ignoriert): {e}")
                        if consecutive_errors > max_consecutive_errors:
                            # Port wahrscheinlich weg → Reconnect
                            print_warning("Verbindung verloren, versuche Reconnect...")
                            reconnect_count += 1
                            break  # Zurück in die äußere Reconnect-Schleife
                        time.sleep(0.05)
                        continue

                    # Andere Serial-Fehler → Reconnect
                    print_error(f"Serial-Fehler: {e}")
                    print_warning("Versuche Reconnect...")
                    reconnect_count += 1
                    time.sleep(2)
                    break  # Zurück in die äußere Reconnect-Schleife
                except Exception as e:
                    print_error(f"Unerwarteter Fehler: {e}")
                    consecutive_errors += 1
                    if consecutive_errors > max_consecutive_errors:
                        import traceback
                        traceback.print_exc()
                        return
                    time.sleep(0.5)

        print_error(f"Maximale Reconnect-Versuche ({max_reconnects}) erreicht. Beende.")

    except KeyboardInterrupt:
        print_warning("\n\nBeende durch Benutzer...")
    finally:
        if ser is not None and ser.is_open:
            ser.close()
            print_info("Serielle Verbindung geschlossen")

    print_success("\nProgramm beendet.")

if __name__ == "__main__":
    main()
