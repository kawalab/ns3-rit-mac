# common ディレクトリ：共通処理・定数定義・ログ形式の集約

このディレクトリは、ログ解析スクリプトにおける「共通処理」「定数」「ログ形式スキーマ」などを集約し、各スクリプト間での一貫性と再利用性を高めることを目的としています。

## 構成と役割

- `config.py`
  → 各種ログファイル名や色定義など、共通で使用する定数の定義

- `log_schema.py`
  → 各ログファイルのカラム構成（スキーマ）を定義
  例：`MAC_TX_COLUMNS = ["time", "event", "frameType", "dstMac"]`

- `io_utils.py`
  → ログCSVの読み込み、保存パス構築、存在チェックなどの I/O 関数

- `frame_utils.py`
  → フレーム種別の正規化、ブロードキャストアドレス判定、MAC表現整形などの補助処理

- `plot_utils.py`
  → 将来的に matplotlib ベースの可視化処理を記述予定

- `__init__.py`
  → 空ファイル。将来的にパッケージ読み込み時の初期化が必要なら記述

## 使用例

```python
from common.config import LOG_FILES
from common.io_utils import load_csv
from common.frame_utils import normalize_frame_type

df = load_csv("mac-txlog.csv")
df["frameType"] = df["frameType"].map(normalize_frame_type)
