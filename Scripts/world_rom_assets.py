"""Reconstruct VARIA level edits from the player's original room data.

The public data contains tile edits and compression commands, never the original
room geometry or literal compressed level payload. Commands only describe the
format; all literal/fill bytes are read from the ROM-derived edited level.
"""
import ast
import bisect
import hashlib
import json


def pc(address):
    return ((address >> 16) & 127) * 32768 + (address & 32767)


def snes(address):
    return ((address // 32768) | 128) << 16 | (address & 32767) | 32768


def decode(source, address):
    output = bytearray()
    commands = bytearray()
    at = address
    while True:
        first = source[at]
        at += 1
        commands.append(first)
        if first == 255:
            break
        mode, length = first >> 5, (first & 31) + 1
        if mode == 7:
            mode, length = (first >> 2) & 7, ((first & 3) << 8 | source[at]) + 1
            commands.append(source[at])
            at += 1
        if mode == 0:
            output += source[at:at + length]
            at += length
        elif mode == 1:
            output += bytes([source[at]]) * length
            at += 1
        elif mode == 2:
            output += (source[at:at + 2] * ((length + 1) // 2))[:length]
            at += 2
        elif mode == 3:
            output += bytes((source[at] + i) & 255 for i in range(length))
            at += 1
        else:
            count = 1 if mode >= 6 else 2
            ref = int.from_bytes(source[at:at + count], 'little')
            commands += source[at:at + count]
            at += count
            position = len(output) - ref if mode >= 6 else ref
            for i in range(length):
                assert 0 <= position + i < len(output)
                output.append(output[position + i] ^ (255 if mode & 1 else 0))
        assert len(output) <= 65536 and at - address <= 32768
    return output, commands, at


def encode(data, commands):
    output = bytearray()
    at = position = 0
    while True:
        first = commands[at]
        at += 1
        output.append(first)
        if first == 255:
            break
        mode, length = first >> 5, (first & 31) + 1
        if mode == 7:
            mode, length = (first >> 2) & 7, ((first & 3) << 8 | commands[at]) + 1
            output.append(commands[at])
            at += 1
        if mode == 0:
            output += data[position:position + length]
        elif mode in (1, 3):
            output += data[position:position + 1]
        elif mode == 2:
            output += data[position:position + 2]
        else:
            count = 1 if mode >= 6 else 2
            output += commands[at:at + count]
            at += count
        position += length
    assert position == len(data) and at == len(commands)
    return output


def build(root, metadata, patch_ranges, spans, payload, audit, domain="world"):
    rom = (root / 'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
    assert hashlib.sha1(rom).hexdigest() == 'da957f0d63d14cb441d215462904c4fa8519c613'
    tree = ast.parse((root / 'Randomizer/upstream/tools/rooms.py').read_text())
    rooms = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                 and any(isinstance(t, ast.Name) and t.id == 'rooms' for t in n.targets))
    word = lambda at: int.from_bytes(rom[at:at + 2], 'little')
    levels = {}
    for room in rooms:
        at = room['Address'] + 11
        for _ in range(20):
            kind = word(at)
            state = at + 2 if kind == 0xe5e6 else 0x70000 + word(at + (3 if kind in (0xe612, 0xe629) else 2))
            address = pc(int.from_bytes(rom[state:state + 3], 'little'))
            levels.setdefault(address, []).append(room['Name'])
            if kind == 0xe5e6:
                break
            at += 5 if kind in (0xe612, 0xe629) else 4
        else:
            raise ValueError('Unterminated room-state list')
    keys = sorted(levels)
    edits, commands, programs, copies, literals, small = [], [], [], [], [], []
    reconstructed = bytearray(len(payload))
    records = []
    for patch in metadata:
        first, count = patch_ranges[patch['id']]
        selected = spans[first:first + count]
        shadow = bytearray(rom)
        for address, offset, size in selected:
            shadow[address:address + size] = bytes(payload[offset:offset + size])
        starts = sorted({keys[bisect.bisect_right(keys, a) - 1] for a, _, _ in selected if a >= 0x200000})
        by_start = {}
        for start in starts:
            original, _, _ = decode(rom, start)
            modified, template, end = decode(shadow, start)
            # VARIA's compressor sometimes leaves one unused trailing byte.
            # Retain it explicitly, without changing the native level contract.
            original = (original + bytes(max(0, len(modified) - len(original))))[:len(modified)]
            changed = [(i, v) for i, v in enumerate(modified) if v != original[i]]
            for at, value in changed:
                original[at] = value
            packed = encode(original, template)
            assert packed == shadow[start:end]
            program = len(programs)
            programs.append((snes(start), len(modified), len(edits), len(changed), len(commands)))
            edits.extend(changed)
            commands.extend(template)
            by_start[start] = (program, packed)
            records.append(dict(patch=patch['name'], sourceRoomAddress=snes(start), rooms=levels[start],
                                changedBytes=len(changed), compressionTemplateBytes=len(template),
                                reconstructedSha256=hashlib.sha256(packed).hexdigest()))
        for address, offset, size in selected:
            wanted = bytes(payload[offset:offset + size])
            if address < 0x200000:
                # Reviewed fields: load/save records, room/door/PLM definitions,
                # enemy populations, and VARIA's own indicator PLM scripts.
                small.append((offset, len(literals), size))
                literals.extend(wanted)
                reconstructed[offset:offset + size] = wanted
            else:
                start = keys[bisect.bisect_right(keys, address) - 1]
                program, packed = by_start[start]
                source = address - start
                copied = min(size, max(0, len(packed) - source))
                assert wanted[copied:] == b'\xff' * (size - copied)
                copies.append((program, source, offset, copied, size))
                reconstructed[offset:offset + size] = packed[source:source + copied] + b'\xff' * (size - copied)
    assert reconstructed == bytes(payload)
    lines = ['/* ROM-derived room geometry + reviewed VARIA edits. No literal level payload. */']
    def array(declaration, rows):
        lines.append('static const ' + declaration + '[]={')
        lines.extend(rows)
        lines.append('};')
    def numbers(values):
        return [','.join(str(v) for v in values[i:i + 24]) + ',' for i in range(0, len(values), 24)]
    array('uint8_t world_level_commands', numbers(commands))
    array('uint8_t world_record_bytes', numbers(literals))
    array('struct {uint16_t offset;uint8_t value;} world_level_edits', ['{%d,%d},' % e for e in edits])
    array('struct {uint32_t source,size,edit_first,edit_count,command_first;} world_level_programs',
          ['{' + ','.join(map(str, p)) + '},' for p in programs])
    array('struct {uint32_t program,source,destination,count,total;} world_level_copies',
          ['{' + ','.join(map(str, p)) + '},' for p in copies])
    array('struct {uint32_t destination,source,size;} world_record_copies',
          ['{' + ','.join(map(str, p)) + '},' for p in small])
    generated = '\n'.join(lines) + '\n'
    (root / f'Native/sm_{domain}_rom_assets.inc').write_text(generated)
    proof = dict(schema=1, originalRomSha1=hashlib.sha1(rom).hexdigest(),
                 payloadSha256=hashlib.sha256(reconstructed).hexdigest(),
                 recipeSha256=hashlib.sha256(generated.encode()).hexdigest(),
                 sourceLevels=len(programs), changedLevelBytes=len(edits),
                 authoredRecordBytes=len(literals), reconstructedBytes=len(reconstructed),
                 byteExactReconstruction=True, levels=records)
    (root / f'Docs/Releases/ALPHA-0.24/{domain}-rom-reconstruction.json').write_text(json.dumps(proof, indent=2) + '\n')


def load_dependencies(root):
    """Hydrate reviewed source bytes from pinned upstream, never published docs."""
    import sys
    audit=json.loads((root/'Docs/Randomizer/FullOptions/Starts/dependencies.json').read_text())
    upstream=root/'Randomizer/upstream'
    sys.path.insert(0,str(upstream))
    from logic.logic import Logic
    Logic.factory('vanilla')
    from rom.flavor import RomFlavor
    RomFlavor.factory(str(upstream))
    from rom.ips import IPS_Patch
    for name,patch in audit['patches'].items():
        if patch['source']=='dictionary':
            data=RomFlavor.patchAccess.getDictPatches()[name]
        else:
            source=(upstream/patch['source']).resolve()
            if not source.is_relative_to(upstream.resolve()):raise ValueError('Invalid audited source')
            data=IPS_Patch.load(str(source)).toDict()
        for span in patch['spans']:
            values=bytes(data[span['address']])
            if len(values)!=span['size'] or hashlib.sha256(values).hexdigest()!=span['sha256']:
                raise ValueError('Audited upstream patch changed: '+name)
            span['data']=list(values)
    return audit
