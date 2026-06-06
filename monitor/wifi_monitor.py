"""
ESP32 Wi-Fi Scanner - Serial Port Monitor
Independent script for monitoring ESP32 serial output.
Does not require idf.py or the ESP-IDF toolchain.

Requirements:   pyserial, rich
Install:        pip install -r requirements.txt
Usage:          python wifi_monitor.py [PORT] [BAUDRATE]
Examples:       python wifi_monitor.py COM3 115200
                python wifi_monitor.py /dev/ttyUSB0 115200
"""

import sys
import json
import time
import argparse
import threading
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[ERROR] No pyserial library. Install: pip install pyserial")
    sys.exit(1)

try:
    from rich.live import Live
    from rich.table import Table
    from rich.text import Text
    from rich.console import Console
    from rich import box
except ImportError:
    print("[ERROR] No rich library. Install: pip install rich")
    sys.exit(1)

console = Console()

# shared state
lock = threading.Lock()
access_points: dict[str, dict] = {}
status_text = Text("Connecting...", style="yellow")

# helpers
def rssi_style(rssi: int) -> str:
    """
    Returns a Rich style string based on signal strength.

    Args:
        rssi: Signal strength in dBm (typically -30 to -90)
    
    Returns:
        'bold green' for strong signal (>= -65 dBm)
        'yellow' for medium (>= -75 dBm)
        'bold red' for weak (< -75 dBm)
    """
    if rssi >= -65:
        return "bold green"
    if rssi >= -75:
        return "yellow"
    return "bold red"

def rssi_bar(rssi: int) -> str:
    """
    Returns a 4-character Unicode block bar representing signal strength.

    Args:
        rssi: Signal strength in dBm
    
    Returns:
        A string of filled/empty block characters (e.g. '██░░')
    """
    if rssi >= -50:
        return "████"
    if rssi >= -65:
        return "███░"
    if rssi >= -75:
        return "██░░"
    if rssi >= -85:
        return "█░░░"
    return "░░░░"
    
def parse_line(line: str) -> dict | None:
    """
    Parses a single JSON line received from the ESP32 over serial.

    Expected format:
        {"ssid":"MyNetwork","bssid":"aa:bb:cc:dd:ee:ff","rssi":-70,"chan":6,"auth":"WPA2"}
    
    Adds a 'seen' key with the current timestamp (HH:MM:SS).
    Replaces empty SSID with '<hidden>' for display purposes.

    Args:
        line: Raw string line read from the serial port

    Returns:
        A dict with keys: ssid, bssid, rssi, chan, auth, seen
        Returns None if the line is not valid JSON or is missing required fields
    """
    try:
        ap = json.loads(line)
        if not all(k in ap for k in ("ssid", "bssid", "rssi", "chan", "auth")):
            return None
        ap["ssid"] = ap["ssid"] or "<hidden>"
        ap["seen"] = datetime.now().strftime("%H:%M:%S")
        return ap
    except json.JSONDecodeError:
        return None
    
def list_ports() -> list[str]:
    """
    Returns a list of all available serial port device names on the system.

    Returns:
        A list of port strings, e.g. ['COM3', 'COM5'] on Windows
        or ['/dev/ttyUSB0'] on Linux
    """
    return [p.device for p in serial.tools.list_ports.comports()]

# rich table builder
def build_table() -> Table:
    """
    Builds and returns a Rich Table populated with the current list of access points.

    Reads access_points and status_text from shared state under the lock.
    Rows are sorted by RSSI in descending order (strongest signal first).
    Each row includes SSID, BSSID, RSSI value, signal bar, channel, auth type,
    and the timestamp of when the network was last seen.

    Returns:
        A fully constructed rich.table.Table ready to be rendered by Live
    """
    with lock:
        aps = sorted(access_points.values(), key=lambda x: x["rssi"], reverse=True)
        status = status_text
    
    table = Table(
        box=box.ROUNDED,
        title="[bold cyan]ESP32 Wi-Fi Scanner Monitor[/] [dim](Ctrl+C to exit)[/]",
        caption=status,
        expand=True,
    )
    table.add_column("SSID", style="white", max_width=32)
    table.add_column("BSSID", style="dim", max_width=18)
    table.add_column("RSSI", justify="right", width=7)
    table.add_column("Signal", justify="center", width=6)
    table.add_column("CHAN", justify="center", width=5)
    table.add_column("AUTH", style="cyan", width=14)
    table.add_column("Time", justify="right", width=9)

    for ap in aps:
        style = rssi_style(ap["rssi"])
        table.add_row(
            ap["ssid"],
            ap["bssid"],
            Text(str(ap["rssi"]), style=style),
            Text(rssi_bar(ap["rssi"]), style=style),
            str(ap["chan"]),
            ap["auth"],
            ap["seen"],
        )
    
    return table

# serial reader with auto-reconnect
def serial_reader(port: str, baud: int, stop_event: threading.Event):
    """
    Background thread that reads JSON lines from the ESP32 serial port.

    Continuously attempts to open the given port and read lines.
    Each valid JSON line is parsed and stored in the shared access_points dict.
    On SerialException (disconnect, port unavailable), clears the network list,
    updates the status to an error message, and retries the connection after 3 seconds.
    Stops cleanly when stop_event is set.
    """
    global status_text

    while not stop_event.is_set():
        try:
            with serial.Serial(port, baud, timeout=1) as ser:
                with lock:
                    status_text = Text(f"Connected to {port} @ {baud} baud", style="green")

                while not stop_event.is_set():
                    raw = ser.readline()
                    if not raw:
                        continue
                    line = raw.decode("utf-8", errors="replace").strip()

                    ap = parse_line(line)
                    if ap:
                        with lock:
                            access_points[ap["bssid"]] = ap
                            status_text = Text(
                                f"Last scan: {ap['seen']} | Networks: {len(access_points)}",
                                style="green"
                            )
        except serial.SerialException as e:
            with lock:
                access_points.clear()
                status_text = Text(f"[ERROR] Lost connection: {e} - retrying in 3s...", style="bold red")
            time.sleep(3)

# port selection
def pick_port(arg_port: str | None) -> str:
    """
    Resolves which serial port to use.

    If arg_port is provided, returns it directly.
    If exactly one port is detected on the system, selects it automatically.
    If multiple ports are found, prints a numbered list and prompts the user to choose.
    Exits the program if no ports are found or the user makes an invalid selection.

    Args:
        arg_port: Port name passed via CLI argument, or None if not provided

    Returns:
        The selected serial port name as a string
    """
    if arg_port:
        return arg_port
    
    ports = list_ports()
    if not ports:
        console.print("[red][ERROR][/] No port found. Please connect your ESP32 and try again.")
        sys.exit(1)
    if len(ports) == 1:
        console.print(f"[yellow][AUTO][/] Found port: [bold]{ports[0]}[/]")
        return ports[0]
    
    for i, p in enumerate(ports):
        console.print(f" [{i}] {p}")
    choice = input("Choose port number: ").strip()
    try:
        return ports[int(choice)]
    except (ValueError, IndexError):
        console.print("[red]Incorrect choice[/]")
        sys.exit(1)

# entry point
def main():
    """
    Entry point for the ESP32 Wi-Fi Scanner Monitor.

    Parses CLI arguments (port, baud rate, refresh interval), selects the serial port,
    starts the serial_reader thread, and runs Rich Live display loop.
    Handles KeyboardInterrupt (Ctrl+C) for clean shutdown.
    """
    parser = argparse.ArgumentParser(description="ESP32 Wi-Fi Scanner - serial port monitor")
    parser.add_argument("port", nargs="?", help="Port COM/TTY (e.g. COM3, /dev/ttyUSB0)")
    parser.add_argument("baud", nargs="?", type=int, default=115200, help="Baudrate (default: 115200)")
    parser.add_argument("--refresh", "-r", type=float, default=1.0, help="Refresh interval in seconds")
    args = parser.parse_args()

    port = pick_port(args.port)
    stop_event = threading.Event()

    reader = threading.Thread(target=serial_reader, args=(port, args.baud, stop_event), daemon=True)
    reader.start()

    try:
        with Live(build_table(), console=console, refresh_per_second=int(1 / args.refresh)) as live:
            while not stop_event.is_set():
                live.update(build_table())
                time.sleep(args.refresh)
    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        reader.join(timeout=2)
        console.print("[dim]Goodbye![/]")

if __name__ == "__main__":
    main()