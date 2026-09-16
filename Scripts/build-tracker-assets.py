#!/usr/bin/env python3
"""Compatibility entry point: trackers are generated from the player ROM."""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name('build-tracker-rom-assets.py')),run_name='__main__')
