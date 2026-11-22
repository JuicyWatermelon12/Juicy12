"""BanditBaller tuning tools for Unreal MCP.

Thin wrappers over the C++ UGSDebugLibrary (the same code behind the gs.* console commands), so
agents can set up and inspect fights in a running PIE session. All tools need PIE to be running.
"""

import json

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

        Includes the GAS attributes (Health, MaxHealth, MoveSpeed, Damage, FireRate, IncomingDamage, ...)
        plus Ammo (equipped magazine), Cash (profile), Level, XP, XPForNextLevel, TotalXP, the kill combo
        (Combo, ComboMultiplier, ComboTimeLeft in seconds), and God, Invulnerable and Dead as 0/1 flags.

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

    @toolset_registry.tool_call
    @staticmethod
    def simulate_upgrade_offers(count: int = 10000, seed: int = 1) -> str:
        """Rolls level-up upgrade offers from DA_UpgradePool and reports how often each upgrade, category and rarity appears.

        Uses the same roll as the game (UGSUpgradePool::RollOffer: per card the Legendary ability chance, else an
        upgrade by AppearanceWeight and a rarity from its category's weights; 3 different upgrades per offer) with its
        own random stream, as for a fresh run with no picks. It never advances the run stream and doesn't need PIE.

        Args:
            count: Offers to roll (1-1,000,000).
            seed: Seed of the private random stream (same seed -> same result).

        Returns:
            A JSON object (as text; the toolset registry needs typed containers) with pool, seed, offers,
            choices_per_offer, total_choices, short_offers (offers with fewer choices than choices_per_offer),
            upgrades (name -> cards, percent of cards, percent of offers), categories (name -> cards, percent of cards,
            offers with one, percent of offers, and the rarity split of its cards in percent), and rarities (name ->
            choices of that rarity, percent of all choices, offers with at least one, percent of offers).
        """
        if not 1 <= count <= 1_000_000:
            raise ValueError('count must be between 1 and 1,000,000')
        result = unreal.GSDebugLibrary.simulate_upgrade_offers(count, seed)
        if not result.pool:
            raise RuntimeError('No UGSUpgradePool asset (DA_UpgradePool) found.')
        offers = max(result.offers, 1)
        choices = max(result.total_choices, 1)
        upgrades = {
            str(name): {
                'cards': int(result.upgrade_choices.get(name, 0)),
                'percent_of_cards': round(100.0 * result.upgrade_choices.get(name, 0) / choices, 2),
                'offers': int(n),
                'percent_of_offers': round(100.0 * n / offers, 2),
            }
            for name, n in sorted(result.upgrade_offers.items(), key=lambda item: -item[1])
        }
        categories = {}
        for name, n in result.category_choices.items():
            with_category = int(result.category_offers.get(name, 0))
            split = {}
            for key, cards in result.category_rarity_choices.items():
                category, _, rarity = str(key).partition(' / ')
                if category == str(name):
                    split[rarity] = round(100.0 * cards / max(n, 1), 2)
            categories[str(name)] = {
                'cards': int(n),
                'percent_of_cards': round(100.0 * n / choices, 2),
                'offers_with': with_category,
                'percent_of_offers': round(100.0 * with_category / offers, 2),
                'rarity_percent_of_its_cards': split,
            }
        rarities = {}
        for name, n in result.rarity_choices.items():
            with_rarity = int(result.rarity_offers.get(name, 0))
            rarities[str(name)] = {
                'choices': int(n),
                'percent_of_choices': round(100.0 * n / choices, 2),
                'offers_with': with_rarity,
                'percent_of_offers': round(100.0 * with_rarity / offers, 2),
            }
        return json.dumps({
            'pool': str(result.pool),
            'seed': int(result.seed),
            'offers': int(result.offers),
            'choices_per_offer': int(result.choices_per_offer),
            'total_choices': int(result.total_choices),
            'short_offers': int(result.short_offers),
            'upgrades': upgrades,
            'categories': categories,
            'rarities': rarities,
        }, indent=1)
