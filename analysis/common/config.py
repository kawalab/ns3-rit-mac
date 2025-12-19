"""
Common configuration settings for analysis scripts.
"""

# Default paths
DEFAULT_LOG_ROOT = "logs"
DEFAULT_ANALYSIS_ROOT = "analysis"

# Plot settings
PLOT_STYLE = "ggplot"
FIGURE_SIZE = (12, 8)
DPI = 300

# Analysis parameters
ENERGY_THRESHOLD_MJ = 1000.0  # Energy threshold in mJ
TIMEOUT_THRESHOLD_MS = 100.0  # Timeout threshold in ms

# Color palette for visualization
COLORS = {
    'primary': '#1f77b4',
    'secondary': '#ff7f0e',
    'success': '#2ca02c',
    'danger': '#d62728',
    'warning': '#ff7f0e',
    'info': '#17a2b8'
}

# File naming conventions
LOG_FILES = {
    'mac_txlog': 'mac-txlog.csv',
    'mac_rxlog': 'mac-rxlog.csv',
    'mac_summary': 'mac-summary.csv',
    'energy_log': 'energy-log.csv'
}

# Analysis output settings
OUTPUT_FORMATS = ['png', 'pdf']
SAVE_INTERMEDIATE = True
