"""
High-level summary aggregation utilities for notebook use.
"""
from common.summary_utils import summarize_app_node, summarize_mac_node, summarize_phy_node, summarize_scenario
import pandas as pd

def aggregate_app_summary(app_send_nodes, app_recv_node, app_txlog, app_rxlog, base_dir, parameter_dir):
    results = []
    for node in app_send_nodes + [app_recv_node]:
        try:
            summary = summarize_app_node(node, app_txlog, app_rxlog, base_dir, parameter_dir, app_recv_node)
            results.append(summary)
        except Exception as e:
            print(f"Appノード{node}の集計失敗: {e}")
            # より詳細なエラー情報を出力
            node_dir = f"{base_dir}/{parameter_dir}/node-{node}"
            print(f"  -> ノードディレクトリ: {node_dir}")
    if results:
        return pd.DataFrame(results).set_index("nodeId")
    else:
        # Return empty DataFrame with nodeId column
        return pd.DataFrame(columns=["nodeId"]).set_index("nodeId")

def aggregate_mac_summary(mac_nodes, mac_log_files, base_dir, parameter_dir):
    results = []
    for node in mac_nodes:
        try:
            summary = summarize_mac_node(node, mac_log_files, base_dir, parameter_dir)
            results.append(summary)
        except Exception as e:
            print(f"MACノード{node}の集計失敗: {e}")
            # より詳細なエラー情報を出力
            node_dir = f"{base_dir}/{parameter_dir}/node-{node}"
            print(f"  -> ノードディレクトリ: {node_dir}")
    if results:
        return pd.DataFrame(results).set_index("nodeId")
    else:
        # Return empty DataFrame with nodeId column
        return pd.DataFrame(columns=["nodeId"]).set_index("nodeId")

def aggregate_phy_summary(mac_nodes, base_dir, parameter_dir):
    results = []
    for node in mac_nodes:
        try:
            summary = summarize_phy_node(node, base_dir, parameter_dir)
            results.append(summary)
        except Exception as e:
            print(f"PHYノード{node}の集計失敗: {e}")
            # より詳細なエラー情報を出力
            node_dir = f"{base_dir}/{parameter_dir}/node-{node}"
            print(f"  -> ノードディレクトリ: {node_dir}")
    if results:
        return pd.DataFrame(results).set_index("nodeId")
    else:
        # Return empty DataFrame with nodeId column
        return pd.DataFrame(columns=["nodeId"]).set_index("nodeId")

def aggregate_scenario_summary(app_summary_df, phy_summary_df):
    """
    シナリオレベルの統計サマリーを作成する関数。
    APPサマリーとPHYサマリーから、ノード全体での統計値を算出。

    Args:
        app_summary_df (pd.DataFrame): アプリケーション層サマリーのDataFrame
        phy_summary_df (pd.DataFrame): PHY層サマリーのDataFrame

    Returns:
        dict: シナリオレベルの統計値
    """
    try:
        return summarize_scenario(app_summary_df, phy_summary_df)
    except Exception as e:
        print(f"シナリオサマリー集計失敗: {e}")
        return {}
