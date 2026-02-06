"""
Entry point for running mudbot as a module.

Usage:
    python -m mudbot run --name TestBot --password secret
    python -m mudbot load --count 5 --prefix LoadBot --password secret
"""

import sys
from .commander.cli import main

if __name__ == '__main__':
    sys.exit(main())
