import os

def check_existing_task_results(task_info, force_rerun=False):
    """
    Check existing task results and determine whether the task can be skipped.

    Args:
        task_info (dict): Task information
        force_rerun (bool): Force rerun flag (default: False)

    Returns:
        dict: {
            'should_skip': bool,           # Whether to skip
            'existing_files': list,        # List of existing files
            'missing_files': list,         # List of missing files
            'error_details': str,          # Error details (only on error)
            'file_status': dict            # Per-file status details
        }
    """

    # Check force-rerun flag
    if force_rerun:
        return {
            'should_skip': False,
            'existing_files': [],
            'missing_files': ['app_summary.csv', 'mac_summary.csv', 'phy_summary.csv', 'scenario_summary.csv'],
            'file_status': {},
            'reason': 'Force rerun requested'
        }

    # Create SimulationConfig
    config = SimulationConfig(task_info["params"], task_info["base_script"])

    # Define files to check
    required_files = {
        'app_summary.csv': config.get_summary_path("app_summary.csv"),
        'mac_summary.csv': config.get_summary_path("mac_summary.csv"),
        'phy_summary.csv': config.get_summary_path("phy_summary.csv"),
        'scenario_summary.csv': config.get_summary_path("scenario_summary.csv")
    }

    file_status = {}

    # Check status of each file
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
            # File existence check
            if os.path.exists(path):
                status['exists'] = True
                status['file_size'] = os.path.getsize(path)

                # File size check (0-byte files are invalid)
                if status['file_size'] > 0:
                    try:
                        # CSV read test
                        df = pd.read_csv(path)
                        status['readable'] = True
                        status['row_count'] = len(df)
                        status['has_data'] = len(df) > 0
                    except Exception as csv_error:
                        status['readable'] = False
                        status['error'] = f"CSV read error: {str(csv_error)}"
                else:
                    status['error'] = "empty file"
            else:
                status['error'] = "file not found"

        except Exception as e:
            status['error'] = f"file access error: {str(e)}"

        file_status[name] = status

    # Determine if all required files are valid
    all_required_valid = all(
        file_status[name]['exists'] and
        file_status[name]['readable'] and
        file_status[name]['has_data']
        for name in required_files.keys()
    )

    # Build result
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