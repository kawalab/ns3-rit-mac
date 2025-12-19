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
            print(f"Failed to aggregate app node {node}: {e}")
            # Print more detailed error info
            node_dir = f"{base_dir}/{parameter_dir}/node-{node}"
            print(f"  -> node directory: {node_dir}")
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
            print(f"Failed to aggregate MAC node {node}: {e}")
            # Print more detailed error info
            node_dir = f"{base_dir}/{parameter_dir}/node-{node}"
            print(f"  -> node directory: {node_dir}")
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
            print(f"Failed to aggregate PHY node {node}: {e}")
            # Print more detailed error info
            node_dir = f"{base_dir}/{parameter_dir}/node-{node}"
            print(f"  -> node directory: {node_dir}")
    if results:
        return pd.DataFrame(results).set_index("nodeId")
    else:
        # Return empty DataFrame with nodeId column
        return pd.DataFrame(columns=["nodeId"]).set_index("nodeId")

def aggregate_scenario_summary(app_summary_df, phy_summary_df):
    """
    Create scenario-level statistical summary.
    Computes aggregated statistics across nodes from APP and PHY summaries.

    Args:
        app_summary_df (pd.DataFrame): Application-layer summary DataFrame
        phy_summary_df (pd.DataFrame): PHY-layer summary DataFrame

    Returns:
        dict: Scenario-level statistics
    """
    try:
        return summarize_scenario(app_summary_df, phy_summary_df)
    except Exception as e:
        print(f"Failed to aggregate scenario summary: {e}")
        return {}
