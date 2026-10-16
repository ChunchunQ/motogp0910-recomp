#!/usr/bin/env python3
"""Run Windows CPU/UI regression probes in isolated folders, preserving user settings.

Build the EXCLUDE_FROM_ALL probe targets first. On Windows omit --wine;
Linux/Wine validation additionally needs a display and the native VC++ runtime.
This suite does not verify rendered Windows GPU pixels or a full career race.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

PROBES = {
    'localization_probe': 'localization', 'runtime_stability_probe': 'game', 'nightly_input_probe': '',
    'fsr_provider_probe': '', 'engine_settings_probe': 'game',
    'game_language_probe': 'game', 'physics_asset_probe': 'game',
    'button_prompt_probe': 'game', 'livery_readback_probe': 'gpu',
    'motogp_headless_probe': 'game', 'launcher_runtime_probe': '',
    'display_settings_probe': '', 'button_prompt_input_probe': '',
    'launcher_probe': 'game', 'launcher_layout_probe': 'ui',
    'achievement_ui_probe': 'achievements', 'quit_menu_probe': 'quit',
    'in_game_settings_probe': 'settings', 'adaptive_triggers_probe': '',
}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--game-root', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, default=Path('out/windows-validation'))
    parser.add_argument('--wine', type=Path)
    parser.add_argument('--skip-ui', action='store_true')
    args = parser.parse_args()
    build, game = args.build_dir.resolve(), args.game_root.resolve()
    for name in [*PROBES, 'input_defaults_probe']:
        if name == 'launcher_layout_probe' and args.skip_ui:
            continue
        if not (build / (name + '.exe')).is_file():
            parser.error(f'Build the missing probe target first: {name}')
    if not (game / 'default.xex').is_file():
        parser.error('Game root must contain default.xex and default.xexp')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix='run-', dir=args.output_dir.resolve()))
    results = []
    provenance = {file.name: hashlib.sha256(file.read_bytes()).hexdigest()
                  for file in (build / 'rexruntime.dll', build / 'rexgpu-xenos.dll',
                               build / 'amd_fidelityfx_dx12.dll')}
    (root / 'runtime-sha256.json').write_text(json.dumps(provenance, indent=2) + '\n')

    def target(path):
        path = str(path.resolve())
        return 'Z:' + path.replace('/', '\\') if args.wine else path

    def run(name, extra=None, config=None, environment=None, label=None):
        label = label or name
        fixture = root / label
        fixture.mkdir()
        for file in build.glob('*.dll'):
            shutil.copy2(file, fixture / file.name)
        shutil.copy2(build / (name + '.exe'), fixture / (name + '.exe'))
        # Launcher validation requires the real game EXE beside the probe.
        if name in ('launcher_probe', 'launcher_layout_probe'):
            shutil.copy2(build / 'motogp0910.exe', fixture / 'motogp0910.exe')
        if (build / 'resources').is_dir():
            shutil.copytree(build / 'resources', fixture / 'resources')
        if config is not None:
            (fixture / 'input-probe.toml').write_text(config, encoding='utf-8')
        command = ([str(args.wine.resolve())] if args.wine else []) + [target(fixture / (name + '.exe')), *(extra or [])]
        env = os.environ.copy()
        for key in ('REX_MNK_MODE', 'REXGLUE_MNK_MODE'):
            env.pop(key, None)
        env.update(environment or {})
        start = time.monotonic()
        with (root / (label + '.log')).open('w', encoding='utf-8') as log:
            try:
                result = subprocess.run(command, cwd=fixture, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=180)
                code = result.returncode
            except subprocess.TimeoutExpired:
                log.write('\nTimed out after 180 seconds\n')
                code = 124
        results.append({'probe': label, 'exit': code, 'seconds': round(time.monotonic() - start, 3)})
        print(f'{label}: {"PASS" if code == 0 else "FAIL"}', flush=True)
        (root / 'results.json').write_text(json.dumps(results, indent=2) + '\n')

    for name, kind in PROBES.items():
        if kind == 'ui' and args.skip_ui:
            continue
        extra = [target(game)] if kind in ('game', 'ui') else [target(build / 'rexgpu-xenos.dll')] if kind == 'gpu' else []
        if kind == 'localization':
            extra = [target(game), target(build / 'resources/localization/tr/overlay')]
        if kind == 'achievements':
            extra = [target(build / 'resources/ui/OpenSans-Semibold.ttf'), target(root / 'achievement-preview')]
        if kind == 'quit':
            extra = [target(game), target(build / 'resources/ui/OpenSans-Semibold.ttf')]
        if kind == 'settings':
            extra = [target(game), target(build / 'resources/ui/OpenSans-Semibold.ttf'), target(root / 'settings-preview')]
        if kind == 'ui':
            extra.append(target(root / 'screenshots'))
        run(name, extra)
    for scenario in ('default', 'config-false', 'env-false', 'cli-false', 'cli-true'):
        extra = [scenario]
        if scenario.startswith('cli-'):
            extra.append('--mnk_mode=' + ('true' if scenario == 'cli-true' else 'false'))
        run('input_defaults_probe', extra, config='mnk_mode=false\n' if scenario == 'config-false' else None,
            environment={'REX_MNK_MODE':'false', 'REXGLUE_MNK_MODE':'false'} if scenario in ('env-false', 'cli-true') else None,
            label='input-' + scenario)
    print('Evidence:', root)
    return 1 if any(item['exit'] for item in results) else 0

if __name__ == '__main__':
    raise SystemExit(main())
