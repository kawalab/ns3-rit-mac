"""
Plotting utilities for consistent visualization.
"""

import matplotlib.pyplot as plt
import matplotlib.style as mplstyle
from pathlib import Path
from .config import PLOT_STYLE, FIGURE_SIZE, DPI, OUTPUT_FORMATS


def set_plot_style():
    """Initialize global plotting styles and fonts."""
    mplstyle.use(PLOT_STYLE)
    plt.rcParams['figure.figsize'] = FIGURE_SIZE
    plt.rcParams['figure.dpi'] = DPI
    plt.rcParams['font.size'] = 12
    plt.rcParams['axes.labelsize'] = 14
    plt.rcParams['axes.titlesize'] = 16
    plt.rcParams['legend.fontsize'] = 12
    plt.rcParams['xtick.labelsize'] = 10
    plt.rcParams['ytick.labelsize'] = 10


def save_fig(fig, path: str, formats: list = None):
    """Save a matplotlib Figure object to one or more files."""
    if formats is None:
        formats = OUTPUT_FORMATS

    path_obj = Path(path)
    # Create directory if it doesn't exist
    path_obj.parent.mkdir(parents=True, exist_ok=True)

    for fmt in formats:
        output_path = path_obj.with_suffix(f'.{fmt}')
        fig.savefig(output_path, dpi=DPI, bbox_inches='tight')
        print(f"Saved: {output_path}")


def create_subplot_grid(rows: int, cols: int, figsize: tuple = None):
    """Create a grid of subplots."""
    if figsize is None:
        figsize = (FIGURE_SIZE[0], FIGURE_SIZE[1] * rows / 2)

    fig, axes = plt.subplots(rows, cols, figsize=figsize)
    if rows == 1 and cols == 1:
        axes = [axes]
    elif rows == 1 or cols == 1:
        axes = axes.flatten()

    return fig, axes


def add_grid_and_labels(ax, xlabel: str = None, ylabel: str = None, title: str = None):
    """Add grid, axis labels, and title to an axis."""
    ax.grid(True, alpha=0.3)
    if xlabel:
        ax.set_xlabel(xlabel)
    if ylabel:
        ax.set_ylabel(ylabel)
    if title:
        ax.set_title(title)
