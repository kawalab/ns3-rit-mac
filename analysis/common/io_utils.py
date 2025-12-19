"""
Input/Output utilities for log file handling.
"""

import pandas as pd
import os
import glob
import shutil
from pathlib import Path
from typing import List


def load_csv(path: str) -> pd.DataFrame:
    """Read a CSV file into a pandas DataFrame (with common options)."""
    return pd.read_csv(path, encoding='utf-8')


def save_csv(df: pd.DataFrame, path: str):
    """Save a DataFrame to CSV (without index)."""
    df.to_csv(path, index=False, encoding='utf-8')


def list_node_dirs(base_path: str) -> List[Path]:
    """List `node-*` directories under `logs/<config>/`."""
    base = Path(base_path)
    return sorted([d for d in base.iterdir() if d.is_dir() and d.name.startswith('node-')])


def get_global_log_path(config_path: str, filename: str) -> str:
    """Get the path to a global log file (e.g., mac-summary)."""
    return str(Path(config_path) / 'global' / filename)


def get_node_log_path(config_path: str, node_id: int, filename: str) -> str:
    """Get the path to a node-specific log file."""
    return str(Path(config_path) / f'node-{node_id}' / filename)


def read_log(node, filename, columns, base_dir, parameter_dir):
    """
    Read a CSV log for the specified node, filename, and columns.
    Pass `base_dir` and `parameter_dir` as absolute paths.
    """
    # import os
    # import pandas as pd
    path = os.path.join(base_dir, parameter_dir, f"node-{node}", filename)
    # TODO: Add handling for corrupted/broken data
    return pd.read_csv(path, header=None, names=columns, on_bad_lines='skip')


def find_node_dirs(base_dir, scenario_type, module_name, parameter_dir):
    """
    Get a list of node directories using a wildcard search.
    """
    # import os
    # import glob
    search_pattern = os.path.join(base_dir, scenario_type, module_name, parameter_dir, 'node-*')
    return glob.glob(search_pattern)


def remove_raw_logs(base_dir: str, parameter_dir: str):
    """
    Remove raw logs (node directories and the global directory) for the specified parameter directory.
    Use this to reduce disk usage after successfully generating summary CSVs.

    Args:
        base_dir (str): Base directory for logs (e.g., "/path/to/ns3/logs")
        parameter_dir (str): Parameter-specific directory name (e.g., "center_dense_periodic_csma_bprecs_BI5000_RDR5_WD5000_RUN05")
    """
    log_path = os.path.join(base_dir, parameter_dir)

    if not os.path.exists(log_path):
        print(f"[WARNING] Log directory does not exist: {log_path}")
        return

    removed_items = []

    try:
        # node-*ディレクトリを削除
        for item in os.listdir(log_path):
            item_path = os.path.join(log_path, item)
            if os.path.isdir(item_path):
                if item.startswith('node-') or item == 'global':
                    shutil.rmtree(item_path)
                    removed_items.append(item)

        # Remove parameter directory if it becomes empty
        if not os.listdir(log_path):
            os.rmdir(log_path)
            removed_items.append(parameter_dir)

        print(f"[CLEANUP] Raw log removal complete for {parameter_dir}: {removed_items}")

    except Exception as e:
        print(f"[ERROR] Failed to remove raw logs for {parameter_dir}: {str(e)}")
        import traceback

        traceback.print_exc()
