"""
TCP Header Data Modulation and Signaling
Flask Web Frontend & API Gateway
=========================================
Connects the web dashboard to the compiled C networking backend
via subprocess execution and JSON IPC.
"""

import os
import sys
import json
import subprocess
from pathlib import Path
from flask import Flask, render_template, request, jsonify

# Define directories
PROJECT_ROOT = Path(__file__).resolve().parent.parent
BACKEND_DIR = PROJECT_ROOT / "backend"
BACKEND_BIN = BACKEND_DIR / "bin" / "backend_engine.exe"
if not BACKEND_BIN.exists() and (BACKEND_DIR / "bin" / "backend.exe").exists():
    BACKEND_BIN = BACKEND_DIR / "bin" / "backend.exe"

app = Flask(__name__)


def get_c_backend_status():
    """Query the C backend executable for its readiness status."""
    if not BACKEND_BIN.exists():
        return {
            "available": False,
            "status": "NOT_COMPILED",
            "error": f"C backend executable not found at {BACKEND_BIN}. Please run 'make' inside the backend/ directory.",
            "methods": []
        }

    try:
        proc = subprocess.run(
            [str(BACKEND_BIN), "--json"],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=5
        )
        if proc.returncode == 0:
            data = json.loads(proc.stdout)
            data["available"] = True
            data["executable_path"] = str(BACKEND_BIN)
            return data
        else:
            return {
                "available": False,
                "status": "ERROR",
                "error": f"C backend exited with code {proc.returncode}: {proc.stderr}",
                "methods": []
            }
    except Exception as e:
        return {
            "available": False,
            "status": "SUBPROCESS_FAILED",
            "error": str(e),
            "methods": []
        }


@app.route("/")
def index():
    """Render the main research dashboard."""
    backend_info = get_c_backend_status()
    return render_template("index.html", backend_info=backend_info)


@app.route("/api/backend-status", methods=["GET"])
def api_backend_status():
    """API endpoint to get live C backend status."""
    info = get_c_backend_status()
    return jsonify(info)


@app.route("/api/run-experiment", methods=["POST"])
def api_run_experiment():
    """
    Execute simulation experiment by delegating to C backend via subprocess.

    Payload:
    {
        "method": "sequence" | "window" | "compare",
        "message": "text string"
    }
    """
    if not BACKEND_BIN.exists():
        return jsonify({
            "status": "error",
            "message": f"C backend binary not found at {BACKEND_BIN}. Build it using 'make' in backend/ directory."
        }), 500

    req_data = request.get_json(silent=True) or {}
    method = req_data.get("method", "sequence").strip().lower()
    message = req_data.get("message", "HELLO NETWORK").strip()

    valid_methods = {"sequence", "window", "compare", "comparison"}
    if method not in valid_methods:
        if method == "timing":
            return jsonify({
                "status": "error",
                "message": "Timing-based signaling has been permanently removed from the project scope. Supported methods: sequence, window."
            }), 400
        return jsonify({
            "status": "error",
            "message": f"Invalid signaling method '{method}'. Supported: sequence, window, compare"
        }), 400

    if method == "comparison":
        method = "compare"

    if not message:
        return jsonify({
            "status": "error",
            "message": "Input message cannot be empty."
        }), 400

    # Parse Network Impairment parameters (supporting both short and full names)
    try:
        loss_val = req_data.get("loss", req_data.get("loss_percent", 0))
        corrupt_val = req_data.get("corrupt", req_data.get("corruption_percent", 0))
        reorder_val = req_data.get("reorder", req_data.get("reorder_percent", 0))
        random_seed = req_data.get("seed", req_data.get("random_seed", None))

        loss_percent = max(0, min(100, int(loss_val)))
        corruption_percent = max(0, min(100, int(corrupt_val)))
        reorder_percent = max(0, min(100, int(reorder_val)))
    except (ValueError, TypeError):
        return jsonify({
            "status": "error",
            "message": "Invalid network impairment parameter values. Must be numeric."
        }), 400

    try:
        cmd = [
            str(BACKEND_BIN),
            "-m", method,
            "-d", message,
            "--loss", str(loss_percent),
            "--corrupt", str(corruption_percent),
            "--reorder", str(reorder_percent),
            "--json"
        ]
        if random_seed is not None and str(random_seed).strip() != "":
            cmd.extend(["--seed", str(int(random_seed))])

        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=10
        )

        if proc.returncode != 0:
            return jsonify({
                "status": "error",
                "message": f"C backend failed (exit code {proc.returncode}): {proc.stderr}"
            }), 500

        result = json.loads(proc.stdout)
        cmd_str = f"backend_engine.exe -m {method} -d \"{message}\" --loss {loss_percent} --corrupt {corruption_percent} --reorder {reorder_percent}"
        if random_seed is not None and str(random_seed).strip() != "":
            cmd_str += f" --seed {random_seed}"
        result["subprocess_command"] = cmd_str + " --json"
        return jsonify(result)

    except subprocess.TimeoutExpired:
        return jsonify({
            "status": "error",
            "message": "C backend execution timed out (exceeded 10 seconds)."
        }), 504
    except json.JSONDecodeError as jde:
        return jsonify({
            "status": "error",
            "message": f"Failed to parse C backend JSON output: {str(jde)}",
            "raw_output": proc.stdout if 'proc' in locals() else ""
        }), 500
    except Exception as e:
        return jsonify({
            "status": "error",
            "message": f"Server error: {str(e)}"
        }), 500


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    print(f"Starting TCP Signaling Web Frontend on http://127.0.0.1:{port}")
    app.run(host="127.0.0.1", port=port, debug=True)
