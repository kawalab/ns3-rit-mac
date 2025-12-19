"""
Frame type and MAC address processing utilities.
"""


def is_broadcast(mac: str) -> bool:
    """0xffff などのブロードキャストMAC判定"""
    if isinstance(mac, str):
        return mac.lower() in ['0xffff', 'ffff', 'broadcast']
    return False


def frame_type_color(frame_type: str) -> str:
    """可視化用のフレーム種別ごとの色定義"""
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
    """フレーム種別の正規化"""
    return frame_type.upper()


def parse_mac_address(mac: str) -> str:
    """MACアドレスの正規化"""
    if isinstance(mac, str):
        # Remove 0x prefix if present
        if mac.startswith('0x'):
            mac = mac[2:]
        return mac.upper()
    return str(mac)
