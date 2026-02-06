"""
Logging configuration for MUD bot.
"""

import logging
import sys
from typing import Optional


def setup_logging(
    level: int = logging.INFO,
    name: Optional[str] = None,
    verbose: bool = False
) -> logging.Logger:
    """
    Set up logging for the bot.

    Args:
        level: Logging level (default INFO).
        name: Logger name (default 'mudbot').
        verbose: If True, set DEBUG level and include more details.

    Returns:
        Configured logger.
    """
    if verbose:
        level = logging.DEBUG

    logger_name = name or "mudbot"
    logger = logging.getLogger(logger_name)

    # Avoid adding handlers multiple times
    if logger.handlers:
        return logger

    logger.setLevel(level)

    # Console handler
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(level)

    # Format: timestamp - level - name - message
    if verbose:
        fmt = "%(asctime)s.%(msecs)03d [%(levelname)s] %(name)s: %(message)s"
    else:
        fmt = "%(asctime)s [%(levelname)s] %(message)s"

    formatter = logging.Formatter(fmt, datefmt="%H:%M:%S")
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    return logger


def get_logger(name: str) -> logging.Logger:
    """Get a child logger under the mudbot namespace."""
    return logging.getLogger(f"mudbot.{name}")
