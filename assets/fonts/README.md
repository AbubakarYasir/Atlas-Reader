# Bundled fonts

| File | Family | Built from | SHA-256 | Use |
|---|---|---|---|---|
| `AtlasNaskh-Regular.ttf` | Atlas Naskh (400) | Noto Naskh Arabic Regular + Latin from Noto Sans Regular | `7511b0df2569cebae105d504bf1489f5a7d086f3db1cfb44979b9aa070fdb4d9` | Arabic interface and Arabic content |
| `AtlasNaskh-SemiBold.ttf` | Atlas Naskh (600) | Noto Naskh Arabic SemiBold + Latin from Noto Sans SemiBold | `823d90ddfb75baa5e4aab041ab5807348c4cb608f659d56d7d8cadc63793d327` | Arabic headings |
| `AtlasNastaliq-Regular.ttf` | Atlas Nastaliq (400) | Noto Nastaliq Urdu at weight 400 | `c4d0284dcb09a35fcd956c3b5a35efdfe78cce773245c2e88de74c9e8606452a` | Urdu interface and Urdu content |
| `AtlasNastaliq-SemiBold.ttf` | Atlas Nastaliq (600) | Noto Nastaliq Urdu at weight 600 | `b59f3f45c604c66d0f0602948d2d226d3d9bee7745569900b6e512ac863cd484` | Urdu headings |

All four also map the Unicode direction and joining controls (U+200B–U+200F, U+202A–U+202E, U+2060, U+2066–U+2069, U+FEFF) to one empty zero-width glyph.

## Why the fonts are derived

On Windows, Qt reads every installed font family the first time a text needs a character its font lacks, or asks for a weight the family does not have. On a PC with thousands of fonts that takes seconds and happened before the first frame. The interface fonts therefore carry every character the interface shows and both weights it uses. `tools/fonts/check-ui-coverage.py` (CI) fails if an interface string contains a character its font lacks.

The families are renamed because a PC may have the original Noto fonts installed; two different fonts under one family name would be ambiguous. Arabic and Urdu letter shapes and positions are unchanged: shaping the same text with the source and the derived font gives the same glyphs and advances.

## Sources (pinned)

| Input | Pin | SHA-256 |
|---|---|---|
| `NotoNaskhArabic-Regular.ttf` | notofonts/arabic release `NotoNaskhArabic-v2.021`, `hinted/ttf/` | `6f0a92031367b2f5a2078fe9d24f3433122b61a0bad57c423aad8f3c39aa2e6e` |
| `NotoNaskhArabic-SemiBold.ttf` | same release | `76616e51f5ff1cf4d81a14df6049646911cd4d7e568903a011711c241991624c` |
| `NotoNastaliqUrdu-wght.ttf` | google/fonts commit `23e54b51ddffbc7713c583748e3bd86f62b1fa4a`, `ofl/notonastaliqurdu/NotoNastaliqUrdu[wght].ttf` | `98a4787f34eb6fde57fb9a1121a8f216301196ab0da98ea110cad581be8abbcd` |
| `NotoSans-Regular.ttf` | notofonts/notofonts.github.io commit `739604e080c772760ae37106fa0013805251eb49`, `fonts/NotoSans/hinted/ttf/` (version 2.015) | `478c558ea716033cd60c03438f628dfa75694dcf6b5f6d505a2f05fd2b4f3823` |
| `NotoSans-SemiBold.ttf` | same commit | `a4e91fd530ac2b4ef5367240144ff37d7d65d66cf76f2e9a2187b93c676f92d0` |

## Rebuilding

```text
python tools/fonts/build-ui-fonts.py --sources <folder with the five inputs> --out assets/fonts
```

The script verifies every input by SHA-256 and is deterministic: the same inputs and fontTools version (4.67.0) give byte-identical files. The N2 PDF evidence downloads its own unmodified Noto files and is not affected.

## Licence

The sources and the derived fonts are licensed under the SIL Open Font License 1.1 ([OFL.txt](OFL.txt)), which allows modification, bundling and redistribution. None of the sources declares a Reserved Font Name. The derived fonts keep the original copyright and licence records and state the modification in their description record.
