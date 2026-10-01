"""
Pytest configuration and environment bootstrap.
Ensures project root and archived legacy modules are reachable.
"""

import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
ARCHIVE_APP_DIR = PROJECT_ROOT / "archive" / "legacy_python_backend"

for p in (PROJECT_ROOT, ARCHIVE_APP_DIR):
    if str(p) not in sys.path:
        sys.path.insert(0, str(p))
