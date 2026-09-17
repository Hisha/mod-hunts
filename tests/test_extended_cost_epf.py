#!/usr/bin/env python3
"""Read-only EPF contract check. Does not connect to a server or build/apply content."""
import argparse
import json
from pathlib import Path
import zipfile


def check(epf, previous=None):
    with zipfile.ZipFile(epf) as archive:
        assert archive.testzip() is None
        assert len(archive.namelist()) == len(set(archive.namelist()))
        manifest = json.loads(archive.read('manifest.json'))
        assert manifest['schema'] == 2 and manifest['package'] == 'mod-hunts'
        assert manifest['version'] == '4.2.0'
        expected = [{'symbol': 'seal-cost-5', 'requirements': [
            {'item': {'symbol': 'seal'}, 'count': 5}]}]
        assert manifest['extendedCosts'] == expected
        assert type(manifest['extendedCosts'][0]['requirements'][0]['count']) is int
        assert len(manifest['dbcRows']) == len(manifest['serverRows']) == 1
        assert manifest['dbcRows'][0]['symbol'] == manifest['serverRows'][0]['symbol'] == 'seal'
        assert manifest['currencies'] == [{'symbol': 'seal-currency', 'item': 'seal', 'category': {'symbol': 'hunts'}}]
        assert manifest['currencyCategories'] == [{'symbol': 'hunts', 'name': {'enUS': 'Hunts'}}]
        assert manifest['content'] == []  # No raw DBC or vendor SQL payload.
        assert 'ID' not in manifest['dbcRows'][0]['fields']
        assert 'entry' not in manifest['serverRows'][0]['fields']
        assert '56807' not in json.dumps(manifest)
        if previous:
            with zipfile.ZipFile(previous) as old:
                before = json.loads(old.read('manifest.json'))
                assert before['version'] == '4.1.0' and 'extendedCosts' not in before
                restored = dict(manifest)
                restored['version'] = '4.1.0'
                del restored['extendedCosts']
                assert restored == before, 'Unrelated manifest content changed'
                assert archive.namelist() == old.namelist(), 'Archive entries changed'
                for name in old.namelist():
                    if name != 'manifest.json':
                        assert archive.read(name) == old.read(name)
    print('PASS EPF 4.2.0: one seal-cost-5, local seal x5, no authored item/cost IDs; existing declarations preserved')
    if previous:
        print('PASS 4.1.0 comparison: only version and extendedCosts changed; other payload bytes identical')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--epf', type=Path, default=Path(__file__).resolve().parents[1] / 'content/mod-hunts.epf')
    parser.add_argument('--previous-epf', type=Path)
    args = parser.parse_args()
    check(args.epf, args.previous_epf)
