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
    """CSVをDataFrameで読み込む（共通オプション付き）"""
    return pd.read_csv(path, encoding='utf-8')


def save_csv(df: pd.DataFrame, path: str):
    """DataFrameをCSVで保存（インデックスなし）"""
    df.to_csv(path, index=False, encoding='utf-8')


def list_node_dirs(base_path: str) -> List[Path]:
    """`logs/<config>/` 以下の `node-*` ディレクトリを一覧取得"""
    base = Path(base_path)
    return sorted([d for d in base.iterdir() if d.is_dir() and d.name.startswith('node-')])


def get_global_log_path(config_path: str, filename: str) -> str:
    """グローバルログファイル（mac-summaryなど）のパスを取得"""
    return str(Path(config_path) / 'global' / filename)


def get_node_log_path(config_path: str, node_id: int, filename: str) -> str:
    """ノード個別ログファイルのパスを取得"""
    return str(Path(config_path) / f'node-{node_id}' / filename)


def read_log(node, filename, columns, base_dir, parameter_dir):
    """
    指定ノード・ファイル名・カラムでCSVログを読み込む関数。
    base_dir, parameter_dirは絶対パスで渡すこと。
    """
    # import os
    # import pandas as pd
    path = os.path.join(base_dir, parameter_dir, f"node-{node}", filename)
    # TODO: 壊れデータがある場合の処理を追加
    return pd.read_csv(path, header=None, names=columns, on_bad_lines='skip')


def find_node_dirs(base_dir, scenario_type, module_name, parameter_dir):
    """
    ワイルドカードでノードディレクトリ一覧を取得する関数。
    """
    # import os
    # import glob
    search_pattern = os.path.join(base_dir, scenario_type, module_name, parameter_dir, 'node-*')
    return glob.glob(search_pattern)


def remove_raw_logs(base_dir: str, parameter_dir: str):
    """
    指定したパラメータディレクトリの生ログ（ノードディレクトリとglobalディレクトリ）を削除する。
    サマリーCSVの生成が成功した後に容量削減のために使用。

    Args:
        base_dir (str): ログのベースディレクトリ（例: "/path/to/ns3/logs"）
        parameter_dir (str): パラメータディレクトリ名（例: "center_dense_periodic_csma_bprecs_BI5000_RDR5_WD5000_RUN05"）
    """
    log_path = os.path.join(base_dir, parameter_dir)

    if not os.path.exists(log_path):
        print(f"[WARNING] ログディレクトリが存在しません: {log_path}")
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

        # パラメータディレクトリが空になった場合は削除
        if not os.listdir(log_path):
            os.rmdir(log_path)
            removed_items.append(parameter_dir)

        print(f"[CLEANUP] 生ログ削除完了 {parameter_dir}: {removed_items}")

    except Exception as e:
        print(f"[ERROR] 生ログ削除失敗 for {parameter_dir}: {str(e)}")
        import traceback

        traceback.print_exc()
