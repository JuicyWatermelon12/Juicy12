"""BanditBaller tuning tools for Unreal MCP.

Thin wrappers over the C++ UGSDebugLibrary (the same code behind the gs.* console commands), so
agents can set up and inspect fights in a running PIE session. All tools need PIE to be running.
"""

import unreal

import toolset_registry


def _game_world() -> unreal.World:
    """The running PIE world, or raise if the game isn't running."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if world is None:
        raise RuntimeError('PIE is not running. Start a play session first (EditorAppToolset.StartPIE).')
    return world


@unreal.uclass()
class BanditBallerTools(unreal.ToolsetDefinition):
    """BanditBaller gameplay tuning tools: spawn enemies around the player and read live player /
    enemy state from the running PIE session. Server-authoritative (acts on the PIE server world)."""

    @toolset_registry.tool_call
    @staticmethod
    def spawn_enemies(type: str, count: int = 1) -> list[str]:
        """Spawns enemies around the first player using the round spawn rules (UGSSpawnLocationFinder):
        a ring from DA_SpawnSettings (12-25 m), on the town NavMesh, reachable, spaced, preferably off-screen,
        with an EnemySpawn Target Point fallback. Refused while the party is in the Market District.

        Args:
            type: Enemy data asset name, e.g. "MeleeBandit", "RangedBandit" or "DA_Enemy_MeleeBandit".
            count: How many to spawn (1-200).

        Returns:
            Names of the spawned enemy actors.
        """
        world = _game_world()
        if not 1 <= count <= 200:
            raise ValueError('count must be between 1 and 200')
        spawned = unreal.GSDebugLibrary.spawn_enemies(world, type, count)
        if not spawned:
            raise ValueError(f'Nothing spawned: unknown enemy type "{type}", no player, or the party is in the Market District. See the output log.')
        return [enemy.get_name() for enemy in spawned]

    @toolset_registry.tool_call
    @staticmethod
    def get_player_attributes() -> dict[str, float]:
        """Returns the first player's current attribute values.

        Includes the GAS attributes (Health, MaxHealth, MoveSpeed, Damage, FireRate, IncomingDamage)
        plus Ammo (equipped magazine), God, Invulnerable and Dead as 0/1 flags.

        Returns:
            Attribute name to current value.
        """
        attributes = unreal.GSDebugLibrary.get_player_attributes(_game_world())
        if not attributes:
            raise RuntimeError('No player with an ability system found in the PIE world.')
        return {str(name): float(value) for name, value in attributes.items()}

    @toolset_registry.tool_call
    @staticmethod
    def list_enemies_alive() -> list[unreal.GSEnemyDebugInfo]:
        """Lists every living enemy, nearest to the player first.

        Returns:
            One entry per enemy: name, enemy_type (data asset), health, max_health, location,
            distance_to_player (cm) and attacking (State.Attacking).
        """
        return list(unreal.GSDebugLibrary.list_enemies_alive(_game_world()))
