"""Offline interactive 3D review of authored meshes, not a game screenshot."""
import argparse, base64, json
from pathlib import Path

def make_review(root: Path, output: Path):
    if output.exists():
        raise ValueError('Refusing to overwrite review')
    models = [json.loads((root/'source'/f'{name}.mesh.json').read_text(encoding='utf-8')) for name in ['ridge_sword', 'ridge_scabbard', 'songshan_gate_study']]
    template = Path(__file__).with_name('review.html').read_text(encoding='utf-8')
    data = json.dumps(models, separators=(',', ':')).replace('<', '\\u003c')
    audio = 'data:audio/wav;base64,' + base64.b64encode((root/'runtime/audio/mountain_motif.wav').read_bytes()).decode()
    output.write_text(template.replace('__DATA__', data).replace('__AUDIO__', audio), encoding='utf-8')

if __name__ == '__main__':
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('root', type=Path)
    cli.add_argument('output', type=Path)
    args = cli.parse_args()
    make_review(args.root, args.output)
