"""BanditBaller project toolsets for Unreal MCP (registered by Content/Python/init_unreal.py)."""

from toolset_registry.registration import Registration

from banditballer_toolset import toolsets

_registration = Registration([
    toolsets.BanditBallerTools,
])
