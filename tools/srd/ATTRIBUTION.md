# Attribution

This directory's data (`srd_data.py`) and invariants (`INVARIANTS.md`) include
material from the Dungeons & Dragons System Reference Documents, used under
the Creative Commons Attribution 4.0 International License, and from the
5e-quint formal specification project.

## SRD 5.2.1 (required attribution statement)

> This work includes material from the System Reference Document 5.2.1
> ("SRD 5.2.1") by Wizards of the Coast LLC, available at
> https://www.dndbeyond.com/srd. The SRD 5.2.1 is licensed under the Creative
> Commons Attribution 4.0 International License, available at
> https://creativecommons.org/licenses/by/4.0/legalcode.

Used for: the weapons table (including the 5.2.1 trident), the Ensnaring
Strike spell, cross-checking of all spell and class mechanics, and the rules
text behind most invariants.

## SRD 5.1 (required attribution statement)

> This work includes material taken from the System Reference Document 5.1
> ("SRD 5.1") by Wizards of the Coast LLC and available at
> https://dnd.wizards.com/resources/systems-reference-document. The SRD 5.1
> is licensed under the Creative Commons Attribution 4.0 International
> License available at https://creativecommons.org/licenses/by/4.0/legalcode.

Used for: class tables and features (levels 1-3), the baseline spell
mechanics (this game uses the classic 2014-rules versions of sleep, vicious
mockery, healing word, cure wounds, and hunter's mark), the imp and boar stat
blocks, and the 14 conditions.

SRD 5.1 JSON was retrieved via the D&D 5e API (https://www.dnd5eapi.co, the
5e-bits project; API code MIT licensed, content SRD 5.1 under CC-BY-4.0 as
above).

## 5e-quint

> D&D 5e Quint Specification, Copyright 2025 Igor Loskutov.
> https://github.com/dearlordylord/5e-quint
> Code licensed under the Apache License, Version 2.0. The project formalizes
> mechanics from the System Reference Document 5.2.1, Copyright Wizards of
> the Coast LLC, available under the Creative Commons Attribution 4.0
> International License (CC BY 4.0).

Used for: the safety invariants in `INVARIANTS.md` (its machine-checked
inductive invariants, rule-core definitions, `ASSUMPTIONS.md` rulings, and
`battle/REQUIREMENTS.md` reaction/turn catalog) and the SRD 5.2.1 weapon and
spell records used for cross-checking.

## Non-SRD monster statistics

The intellect devourer, cambion, and mind flayer do not appear in SRD 5.1 or
SRD 5.2.1 and are therefore NOT covered by the Creative Commons license
above. Their entries in `srd_data.py` (marked `"srd": False`) are numeric
game statistics from the 2014 Dungeons & Dragons Monster Manual, with all
descriptive text rewritten as our own brief mechanical summaries. Game
mechanics and statistics are not themselves protectable expression, but these
entries carry no SRD provenance and no CC license; do not redistribute them
as SRD content.

## Trademarks

Dungeons & Dragons, D&D, their respective logos, and the monster names
identified as Product Identity are trademarks of Wizards of the Coast LLC.
This project is not affiliated with, endorsed, sponsored, or specifically
approved by Wizards of the Coast.
