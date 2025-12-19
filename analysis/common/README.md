# common directory: common logic, constants, and log schema

This directory consolidates common utilities, constants, and log-format schemas used by the log-analysis scripts to improve consistency and reusability across the codebase.

## Structure and responsibilities

- `config.py`
  → Definitions of constants used across scripts, e.g. log filenames and color schemes.

- `log_schema.py`
  → Column definitions (schemas) for each log file.
  Example: `MAC_TX_COLUMNS = ["time", "event", "frameType", "dstMac"]`

- `io_utils.py`
  → I/O helpers for loading log CSVs, building save paths, and existence checks.

- `frame_utils.py`
  → Helpers for normalizing frame types, checking broadcast addresses, formatting MAC addresses, etc.

- `plot_utils.py`
  → Matplotlib-based visualization utilities (planned / placeholder).

- `__init__.py`
  → Empty for now. Add package initialization here if needed in the future.

## Example usage

```python
from common.config import LOG_FILES
from common.io_utils import load_csv
from common.frame_utils import normalize_frame_type

df = load_csv("mac-txlog.csv")
df["frameType"] = df["frameType"].map(normalize_frame_type)

````
