"""
Frame type and MAC address processing utilities.
"""


def is_broadcast(mac: str) -> bool:
    """Check whether a MAC address is a broadcast address (e.g., '0xffff')."""
    if isinstance(mac, str):
        return mac.lower() in ['0xffff', 'ffff', 'broadcast']
    return False


def frame_type_color(frame_type: str) -> str:
    """Color mapping for frame types used in visualization."""
    color_map = {
        'beacon': '#1f77b4',  # blue
        'data': '#ff7f0e',    # orange
        'ack': '#2ca02c',     # green
        'BEACON': '#1f77b4',
        'DATA': '#ff7f0e',
        'ACK': '#2ca02c'
    }
    return color_map.get(frame_type, '#7f7f7f')  # default gray


def normalize_frame_type(frame_type: str) -> str:
    """Normalize a frame type string to a standard representation."""
    return frame_type.upper()


def parse_mac_address(mac: str) -> str:
    """Normalize a MAC address string (remove 0x prefix and uppercase)."""
    if isinstance(mac, str):
        # Remove 0x prefix if present
        if mac.startswith('0x'):
            mac = mac[2:]
        return mac.upper()
    return str(mac)
