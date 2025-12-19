"""
Log file column definitions and schema management.
"""

# MAC TX Log columns
MAC_TXLOG_COLUMNS = ["time", "event", "frameType", "dstMac"]

# MAC RX Log columns
MAC_RXLOG_COLUMNS = ["time", "event", "frameType", "srcMac"]

# MAC Summary columns
MAC_SUMMARY_COLUMNS = [
    "nodeId", "txOk", "txDrop", "rxOk", "rxDrop",
    "beaconTx", "dataTx", "ackTx", "dataRelay", "energy_mJ",
    "rxTimeouts", "txTimeouts", "avgDataWaitTimeMs", "avgBeaconWaitTimeTxMs"
]

# Frame type mappings
FRAME_TYPES = {
    "BEACON": "beacon",
    "DATA": "data",
    "ACK": "ack"
}

# Event type mappings
EVENT_TYPES = {
    "TX_OK": "tx_ok",
    "TX_DROP": "tx_drop",
    "RX_OK": "rx_ok",
    "RX_DROP": "rx_drop"
}
