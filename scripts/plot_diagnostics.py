"""Plot saved serial diagnostics. Install: pip install plotly

Optional PNG support: pip install kaleido
Usage: python scripts/plot_diagnostics.py diagnostic.log [--output-dir plots] [--png]
Run without a file to plot EXAMPLE_DATA below.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

EXAMPLE_DATA = """\
[100] DIAG_BEGIN type=switch id=1 result=complete duration_ms=1000 events=7 overflow=0 board=pop-up-controller-v10-r firmware=dev
[101] DIAG_EVENT id=1 ms=0 kind=snapshot source=1 from=0 to=0 duration_ms=0
[102] DIAG_EVENT id=1 ms=0 kind=snapshot source=2 from=0 to=0 duration_ms=0
[103] DIAG_EVENT id=1 ms=100 kind=raw source=2 from=0 to=1 duration_ms=0
[104] DIAG_EVENT id=1 ms=130 kind=debounced source=2 from=0 to=1 duration_ms=0
[105] DIAG_EVENT id=1 ms=500 kind=debounced source=2 from=1 to=0 duration_ms=0
[106] DIAG_EVENT id=1 ms=500 kind=raw source=2 from=1 to=0 duration_ms=0
[107] DIAG_EVENT id=1 ms=700 kind=command source=3 from=1 to=0 duration_ms=0
[108] DIAG_END id=1
[200] DIAG_BEGIN type=motion id=2 result=complete duration_ms=900 events=5 overflow=0 board=pop-up-controller-v10-r firmware=dev
[201] DIAG_EVENT id=2 ms=0 kind=snapshot source=3 from=0 to=0 duration_ms=0
[202] DIAG_EVENT id=2 ms=0 kind=snapshot source=4 from=0 to=0 duration_ms=0
[203] DIAG_EVENT id=2 ms=100 kind=command source=3 from=0 to=1 duration_ms=0
[204] DIAG_EVENT id=2 ms=200 kind=position source=3 from=0 to=2 duration_ms=0
[205] DIAG_EVENT id=2 ms=660 kind=moveDone source=3 from=2 to=1 duration_ms=560
[206] DIAG_END id=2
"""

SWITCH = {1: "UP", 2: "HOLD"}
MOTOR = {3: "RH", 4: "LH", 5: "Both"}
POSITION = {0: "DOWN", 1: "UP", 2: "IN_BETWEEN", 3: "IDLE", 4: "TIMEOUT"}
ABORT = {1: "switch became active", 2: "motor lockout"}
KNOWN_KINDS = {"snapshot", "raw", "debounced", "command", "position", "moveDone", "abort"}
RECORD = re.compile(r"\b(DIAG_BEGIN|DIAG_EVENT|DIAG_END)\b")
PAIR = re.compile(r"([A-Za-z_]+)=([^\s]+)")


def warn(message: str) -> None:
    print(f"Warning: {message}", file=sys.stderr)


def number(fields: dict[str, str], key: str, default: int = 0) -> int:
    try:
        return int(fields.get(key, str(default)))
    except ValueError:
        warn(f"invalid {key}={fields[key]!r}; using {default}")
        return default


@dataclass
class DiagnosticEvent:
    id: int
    ms: int
    kind: str
    source: int
    from_state: int
    to_state: int
    duration_ms: int


@dataclass
class DiagnosticTest:
    type: str
    id: int
    result: str
    duration_ms: int
    expected_events: int
    overflow: bool
    board: str
    firmware: str
    events: list[DiagnosticEvent] = field(default_factory=list)
    ended: bool = False


def parse_diagnostics(text: str) -> list[DiagnosticTest]:
    tests: list[DiagnosticTest] = []
    current: DiagnosticTest | None = None
    for line in text.splitlines():
        match = RECORD.search(line)
        if not match:
            continue
        tag = match.group(1)
        fields = dict(PAIR.findall(line[match.end():]))
        if tag == "DIAG_BEGIN":
            if current and not current.ended:
                warn(f"test {current.id} has no matching DIAG_END")
            current = DiagnosticTest(
                fields.get("type", "unknown"), number(fields, "id"),
                fields.get("result", "unknown"), number(fields, "duration_ms"),
                number(fields, "events"), bool(number(fields, "overflow")),
                fields.get("board", "unknown"), fields.get("firmware", "unknown"),
            )
            tests.append(current)
        elif current is None or current.ended:
            warn(f"{tag} outside a diagnostic; skipped")
        elif number(fields, "id", -1) != current.id:
            warn(f"{tag} ID does not match test {current.id}; skipped")
        elif tag == "DIAG_END":
            current.ended = True
            if len(current.events) != current.expected_events:
                warn(f"test {current.id}: events={current.expected_events}, parsed={len(current.events)}")
            if current.overflow:
                warn(f"test {current.id}: diagnostic buffer overflow; later events may be missing")
        else:
            event = DiagnosticEvent(
                number(fields, "id"), number(fields, "ms"), fields.get("kind", "unknown"),
                number(fields, "source", -1), number(fields, "from"),
                number(fields, "to"), number(fields, "duration_ms"),
            )
            if event.kind not in KNOWN_KINDS:
                warn(f"test {current.id}: unknown event kind {event.kind!r}")
            if event.source not in SWITCH | MOTOR:
                warn(f"test {current.id}: unknown source {event.source}")
            current.events.append(event)
    if current and not current.ended:
        warn(f"test {current.id} has no matching DIAG_END")
    return tests


def label(mapping: dict[int, str], value: int) -> str:
    return mapping.get(value, f"Unknown ({value})")


def base_figure(test: DiagnosticTest, title: str):
    import plotly.graph_objects as go

    fig = go.Figure()
    fig.update_layout(
        template="plotly_dark", title=f"{title} · Test {test.id} · {test.result}",
        paper_bgcolor="#111820", plot_bgcolor="#111820", font=dict(color="#d9e3ed"),
        height=620, margin=dict(l=130, r=45, t=105, b=70),
        legend=dict(orientation="h", y=1.02, x=0), hovermode="closest",
    )
    fig.update_xaxes(title="Time from test start (ms)", range=[0, max(1, test.duration_ms)],
                     gridcolor="#283440", zeroline=False)
    fig.update_yaxes(gridcolor="#283440", zeroline=False)
    fig.add_annotation(x=1, y=1.13, xref="paper", yref="paper", showarrow=False,
                       xanchor="right", text=f"{test.board} · {test.firmware} · {test.duration_ms} ms",
                       font=dict(size=11, color="#99aabb"))
    if test.overflow:
        fig.add_annotation(x=0.5, y=1.19, xref="paper", yref="paper", showarrow=False,
                           text="WARNING: diagnostic buffer overflow — later events may be missing",
                           font=dict(size=16, color="#ff6b6b"))
    return fig


def step_trace(fig, test: DiagnosticTest, name: str, source: int, kind: str,
               baseline: float, color: str, motion: bool = False) -> None:
    import plotly.graph_objects as go

    relevant = [e for e in test.events if e.source == source and
                (e.kind == "snapshot" or e.kind == kind)]
    relevant.sort(key=lambda e: e.ms)
    if not relevant:
        warn(f"test {test.id}: no initial data for {name}")
        return
    first = relevant[0]
    if first.kind != "snapshot":
        warn(f"test {test.id}: {name} has no snapshot; trace starts at first transition")
    start = 0 if first.kind == "snapshot" else first.ms
    state = first.to_state if motion or kind == "debounced" else first.from_state
    x = [start]
    y = [baseline + state * (0.15 if motion else 0.55)]
    hover = [f"{name}<br>{start} ms<br>{label(POSITION, state) if motion else state}"]
    for e in relevant[1:]:
        if e.kind == "snapshot":
            warn(f"test {test.id}: repeated snapshot for {name}; ignored")
            continue
        state = e.to_state
        x.append(e.ms)
        y.append(baseline + state * (0.15 if motion else 0.55))
        hover.append(f"{name}<br>{e.ms} ms<br>{label(POSITION, state) if motion else state}")
    x.append(max(test.duration_ms, x[-1]))
    y.append(y[-1])
    hover.append(hover[-1])
    fig.add_trace(go.Scatter(x=x, y=y, mode="lines+markers", name=name,
                             line=dict(color=color, width=2, shape="hv"),
                             marker=dict(size=5), text=hover, hovertemplate="%{text}<extra></extra>"))


def event_markers(fig, events: list[DiagnosticEvent], kind: str, y: float, color: str,
                  symbol: str, hover_texts: list[str]) -> None:
    if not events:
        return
    import plotly.graph_objects as go

    fig.add_trace(go.Scatter(x=[e.ms for e in events], y=[y] * len(events), mode="markers",
                             name=kind, marker=dict(color=color, symbol=symbol, size=12),
                             text=hover_texts, hovertemplate="%{text}<extra></extra>"))


def build_switch_figure(test: DiagnosticTest):
    fig = base_figure(test, "Switch diagnostic")
    lanes = [("UP raw", 1, "raw", 0, "#42d6d0"),
             ("UP debounced", 1, "debounced", 1, "#65a9ff"),
             ("HOLD raw", 2, "raw", 2, "#ffc86a"),
             ("HOLD debounced", 2, "debounced", 3, "#ff79aa")]
    for name, source, kind, lane, color in lanes:
        step_trace(fig, test, name, source, kind, float(lane), color)

    # Snapshot values establish the initial state; only debounced events change it.
    state: dict[int, int] = {}
    inactive_since: int | None = None
    intervals: list[tuple[int, int]] = []
    inactive_at_command: dict[int, int] = {}
    for e in sorted(test.events, key=lambda event: event.ms):
        if e.kind == "snapshot" and e.source in SWITCH:
            state[e.source] = e.to_state
        elif e.kind == "debounced" and e.source in SWITCH:
            state[e.source] = e.to_state
        elif e.kind == "command" and e.to_state == 0 and inactive_since is not None:
            inactive_at_command[id(e)] = e.ms - inactive_since
        else:
            continue
        both_inactive = len(state) == 2 and all(value == 0 for value in state.values())
        if both_inactive and inactive_since is None:
            inactive_since = 0 if e.kind == "snapshot" and e.ms == 0 else e.ms
        elif not both_inactive and inactive_since is not None:
            intervals.append((inactive_since, e.ms))
            inactive_since = None
    if inactive_since is not None:
        intervals.append((inactive_since, test.duration_ms))
    for start, end in intervals:
        if end > start:
            fig.add_vrect(x0=start, x1=end, fillcolor="#77b894", opacity=0.08, line_width=0,
                          layer="below")

    commands = [e for e in test.events if e.kind == "command"]
    hovers = []
    for e in commands:
        target = label(POSITION, e.to_state)
        detail = f"{e.ms} ms<br>{label(MOTOR, e.source)} command {target}"
        if e.to_state == 0 and id(e) in inactive_at_command:
            detail += f"<br>Both switches inactive for {inactive_at_command[id(e)]} ms"
        hovers.append(detail)
    event_markers(fig, commands, "Commands", -1, "#a78bfa", "diamond", hovers)
    fig.update_yaxes(range=[3.8, -1.5], tickvals=[-1, 0.28, 1.28, 2.28, 3.28],
                     ticktext=["Commands", "UP raw", "UP debounced", "HOLD raw", "HOLD debounced"])
    return fig


def build_motion_figure(test: DiagnosticTest):
    fig = base_figure(test, "Motion diagnostic")
    step_trace(fig, test, "RH position", 3, "position", 0, "#42d6d0", motion=True)
    step_trace(fig, test, "LH position", 4, "position", 1, "#ffc86a", motion=True)
    for kind, y, color, symbol in [("command", -1, "#a78bfa", "diamond"),
                                    ("moveDone", -2, "#71d995", "circle"),
                                    ("abort", -3, "#ff6b6b", "x")]:
        events = [e for e in test.events if e.kind == kind]
        hovers = []
        for e in events:
            detail = f"{e.ms} ms<br>{label(MOTOR, e.source)} "
            if kind == "command":
                detail += f"command {label(POSITION, e.to_state)}"
            elif kind == "moveDone":
                detail += f"moveDone: {label(POSITION, e.to_state)}<br>Duration: {e.duration_ms} ms"
            else:
                detail += f"abort: {label(ABORT, e.to_state)}<br>Duration: {e.duration_ms} ms"
            hovers.append(detail)
        event_markers(fig, events, kind, y, color, symbol, hovers)
    has_abort = any(e.kind == "abort" for e in test.events)
    low = -3.5 if has_abort else -2.5
    ticks = [-1, -2] + ([-3] if has_abort else []) + [0.3, 1.3]
    labels = ["Commands", "moveDone"] + (["Abort"] if has_abort else []) + ["RH position", "LH position"]
    fig.update_yaxes(range=[1.85, low], tickvals=ticks, ticktext=labels)
    return fig


def save_figure(fig, test: DiagnosticTest, output_dir: Path, png: bool) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    stem = f"diagnostic_{test.type}_{test.id}"
    path = output_dir / f"{stem}.html"
    fig.write_html(str(path), include_plotlyjs=True, full_html=True)
    if png:
        try:
            fig.write_image(str(output_dir / f"{stem}.png"))
        except Exception as exc:
            warn(f"PNG export failed for test {test.id}: {exc}")
    return path


def main() -> int:
    parser = argparse.ArgumentParser(description="Plot ESP32 serial diagnostics as interactive HTML")
    parser.add_argument("input", nargs="?", type=Path, help=".txt or .log serial capture; defaults to EXAMPLE_DATA")
    parser.add_argument("--output-dir", type=Path, default=Path("plots"))
    parser.add_argument("--png", action="store_true", help="also export PNG (requires Kaleido)")
    args = parser.parse_args()
    if args.input and args.input.suffix.lower() not in {".txt", ".log"}:
        parser.error("input must be a .txt or .log file")
    try:
        source = args.input.read_text(encoding="utf-8", errors="replace") if args.input else EXAMPLE_DATA
    except OSError as exc:
        parser.error(str(exc))
    tests = parse_diagnostics(source)
    made = 0
    for test in tests:
        if not test.ended:
            warn(f"test {test.id}: skipping incomplete diagnostic")
            continue
        if test.type == "switch":
            fig = build_switch_figure(test)
        elif test.type == "motion":
            fig = build_motion_figure(test)
        else:
            warn(f"test {test.id}: unknown type {test.type!r}; skipped")
            continue
        path = save_figure(fig, test, args.output_dir, args.png)
        print(path)
        made += 1
    if not made:
        warn("no complete switch or motion diagnostics found")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
