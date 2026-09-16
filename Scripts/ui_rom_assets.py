"""Split VARIA's mixed UI sheets into ROM references and its authored glyphs.

Only replacement lettering, counters and objective symbols are retained. Original
HUD/menu shapes are decoded from the validated player's ROM, including palette
index remaps. Unused pause tiles keep the original ROM art, never a copied sheet.
"""
import hashlib
import json


def pc(address):
    return ((address >> 16) & 127) * 32768 + (address & 32767)


def decode(data, depth):
    return tuple(sum(((data[y*2+p//2*16+p%2] >> (7-x)) & 1) << p
                     for p in range(depth)) for y in range(8) for x in range(8))


def signature(pixels):
    colors = {}
    return bytes(colors.setdefault(v, len(colors)) for v in pixels)


def recipe(root, symbol, data, depth, address, source_size, needed):
    rom = (root / 'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
    assert hashlib.sha1(rom).hexdigest() == 'da957f0d63d14cb441d215462904c4fa8519c613'
    stride = depth * 8
    lookup = {}
    for off in range(0, source_size, stride):
        pixels = decode(rom[pc(address)+off:pc(address)+off+stride], depth)
        lookup.setdefault(signature(pixels), (address+off, pixels))
    refs, authored, rebuilt = [], [], bytearray(rom[pc(address):pc(address)+len(data)])
    for tile in sorted(needed):
        payload = data[tile*stride:(tile+1)*stride]
        pixels = decode(payload, depth)
        match = lookup.get(signature(pixels))
        if match:
            source, original = match
            colors = dict(zip(original, pixels))
            assert all(colors[a] == b for a, b in zip(original, pixels))
            refs.append((tile, source, [colors.get(i, 0) for i in range(16)]))
        else:
            authored.append((tile, payload))
        rebuilt[tile*stride:(tile+1)*stride] = payload
    # These lists are reviewable, fixed font/widget inventories, not a blanket
    # exception for arbitrary unidentified graphics added to an upstream sheet.
    approved = {
      'varia_gfx': set(range(0x10,0x1c))|set(range(0x20,0x30))|set(range(0x33,0x38))|
        set(range(0x3c,0x4c))|{0x4f,0x55,0x57,0x59,0x5a,0x60,0x63,0x64,0x65,0x66,0x6a,0x70}|
        set(range(0x55,0x7b))|{0x7a,0x8d,0x8e,0x8f,0xca}|set(range(0xd4,0xdc))|{0xfd},
      'objective_pause_gfx': {0x30,0x32,0x34,0x35,0x36,0x39,0x3a,0x3b,0x3c,0x40,0x41,0x43,
        0x45,0x46,0x47,0x48,0x49,0x4b,0x4e,0x87,0x88,0x95,0x9e,0xa3,0xa5,0xad,0xce,
        0x10c,0x10d,0x10e,0x11c,0x11d,0x11e,0x12c,0x12d,0x12e,0x144,0x14f,0x150,0x15f,
        0x161,0x162,0x163,0x164,0x165,0x166,0x167,0x168,0x169,0x175,0x176,0x177,0x17a,
        0x1b8,0x1b9,0x1ba,0x1bb,0x1bf,0x1d8,0x1d9,0x1f4,0x1f5,0x1f6,0x1f7,
        0x96,0x97,0x98,0xa6,0xa7,0xa8,0x4a,0xbc,0x1ab},
    }
    unexpected = {tile for tile, _ in authored} - approved[symbol]
    assert not unexpected, f'Unreviewed {symbol} artwork: {sorted(unexpected)}'
    lines = [f'static uint8_t {symbol}[{len(data)}];',
             f'static const SmUiRomTile {symbol}_rom[]={{']
    lines += ['{%d,0x%x,{%s}},' % (tile, source, ','.join(map(str, colors)))
              for tile, source, colors in refs]
    lines += ['};', f'static const struct {{uint16_t tile;uint8_t pixels[{stride}];}} {symbol}_varia[]={{']
    lines += ['{%d,{%s}},' % (tile, ','.join(map(str, payload))) for tile, payload in authored]
    lines += ['};', f'static void {symbol}_load(void){{',
              f' memcpy({symbol},RomFixedPtr(0x{address:x}),sizeof({symbol}));',
              f' for(unsigned i=0;i<sizeof({symbol}_rom)/sizeof(*{symbol}_rom);i++)',
              f'  sm_ui_rom_tile({symbol},{depth},&{symbol}_rom[i]);',
              f' for(unsigned i=0;i<sizeof({symbol}_varia)/sizeof(*{symbol}_varia);i++)',
              f'  memcpy({symbol}+{symbol}_varia[i].tile*{stride},{symbol}_varia[i].pixels,{stride});',
              '}']
    proof = dict(schema=1, romBase=hex(address), romTiles=len(refs), authoredTiles=[t for t,_ in authored],
                 usedTiles=sorted(needed), unusedPolicy='Original player ROM tile',
                 baselineSha256=hashlib.sha256(data).hexdigest(),
                 reconstructedSha256=hashlib.sha256(rebuilt).hexdigest(), usedTilesByteExact=True,
                 source='VARIA patches/common/src/map; max_ammo_display.asm',
                 authors='MFreak; VARIA contributors; Personitis, theonlydude, maddo (maximum ammo)',
                 recipeSha256=hashlib.sha256(('\n'.join(lines)+'\n').encode()).hexdigest())
    (root/'Docs/Releases/ALPHA-0.24'/f'{symbol}-rom-reconstruction.json').write_text(json.dumps(proof,indent=2)+'\n')
    return lines
