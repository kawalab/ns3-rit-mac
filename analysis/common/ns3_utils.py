"""
Utility for running ns-3 simulation from SimulationConfig.
"""

def run_ns3_simulation(config):
    import subprocess
    try:
        result = subprocess.run(config.ns3_command, shell=True, check=True, text=True, capture_output=True, cwd=config.ns3_working_dir)
        print("=== ns-3 実行結果 ===")
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print("ns-3 実行に失敗しました:", e.stderr)

def run_ns3_simulation_with_result(config, log_path=None):
    """
    ns-3シミュレーションを実行し、結果（stdout, stderr, returncode）を返す。
    log_pathを指定すると標準出力をファイル保存。
    """
    import subprocess
    try:
        result = subprocess.run(config.ns3_command, shell=True, check=True, text=True, capture_output=True, cwd=config.ns3_working_dir)
        if log_path:
            with open(log_path, 'w') as f:
                f.write(result.stdout)
        return {
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode,
            'success': True
        }
    except subprocess.CalledProcessError as e:
        if log_path:
            with open(log_path, 'w') as f:
                f.write(e.stdout or '')
        return {
            'stdout': e.stdout,
            'stderr': e.stderr,
            'returncode': e.returncode,
            'success': False
        }
