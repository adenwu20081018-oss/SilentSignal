"""Make the two results graphs from test_results.csv.

Run from the repository root:  python results/plot_results.py
Needs: matplotlib
"""
import csv
from pathlib import Path

import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
TEXT, TEXT2, GRID = "#1a1a1a", "#666666", "#e6e6e6"
COLORS = {"knock": "#257de0", "doorbell": "#f47230", "other": "#00b894", "nothing": "#f2a600"}
LABELS = {"knock": "Knock", "doorbell": "Doorbell", "other": "Other", "nothing": "Nothing"}

plt.rcParams.update({"font.size": 11, "text.color": TEXT, "axes.labelcolor": TEXT2,
                     "xtick.color": TEXT2, "ytick.color": TEXT})

with open(HERE / "test_results.csv", newline="") as f:
    rows = list(csv.DictReader(f))
for r in rows:
    for k in ("trials", *COLORS):
        r[k] = int(r[k])
    # Target sounds are correct when labelled as themselves; trick sounds when they
    # don't trigger a knock or doorbell alert.
    if r["group"] == "target":
        want = "knock" if "knock" in r["scenario"].lower() else "doorbell"
        r["correct"] = r[want]
    else:
        r["correct"] = r["other"] + r["nothing"]

names = [r["scenario"] for r in rows][::-1]
ypos = range(len(rows))


def style(ax):
    for s in ("top", "right", "left"):
        ax.spines[s].set_visible(False)
    ax.spines["bottom"].set_color(GRID)
    ax.tick_params(axis="y", length=0)
    ax.xaxis.grid(True, color=GRID, linewidth=1)
    ax.set_axisbelow(True)


# Graph 1: correct rate per scenario
fig, ax = plt.subplots(figsize=(9, 4.8), dpi=150)
pct = [100 * r["correct"] / r["trials"] for r in rows][::-1]
ax.barh(ypos, pct, height=0.6, color=COLORS["knock"])
for y, p in zip(ypos, pct):
    ax.text(p + 1.5, y, f"{p:.0f}%", va="center", color=TEXT)
ax.set_yticks(list(ypos), names)
ax.set_xlim(0, 100)
ax.set_xlabel("Correct (%) out of 10 trials")
ax.axhline(2.5, color=TEXT2, linewidth=1, linestyle=(0, (3, 3)))
ax.text(99, 2.4, "Trick sounds: correct = no false alert", ha="right", va="top",
        color=TEXT2, fontsize=9)
ax.set_title("SilentSignal: correct rate by test", loc="left", fontweight="bold")
style(ax)
fig.tight_layout()
fig.savefig(HERE / "graph1_correct_rate.png", facecolor="white")

# Graph 2: what each sound was detected as
fig, ax = plt.subplots(figsize=(9, 5.2), dpi=150)
left = [0] * len(rows)
for k, c in COLORS.items():
    vals = [r[k] for r in rows][::-1]
    ax.barh(ypos, vals, left=left, height=0.6, color=c, edgecolor="white",
            linewidth=2, label=LABELS[k])
    for y, v, l in zip(ypos, vals, left):
        if v >= 1:
            ax.text(l + v / 2, y, str(v), ha="center", va="center", color=TEXT, fontsize=10)
    left = [a + b for a, b in zip(left, vals)]
ax.set_yticks(list(ypos), names)
ax.set_xlim(0, 10)
ax.set_xlabel("Number of trials (out of 10)")
ax.set_title("What SilentSignal detected for each sound", loc="left", fontweight="bold", pad=28)
ax.legend(ncol=4, loc="lower left", bbox_to_anchor=(0, 1.0), frameon=False,
          handlelength=1, columnspacing=1.2, fontsize=10)
style(ax)
fig.tight_layout()
fig.savefig(HERE / "graph2_detections.png", facecolor="white")
print("saved graph1_correct_rate.png and graph2_detections.png")
