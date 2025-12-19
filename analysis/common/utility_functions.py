import os

def check_existing_task_results(task_info, force_rerun=False):
    """
    既存のタスク結果をチェックし、スキップ可能かを判定

    Args:
        task_info (dict): タスク情報
        force_rerun (bool): 強制実行フラグ（デフォルト: False）

    Returns:
        dict: {
            'should_skip': bool,           # スキップすべきかどうか
            'existing_files': list,        # 存在するファイルリスト
            'missing_files': list,         # 欠損ファイルリスト
            'error_details': str,          # エラー詳細（エラー時のみ）
            'file_status': dict            # 各ファイルの詳細ステータス
        }
    """

    # 強制実行フラグチェック
    if force_rerun:
        return {
            'should_skip': False,
            'existing_files': [],
            'missing_files': ['app_summary.csv', 'mac_summary.csv', 'phy_summary.csv', 'scenario_summary.csv'],
            'file_status': {},
            'reason': 'Force rerun requested'
        }

    # SimulationConfig作成
    config = SimulationConfig(task_info["params"], task_info["base_script"])

    # チェック対象ファイル定義
    required_files = {
        'app_summary.csv': config.get_summary_path("app_summary.csv"),
        'mac_summary.csv': config.get_summary_path("mac_summary.csv"),
        'phy_summary.csv': config.get_summary_path("phy_summary.csv"),
        'scenario_summary.csv': config.get_summary_path("scenario_summary.csv")
    }

    file_status = {}

    # 各ファイルの状態をチェック
    for name, path in required_files.items():
        status = {
            'exists': False,
            'readable': False,
            'has_data': False,
            'file_size': 0,
            'row_count': 0,
            'error': None
        }

        try:
            # ファイル存在チェック
            if os.path.exists(path):
                status['exists'] = True
                status['file_size'] = os.path.getsize(path)

                # ファイルサイズチェック（0バイトファイルは無効）
                if status['file_size'] > 0:
                    try:
                        # CSV読み込みテスト
                        df = pd.read_csv(path)
                        status['readable'] = True
                        status['row_count'] = len(df)
                        status['has_data'] = len(df) > 0
                    except Exception as csv_error:
                        status['readable'] = False
                        status['error'] = f"CSV読み込みエラー: {str(csv_error)}"
                else:
                    status['error'] = "空ファイル"
            else:
                status['error'] = "ファイルが存在しません"

        except Exception as e:
            status['error'] = f"ファイルアクセスエラー: {str(e)}"

        file_status[name] = status

    # 判定: 必須ファイルがすべて有効かチェック
    all_required_valid = all(
        file_status[name]['exists'] and
        file_status[name]['readable'] and
        file_status[name]['has_data']
        for name in required_files.keys()
    )

    # 結果作成
    existing_files = [name for name, status in file_status.items()
                     if status.get('exists', False) and status.get('readable', False) and status.get('has_data', False)]
    missing_files = [name for name, status in file_status.items()
                    if not (status.get('exists', False) and status.get('readable', False) and status.get('has_data', False))]

    return {
        'should_skip': all_required_valid,
        'existing_files': existing_files,
        'missing_files': missing_files,
        'file_status': file_status
    }