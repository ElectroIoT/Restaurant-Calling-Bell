# Hardware

You lay the boards out yourself in **EasyEDA Pro**. Start with [`../docs/easyeda-guide.md`](../docs/easyeda-guide.md).

| Folder | Content |
|---|---|
| `remote/easyeda`, `receiver/easyeda` | netlist, per-pin nets, BOM with LCSC numbers, placement hints (generated from `tools/*_design.py`) |
| `remote/bom`, `receiver/bom`, `bom/` | BOM CSVs (1x, 10x, system) and `BOM-SUMMARY.md` |
| `remote/kicad`, `receiver/kicad` | **reference only**: KiCad 10 schematic and an auto-routed PCB. ERC and DRC were run (0 errors on the last runs, silkscreen warnings only). Not reviewed by a person, not fabricated. |
| `lib/BellParts.pretty` | KiCad footprints for the SI24R1 module, CR2450 holder and tact switch, from vendor drawings (polarity of the holder unknown) |
| `diagrams/` | Mermaid block, sequence and state diagrams, `wiring.md` pin maps |
| `tools/` | generators: schematic/PCB for KiCad (adapted from the NewYearPixelFireworks scripts), `gen_bom.py`, `gen_easyeda_pack.py`; Freerouting jar is not committed |

Single source of truth: `tools/remote_design.py` and `tools/receiver_design.py`.
