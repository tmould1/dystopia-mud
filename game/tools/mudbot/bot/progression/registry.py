"""
Class plugin registry for mudbot.

Provides a decorator-based registration system for class progression bots.
"""

import logging
from typing import Dict, Type, List, TYPE_CHECKING

if TYPE_CHECKING:
    from .base import ClassProgressionBot
    from ...config import BotConfig

logger = logging.getLogger(__name__)


class ClassRegistry:
    """
    Registry for class progression bot plugins.

    Usage:
        @ClassRegistry.register("demon")
        class DemonProgressionBot(ClassProgressionBot):
            ...

        # Later:
        bot = ClassRegistry.create("demon", config)
    """

    _classes: Dict[str, Type["ClassProgressionBot"]] = {}

    @classmethod
    def register(cls, name: str):
        """
        Decorator to register a class progression bot.

        Args:
            name: The class name (e.g., "demon", "vampire").

        Returns:
            Decorator function.
        """
        def decorator(bot_class: Type["ClassProgressionBot"]):
            cls._classes[name.lower()] = bot_class
            logger.debug(f"Registered class progression bot: {name}")
            return bot_class
        return decorator

    @classmethod
    def create(cls, class_name: str, config: "BotConfig", **kwargs) -> "ClassProgressionBot":
        """
        Factory to create the appropriate bot type.

        Args:
            class_name: The class name (e.g., "demon").
            config: Bot configuration.
            **kwargs: Additional arguments passed to bot constructor.

        Returns:
            Instance of the appropriate ClassProgressionBot subclass.

        Raises:
            ValueError: If class_name is not registered.
        """
        key = class_name.lower()
        if key not in cls._classes:
            available = ", ".join(cls._classes.keys()) or "(none)"
            raise ValueError(f"Unknown class: {class_name}. Available: {available}")

        bot_class = cls._classes[key]
        logger.info(f"Creating {class_name} progression bot: {config.name}")
        return bot_class(config, **kwargs)

    @classmethod
    def available_classes(cls) -> List[str]:
        """
        Get list of available class names.

        Returns:
            List of registered class names.
        """
        return list(cls._classes.keys())

    @classmethod
    def is_registered(cls, name: str) -> bool:
        """
        Check if a class is registered.

        Args:
            name: Class name to check.

        Returns:
            True if registered.
        """
        return name.lower() in cls._classes

    @classmethod
    def get_class(cls, name: str) -> Type["ClassProgressionBot"]:
        """
        Get the bot class for a given class name.

        Args:
            name: Class name.

        Returns:
            The bot class type.

        Raises:
            ValueError: If not registered.
        """
        key = name.lower()
        if key not in cls._classes:
            raise ValueError(f"Unknown class: {name}")
        return cls._classes[key]
