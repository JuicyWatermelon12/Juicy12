"""BanditBaller tuning tools for Unreal MCP.

Thin wrappers over the C++ UGSDebugLibrary (the same code behind the gs.* console commands), so
agents can set up and inspect fights in a running PIE session. All tools need PIE to be running.
"""

import json
import os

import unreal

import toolset_registry


def _project_dir() -> str:
    return os.path.normcase(os.path.abspath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())))


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
            type: Enemy data asset name, e.g. "BeatCop", "PatrolOfficer" or "DA_Enemy_BeatCop".
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
    def export_asset_fbx(asset_path: str, file_path: str) -> str:
        """Exports a mesh asset (SkeletalMesh with its skeleton, or StaticMesh) to an FBX file, LOD 0 only.

        Editor tool for the Blender placeholder pipeline (docs/PROGRESS.md, Prompt 56): Unreal MCP can import meshes
        but has no exporter. Doesn't need PIE.

        Args:
            asset_path: The asset, e.g. "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple".
            file_path: Absolute .fbx path inside the project folder (e.g. <project>/Art/Export/X.fbx).

        Returns:
            The written file path and its size in bytes, as text.
        """
        target = os.path.normcase(os.path.abspath(file_path))
        if not target.startswith(_project_dir()) or not target.endswith('.fbx'):
            raise ValueError('file_path must be an .fbx file inside the project folder')
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if asset is None:
            raise ValueError(f'No asset at {asset_path}')
        os.makedirs(os.path.dirname(os.path.abspath(file_path)), exist_ok=True)  # not `target`: normcase lowercases
        options = unreal.FbxExportOption()
        options.level_of_detail = False
        options.collision = False
        options.export_morph_targets = False
        options.vertex_color = True
        task = unreal.AssetExportTask()
        task.object = asset
        task.filename = os.path.abspath(file_path)
        task.automated = True
        task.prompt = False
        task.replace_identical = True
        task.options = options
        if not unreal.Exporter.run_asset_export_task(task) or not os.path.exists(task.filename):
            raise RuntimeError(f'Export of {asset_path} failed: {list(task.errors)}')
        return f'{task.filename} ({os.path.getsize(task.filename)} bytes)'

    @toolset_registry.tool_call
    @staticmethod
    def describe_ref_pose(mesh_path: str, bones: str = "", max_bones: int = 0) -> str:
        """Lists a skeletal mesh's own reference pose per bone: local translation / rotation / scale and the
        component-space location (UGSDebugLibrary::DescribeRefPose). Doesn't need PIE.

        Args:
            mesh_path: e.g. "/Game/Art/Placeholders/Characters/SK_LowPolyBody".
            bones: Comma-separated bone names to list (empty = all, or the first max_bones).
            max_bones: Limit when bones is empty (0 = all).

        Returns:
            One line per bone, as text.
        """
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if mesh is None:
            raise ValueError(f'No asset at {mesh_path}')
        names = [unreal.Name(b.strip()) for b in bones.split(',') if b.strip()]
        return unreal.GSDebugLibrary.describe_ref_pose(mesh, names, max_bones)

    @toolset_registry.tool_call
    @staticmethod
    def console_command(command: str) -> str:
        """Runs one console command (e.g. "gs.God 1", "gs.Perf 5", "shot showui") in the PIE world if PIE is
        running, otherwise in the editor world. Faster than typing into the Cmd box through SlateInspectorToolset.

        Args:
            command: The console command line.

        Returns:
            Which world it ran in.
        """
        subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        world = subsystem.get_game_world() or subsystem.get_editor_world()
        player = unreal.GameplayStatics.get_player_controller(world, 0) if subsystem.get_game_world() else None
        unreal.SystemLibrary.execute_console_command(world, command, player)
        return f'ran "{command}" in {"PIE" if subsystem.get_game_world() else "the editor world"}'

    @toolset_registry.tool_call
    @staticmethod
    def import_mesh_file(file_path: str, destination_path: str, asset_name: str) -> list[str]:
        """Imports a mesh file (.glb, .gltf, .fbx, .obj) with Unreal's default importer (Interchange in UE 5.8),
        unlike StaticMeshTools.import_file, which only takes FBX / OBJ through the legacy FbxFactory.

        Editor tool for the Blender placeholder pipeline (Prompt 56: the FBX vs glTF comparison). Doesn't need PIE.

        Args:
            file_path: Absolute path of the source file.
            destination_path: Content folder, e.g. "/Game/Art/Placeholders/_ImportTest".
            asset_name: Name for the imported asset.

        Returns:
            The object paths of every asset the import created.
        """
        if not os.path.exists(file_path):
            raise ValueError(f'No file at {file_path}')
        task = unreal.AssetImportTask()
        task.filename = file_path
        task.destination_path = destination_path
        task.destination_name = asset_name
        task.automated = True
        task.replace_existing = True
        task.save = False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        paths = [str(p) for p in task.imported_object_paths]
        if not paths:
            raise RuntimeError(f'Nothing imported from {file_path}; see the output log.')
        return paths

    @toolset_registry.tool_call
    @staticmethod
    def simulate_upgrade_offers(count: int = 10000, seed: int = 1, owned: str = "") -> str:
        """Rolls level-up upgrade offers from DA_UpgradePool and reports how often each upgrade, category and rarity appears.

        Uses the same roll as the game (UGSUpgradePool::RollOffer: per card the Legendary ability chance, else an
        upgrade by AppearanceWeight and a rarity from its category's weights; 3 different upgrades per offer) with its
        own random stream, as for a run with no picks (or with the `owned` upgrades, one pick each). It never advances
        the run stream and doesn't need PIE.

        Args:
            count: Offers to roll (1-1,000,000).
            seed: Seed of the private random stream (same seed -> same result).
            owned: Comma-separated upgrades counted as owned, one pick each ("FloatingGun,Dynamite,Molotov": the
                weapon slots are full, so only those weapons can be offered). Empty = nothing owned.

        Returns:
            A JSON object (as text; the toolset registry needs typed containers) with pool, seed, offers,
            choices_per_offer, total_choices, short_offers (offers with fewer choices than choices_per_offer),
            upgrades (name -> cards, percent of cards, percent of offers), categories (name -> cards, percent of cards,
            offers with one, percent of offers, and the rarity split of its cards in percent), and rarities (name ->
            choices of that rarity, percent of all choices, offers with at least one, percent of offers).
        """
        if not 1 <= count <= 1_000_000:
            raise ValueError('count must be between 1 and 1,000,000')
        owned_names = [name.strip() for name in owned.split(",") if name.strip()]
        result = unreal.GSDebugLibrary.simulate_upgrade_offers_owning(count, seed, owned_names)
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
            'owned': [str(name) for name in result.owned],
            'offers': int(result.offers),
            'choices_per_offer': int(result.choices_per_offer),
            'total_choices': int(result.total_choices),
            'short_offers': int(result.short_offers),
            'upgrades': upgrades,
            'categories': categories,
            'rarities': rarities,
        }, indent=1)
