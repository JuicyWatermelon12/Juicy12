"""Runs at editor startup (PythonScriptPlugin): registers the BanditBaller MCP toolsets."""

import unreal

try:
    import banditballer_toolset

    if banditballer_toolset._registration.register():
        unreal.log('BanditBaller: registered MCP toolset banditballer_toolset.toolsets.BanditBallerTools')
    else:
        unreal.log_warning('BanditBaller: toolset registry unavailable; MCP toolset not registered')
except Exception as error:  # Never break editor startup over a debug toolset.
    unreal.log_error(f'BanditBaller: failed to register MCP toolset: {error}')
