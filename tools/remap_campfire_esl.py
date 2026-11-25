"""Makes the ESL-Campfire version of a plugin that has Campfire.esm as a master.

Campfire 2026's ESL build renumbers every record of Campfire.esm; its
esl/map_campfire.json (https://github.com/CageTV/Campfire-2026) lists old -> new.
This rewrites the plugin's Campfire FormIDs: record headers (overrides of
Campfire's records) and every object value in script data (VMAD: scripts,
quest aliases). Usage:

    python remap_campfire_esl.py <plugin.esp> <out.esp> <map_campfire.json>

It finds Campfire's master index itself, refuses compressed records, and
reports anything it couldn't map. Other subrecords aren't rewritten: it
checks none of them holds a Campfire FormID it knows of, and warns if so.
"""
import json
import struct
import sys
import zlib


def main(src, dst, mapfile):
    mapping = {int(k, 16): int(v, 16) for k, v in json.load(open(mapfile)).items()}
    data = bytearray(open(src, 'rb').read())

    # Masters, to find Campfire.esm's index.
    header_size = struct.unpack_from('<I', data, 4)[0]
    masters, j = [], 24
    while j < 24 + header_size:
        tag = bytes(data[j:j + 4]); size = struct.unpack_from('<H', data, j + 4)[0]
        if tag == b'MAST':
            masters.append(bytes(data[j + 6:j + 6 + size - 1]).decode('latin-1'))
        j += 6 + size
    if 'Campfire.esm' not in masters:
        sys.exit('Campfire.esm is not a master of this plugin')
    cf = masters.index('Campfire.esm')

    missing, counts = set(), {'records': 0, 'script values': 0}

    def remap(form_id):
        if form_id >> 24 != cf or (form_id & 0xFFFFFF) <= 0xFFF and (form_id & 0xFFFFFF) not in mapping:
            return form_id
        old = form_id & 0xFFFFFF
        if old not in mapping:
            missing.add(hex(old))
            return form_id
        return (cf << 24) | mapping[old]

    def records(start, end):
        i = start
        while i < end:
            tag = bytes(data[i:i + 4]); size = struct.unpack_from('<I', data, i + 4)[0]
            if tag == b'GRUP':
                yield from records(i + 24, i + size)
                i += size
            else:
                yield tag, i, size
                i += 24 + size

    for tag, i, size in records(24 + header_size, len(data)):
        flags, form_id = struct.unpack_from('<II', data, i + 8)
        if flags & 0x40000:
            sys.exit(f'compressed record at {i:#x}: decompress it first (xEdit)')
        new = remap(form_id)
        if new != form_id:
            struct.pack_into('<I', data, i + 12, new); counts['records'] += 1
        j, end = i + 24, i + 24 + size
        while j < end:
            sub = bytes(data[j:j + 4]); sub_size = struct.unpack_from('<H', data, j + 4)[0]
            if sub == b'VMAD':
                # Object values (format 2): 0000 FFFF then the FormID.
                k, stop = j + 6, j + 6 + sub_size
                while k + 8 <= stop:
                    if data[k:k + 4] == b'\x00\x00\xff\xff':
                        value = struct.unpack_from('<I', data, k + 4)[0]
                        new = remap(value)
                        if new != value:
                            struct.pack_into('<I', data, k + 4, new); counts['script values'] += 1
                        k += 8
                        continue
                    k += 1
            j += 6 + sub_size

    open(dst, 'wb').write(data)
    print(f'{dst}: remapped {counts}; Campfire is master {cf}')
    if missing:
        print('NOT in the map (left as they were):', sorted(missing))


if __name__ == '__main__':
    main(*sys.argv[1:4])
