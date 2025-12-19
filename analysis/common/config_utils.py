"""
SimulationConfig class for unified parameter and path management.
"""

import os

class SimulationConfig:
    def __init__(self, simulation_params, base_script, ns3_working_dir="~/workspace/ns3-rit-mac", summary_dir="./summary"):
        # パラメータの型確認・正規化
        self.simulation_params = self._normalize_params(simulation_params)
        self.base_script = base_script
        self.ns3_working_dir = os.path.expanduser(ns3_working_dir)
        self.summary_dir = summary_dir
        self.config = self.extract_config(self.simulation_params)
        self.module_name = self.generate_module_name(self.config)
        self.scenario_type = self.generate_scenario_type(self.simulation_params)
        self.parameter_dir = self.generate_log_base_dir()
        self.ns3_command = self.generate_ns3_command()

    def _normalize_params(self, params):
        """パラメータの型を正規化"""
        numeric_params = {"BI", "TWD", "DWD", "Nodes", "Days", "DR", "AppPacketSize", "Seed"}
        normalized = {}
        for key, value in params.items():
            if key in numeric_params:
                try:
                    normalized[key] = int(value) if key != "DR" else float(value)
                except (ValueError, TypeError):
                    print(f"[WARNING] {key}の型変換失敗: {value}, デフォルト値を使用")
                    normalized[key] = 0
            else:
                normalized[key] = str(value)
        return normalized

    def extract_config(self, simulation_params):
        return {
            'dataCsmaEnabled': simulation_params['DataCsma'] == "true",
            'dataPreCsEnabled': simulation_params['DataPreCs'] == "true",
            'dataPreCsBEnabled': simulation_params['DataPreCsB'] == "true",
            'beaconCsmaEnabled': simulation_params['BeaconCsma'] == "true",
            'beaconPreCsEnabled': simulation_params['BeaconPreCs'] == "true",
            'beaconPreCsBEnabled': simulation_params['BeaconPreCsB'] == "true",
            'continuousTxEnabled': simulation_params['ContinuousTx'] == "true",
            'beaconRandomizeEnabled': simulation_params['BeaconRandomize'] == "true",
            'compactRitDataRequestEnabled': simulation_params['CompactRitDataRequest'] == "true",
            'beaconAckEnabled': simulation_params['BeaconAck'] == "true"
        }

    def generate_module_name(self, config):
        tags = []
        if config['dataCsmaEnabled'] and config['dataPreCsEnabled']:
            tags.append("csma_precs")
        elif config['dataCsmaEnabled']:
            tags.append("csma")
        elif config['dataPreCsEnabled']:
            tags.append("precs")
        elif config['dataPreCsBEnabled']:
            tags.append("precsb")
        else:
            tags.append("nocsma")
        if config['beaconCsmaEnabled'] and config['beaconPreCsEnabled']:
            tags.append("bcsma_bprecs")
        elif config['beaconCsmaEnabled']:
            tags.append("bcsma")
        elif config['beaconPreCsEnabled']:
            tags.append("bprecs")
        elif config['beaconPreCsBEnabled']:
            tags.append("bprecsb")
        else:
            tags.append("bnocsma")
        if config['continuousTxEnabled']:
            tags.append("cont")
        if config['beaconRandomizeEnabled']:
            tags.append("random")
        if config['compactRitDataRequestEnabled']:
            tags.append("compact")
        if config['beaconAckEnabled']:
            tags.append("back")
        return "_".join(tags)

    def generate_scenario_type(self, params):
        return f"{params['Placement']}_{params['Density']}_{params['App']}"

    def generate_log_base_dir(self):
        mac_rit_period = self.simulation_params["BI"]
        mac_rit_tx_wait_duration = self.simulation_params["TWD"]
        mac_rit_data_wait_duration = self.simulation_params["DWD"]
        simulation_days = self.simulation_params["Days"]
        run_number = self.simulation_params["Seed"]

        return f"{self.scenario_type}/{self.module_name}/BI{mac_rit_period}_TWD{mac_rit_tx_wait_duration}_DWD{mac_rit_data_wait_duration}_Days{simulation_days}/SEED{run_number:02d}/"

    def generate_ns3_command(self):
        scenario_args = f"--Placement={self.simulation_params['Placement']} --Density={self.simulation_params['Density']} --App={self.simulation_params['App']}"

        # パラメータの順序を明示的に制御
        param_order = ['BI', 'TWD', 'DWD', 'Nodes', 'Days', 'DR', 'Seed', 'DataCsma', 'DataPreCs', 'BeaconCsma', 'BeaconPreCs', 'ContinuousTx', 'BeaconRandomize', 'CompactRitDataRequest', 'BeaconAck']
        ordered_args = []
        for key in param_order:
            if key in self.simulation_params and key not in ['Placement', 'Density', 'App']:
                ordered_args.append(f"--{key}={self.simulation_params[key]}")

        # 順序指定にない残りのパラメータも追加
        for key, value in self.simulation_params.items():
            if key not in param_order and key not in ['Placement', 'Density', 'App']:
                ordered_args.append(f"--{key}={value}")

        args = " ".join(ordered_args)
        command = f"./ns3 run {self.base_script} -- {scenario_args} {args}"

        return command

    def get_log_path(self, node, filename):
        return os.path.join(self.ns3_working_dir, "logs", self.parameter_dir, f"node-{node}", filename)

    def get_summary_path(self, filename):
        # ログディレクトリ内にsummaryフォルダを作成: ns3_working_dir/logs/parameter_dir/summary/
        summary_dir = os.path.join(self.ns3_working_dir, "logs", self.parameter_dir, "summary")
        return os.path.join(summary_dir, filename)
