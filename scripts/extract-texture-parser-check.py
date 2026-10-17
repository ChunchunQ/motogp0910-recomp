#!/usr/bin/env python3
"""Compile the SDK's exact CPU texture parser alone, without a fake GPU backend."""
import argparse
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
s=a.source.read_text();start='void TextureCache::BindingInfoFromFetchConstant(';end='\nvoid TextureCache::ResetTextureBindings('
assert s.count(start)==1 and s.count(end)==1,'Pinned SDK texture parser boundaries changed; review the extraction'
body=s[s.index(start):s.index(end)];assert body.rstrip().endswith('}')
prefix=s[:s.index('REXCVAR_DEFINE_')]
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.write_text(prefix+'\n// Exact function body extracted from the pinned patched SDK.\nnamespace rex::graphics {\n'+body+'\n}\n')
