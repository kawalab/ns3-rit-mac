"""
アプリ層・MAC層・PHY層の集計処理を提供する外部ユーティリティモジュール。

- 各層のログファイルを読み込み、主要な統計値（PDR、遅延、送受信数、状態比率など）を算出。
- ノートブックや他の分析スクリプトから呼び出し可能な関数群として設計。
- パラメータ・パス管理は SimulationConfig など外部クラス・モジュールに委譲。
"""

import pandas as pd
from common.io_utils import read_log


def summarize_app_node(node_id, app_txlog, app_rxlog, base_dir, parameter_dir, app_recv_node):
    """
    アプリ層ログの集計関数。
    指定ノードの送信(app-txlog)・受信(app-rxlog)ログから、PDR（受信率）・平均遅延・送受信数を算出。

    Args:
        node_id (str): 集計対象ノードID
        app_txlog (str): 送信ログファイル名
        app_rxlog (str): 受信ログファイル名
        base_dir (str): ログのベースディレクトリ
        parameter_dir (str): パラメータディレクトリ
        app_recv_node (str): 受信ノードID
    Returns:
        dict: 集計結果（PDR, 平均遅延, 送受信数など）
    """
    rx_df = read_log(app_recv_node, app_rxlog, ["time", "uid"], base_dir, parameter_dir)
    tx_df = read_log(node_id, app_txlog, ["time", "uid"], base_dir, parameter_dir)
    tx_uids = set(tx_df["uid"].values)
    rx_uids = set(rx_df["uid"].values)
    n_tx = len(tx_uids)
    n_rx = len(tx_uids & rx_uids)
    pdr = n_rx / n_tx if n_tx > 0 else None
    delays = []
    for uid in tx_uids:
        tx_time = tx_df[tx_df["uid"]==uid]["time"].values
        rx_time = rx_df[rx_df["uid"]==uid]["time"].values
        if len(tx_time)>0 and len(rx_time)>0:
            delays.append(rx_time[0] - tx_time[0])
    avg_delay = sum(delays)/len(delays) if delays else None
    tx_total = len(tx_df) if node_id != app_recv_node else 0
    rx_total = len(rx_df) if node_id == app_recv_node else 0
    return {
        "nodeId": node_id,
        "pdr": pdr,
        "avg_delay": avg_delay,
        "tx_total": tx_total,
        "rx_total": rx_total
    }


def summarize_mac_node(node_id, mac_log_files, base_dir, parameter_dir):
    """
    MAC層ログの集計関数。
    指定ノードのMAC層各種ログから、送受信数・ドロップ数・待機時間・状態比率などを算出。

    Args:
        node_id (str): 集計対象ノードID
        mac_log_files (dict): MAC層ログファイル名の辞書
        base_dir (str): ログのベースディレクトリ
        parameter_dir (str): パラメータディレクトリ
    Returns:
        dict: 集計結果（送受信数、ドロップ数、待機時間、状態比率など）
    """
    tx = read_log(node_id, mac_log_files["tx"], ["time", "type", "subtype", "src", "dst"], base_dir, parameter_dir)
    rx = read_log(node_id, mac_log_files["rx"], ["time", "status", "subtype", "src", "dst"], base_dir, parameter_dir)
    beacon_wait = read_log(node_id, mac_log_files["beacon_wait"], ["time", "event"], base_dir, parameter_dir)
    data_wait = read_log(node_id, mac_log_files["data_wait"], ["time", "event"], base_dir, parameter_dir)
    state_df = read_log(node_id, mac_log_files["state"], ["time", "state"], base_dir, parameter_dir)
    tx_types = ["Data", "Command", "Multipurpose"]
    txOk = tx[tx["subtype"].isin(tx_types) & (tx["type"]=="TxOk")].shape[0]
    txDrop = tx[tx["subtype"].isin(tx_types) & (tx["type"]=="TxDrop")].shape[0]
    txDataDrop = tx[(tx["subtype"]=="Data") & (tx["type"]=="TxDrop")].shape[0]
    txCommandDrop = tx[(tx["subtype"]=="Command") & (tx["type"]=="TxDrop")].shape[0]
    rxOk = rx[rx["status"]=="RxOk"].shape[0]
    rxDrop = rx[rx["status"]=="timeout"].shape[0]
    tx_data = tx[(tx["subtype"]=="Data") & (tx["type"]=="Tx")].shape[0]
    tx_command = tx[(tx["subtype"]=="Command")].shape[0]
    tx_multipurpose = tx[(tx["subtype"]=="Multipurpose")].shape[0]
    tx_ack = tx[(tx["subtype"]=="Ack")].shape[0]
    rx_data = rx[(rx["subtype"]=="Data") & (rx["status"]=="RxOk")].shape[0]
    rx_command = rx[(rx["subtype"]=="Command") & (rx["status"]=="RxOk")].shape[0]
    rx_multipurpose = rx[(rx["subtype"]=="Multipurpose") & (rx["status"]=="RxOk")].shape[0]
    rx_ack = rx[(rx["subtype"]=="Ack") & (rx["status"]=="RxOk")].shape[0]
    rxTimeouts = data_wait[data_wait["event"]=="timeout"].shape[0]
    txTimeouts = beacon_wait[beacon_wait["event"]=="timeout"].shape[0]
    def avg_wait(df):
        times = df[["time", "event"]].values
        waits = []
        waiting = None
        for t, ev in times:
            if ev == "start":
                waiting = t
            elif ev == "end" and waiting is not None:
                waits.append(t - waiting)
                waiting = None
            elif ev == "timeout":
                waiting = None
        return sum(waits)/len(waits)*1000 if waits else None
    avgDataWaitTimeMs = avg_wait(data_wait)
    avgBeaconWaitTimeTxMs = avg_wait(beacon_wait)
    state_times = {}
    if not state_df.empty:
        states = state_df["state"].values
        times = state_df["time"].values
        total_time = times[-1] - times[0] if len(times) > 1 else 0
        for i in range(len(states)-1):
            s = states[i]
            dt = times[i+1] - times[i]
            state_times[s] = state_times.get(s, 0) + dt
        state_ratios = {f"{k}_ratio": (v/total_time if total_time > 0 else None) for k, v in state_times.items()}
    else:
        state_ratios = {}
    return {
        "nodeId": node_id,
        "txOk": txOk,
        "txDrop": txDrop,
        "txData": tx_data,
        "txCommand": tx_command,
        "txMultipurpose": tx_multipurpose,
        "txAck": tx_ack,
        "txDataDrop": txDataDrop,
        "txCommandDrop": txCommandDrop,
        "rxOk": rxOk,
        "rxDrop": rxDrop,
        "rxData": rx_data,
        "rxCommand": rx_command,
        "rxMultipurpose": rx_multipurpose,
        "rxAck": rx_ack,
        "rxTimeouts": rxTimeouts,
        "txTimeouts": txTimeouts,
        "avgDataWaitTimeMs": avgDataWaitTimeMs,
        "avgBeaconWaitTimeTxMs": avgBeaconWaitTimeTxMs,
        **state_ratios
    }


def summarize_phy_node(node_id, base_dir, parameter_dir):
    """
    PHY層ログの集計関数。
    指定ノードのPHY層ログから、送受信数・ドロップ数・状態比率などを算出。

    Args:
        node_id (str): 集計対象ノードID
        base_dir (str): ログのベースディレクトリ
        parameter_dir (str): パラメータディレクトリ
    Returns:
        dict: 集計結果（送受信数、ドロップ数、状態比率など）
    """
    tx_df = read_log(node_id, "phy-txlog.csv", ["time", "event", "addr"], base_dir, parameter_dir)
    rx_df = read_log(node_id, "phy-rxlog.csv", ["time", "event", "addr", "val"], base_dir, parameter_dir)
    state_df = read_log(node_id, "phy-statelog.csv", ["time", "state"], base_dir, parameter_dir)
    txDropCount = tx_df[tx_df["event"]=="TxDrop"].shape[0] if not tx_df.empty else None
    rxDropCount = rx_df[rx_df["event"]=="RxDrop"].shape[0] if not rx_df.empty else None
    txCount = tx_df[tx_df["event"]=="TxEnd"].shape[0] if not tx_df.empty else None
    rxCount = rx_df[rx_df["event"]=="RxEnd"].shape[0] if not rx_df.empty else None
    idleCount = state_df[state_df["state"]=="TRX_OFF"].shape[0] if not state_df.empty else None
    state_times = {}
    if not state_df.empty:
        states = state_df["state"].values
        times = state_df["time"].values
        total_time = times[-1] - times[0] if len(times) > 1 else 0
        for i in range(len(states)-1):
            s = states[i]
            dt = times[i+1] - times[i]
            state_times[s] = state_times.get(s, 0) + dt
        state_ratios = {f"{k}_ratio": (v/total_time if total_time > 0 else None) for k, v in state_times.items()}
    else:
        state_ratios = {}
    return {
        "nodeId": node_id,
        "tx": txCount,
        "rx": rxCount,
        "txDrop": txDropCount,
        "rxDrop": rxDropCount,
        # "idle": idleCount,
        **state_ratios
    }


def summarize_scenario(app_summary_df, phy_summary_df):
    """
    シナリオレベルの統計サマリーを作成する関数。
    APPサマリーとPHYサマリーから、ノード全体での統計値を算出。

    Args:
        app_summary_df (pd.DataFrame): アプリケーション層サマリーのDataFrame
        phy_summary_df (pd.DataFrame): PHY層サマリーのDataFrame

    Returns:
        dict: シナリオレベルの統計値
            - PDR統計（送信ノードのみ対象）
            - 遅延統計（送信ノードのみ対象）
            - 起床時間統計（全ノード対象）
    """
    result = {}

    # PDR統計（送信ノード: tx_total > 0）
    try:
        tx_nodes = app_summary_df[app_summary_df['tx_total'] > 0]
        pdr_values = tx_nodes['pdr'].dropna()

        if len(pdr_values) > 0:
            result['pdr_mean'] = float(pdr_values.mean())
            result['pdr_min'] = float(pdr_values.min())
            result['pdr_max'] = float(pdr_values.max())
            result['pdr_std'] = float(pdr_values.std())
            result['pdr_node_count'] = len(pdr_values)
        else:
            result['pdr_mean'] = None
            result['pdr_min'] = None
            result['pdr_max'] = None
            result['pdr_std'] = None
            result['pdr_node_count'] = 0
    except Exception as e:
        print(f"PDR統計計算失敗: {e}")
        result['pdr_mean'] = None
        result['pdr_min'] = None
        result['pdr_max'] = None
        result['pdr_std'] = None
        result['pdr_node_count'] = 0

    # 遅延統計（送信ノード: tx_total > 0）
    try:
        tx_nodes = app_summary_df[app_summary_df['tx_total'] > 0]
        delay_values = tx_nodes['avg_delay'].dropna()

        if len(delay_values) > 0:
            result['delay_mean'] = float(delay_values.mean())
            result['delay_min'] = float(delay_values.min())
            result['delay_max'] = float(delay_values.max())
            result['delay_std'] = float(delay_values.std())
            result['delay_node_count'] = len(delay_values)
        else:
            result['delay_mean'] = None
            result['delay_min'] = None
            result['delay_max'] = None
            result['delay_std'] = None
            result['delay_node_count'] = 0
    except Exception as e:
        print(f"遅延統計計算失敗: {e}")
        result['delay_mean'] = None
        result['delay_min'] = None
        result['delay_max'] = None
        result['delay_std'] = None
        result['delay_node_count'] = 0

    # 起床時間統計（TRX_OFF_ratioから算出: wake_ratio = 1 - TRX_OFF_ratio）
    try:
        if 'TRX_OFF_ratio' in phy_summary_df.columns:
            trx_off_values = phy_summary_df['TRX_OFF_ratio'].dropna()

            if len(trx_off_values) > 0:
                # 起床時間比率 = 1 - TRX_OFF_ratio
                wake_ratios = 1 - trx_off_values

                result['wake_ratio_mean'] = float(wake_ratios.mean())
                result['wake_ratio_min'] = float(wake_ratios.min())
                result['wake_ratio_max'] = float(wake_ratios.max())
                result['wake_ratio_std'] = float(wake_ratios.std())
                result['wake_node_count'] = len(wake_ratios)
            else:
                result['wake_ratio_mean'] = None
                result['wake_ratio_min'] = None
                result['wake_ratio_max'] = None
                result['wake_ratio_std'] = None
                result['wake_node_count'] = 0
        else:
            print("PHYサマリーにTRX_OFF_ratioカラムが見つかりません")
            result['wake_ratio_mean'] = None
            result['wake_ratio_min'] = None
            result['wake_ratio_max'] = None
            result['wake_ratio_std'] = None
            result['wake_node_count'] = 0
    except Exception as e:
        print(f"起床時間統計計算失敗: {e}")
        result['wake_ratio_mean'] = None
        result['wake_ratio_min'] = None
        result['wake_ratio_max'] = None
        result['wake_ratio_std'] = None
        result['wake_node_count'] = 0

    return result
