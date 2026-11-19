"""Add a read-only dispatch-table observer to original-ROM probe scripts."""
from pathlib import Path
import json

HERE = Path(__file__).resolve().parent
INVENTORY = HERE.parents[1] / 'abi-audit/rom-inventory.json'


def instrument(script, neo, first=None):
    """Log table entries fetched after a fixture takes control of the CPU."""
    name = 'neo_3_21' if neo else 'as3000'
    rom = next(item for item in json.loads(INVENTORY.read_text()) if item['name'] == name)
    table = rom['base'] + rom['table_offset']
    prefix = (HERE / 'observe.lua.in').read_text()
    for key, value in {'FIRST': first if first is not None else (150 if neo else 540),
                       'TABLE': table, 'END': table + 4 * rom['slots'] - 1}.items():
        prefix = prefix.replace('@COVERAGE_' + key + '@', str(value))
    return prefix + '\n' + script
