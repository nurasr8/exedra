#!/usr/bin/env python3
# Mark the El Torito EFI image of an isohybrid ISO as ESP in GPT.
# Finds the catalog-referenced boot image whose first sector is a FAT VBR,
# sets the covering GPT entry type to ESP GUID, fixes CRCs (primary+backup).
import struct
import sys
import zlib

ESP_GUID_LE = bytes.fromhex('28732ac11ff8d211ba4b00a0c93ec93b')


def rd(f, off, n):
    f.seek(off)
    return f.read(n)


def is_fat_vbr(sec):
    return len(sec) == 512 and sec[0] == 0xEB and sec[2] == 0x90 and \
        (b'FAT' in sec[0x36:0x3E] or b'FAT' in sec[0x52:0x5A])


def main(path):
    ss = 512
    with open(path, 'r+b') as f:
        f.seek(0, 2)
        size = f.tell()
        hdr = rd(f, ss, 92)
        if hdr[:8] != b'EFI PART':
            print('no GPT header at LBA1')
            return 1
        (sig, rev, hsz, hcrc, rsv, cur, bak, first, last,
         dguid, pent_lba, pent_n, pent_sz, pcrc) = struct.unpack('<8sIIIIQQQQ16sQIII', hdr[:92])
        if pent_sz != 128:
            print(f'entry size {pent_sz}, unsupported')
            return 1
        entries = rd(f, pent_lba * ss, pent_n * pent_sz)
        # El Torito: boot record volume descriptor at ISO LBA 17 (byte 32768+71 = catalog LBA)
        if rd(f, 17 * 2048 + 1, 5) != b'CD001':
            print('no ISO9660 boot record')
            return 1
        cat_lba_2k = struct.unpack('<I', rd(f, 17 * 2048 + 71, 4))[0]
        cat = rd(f, cat_lba_2k * 2048, 2048)
        fat_start = None
        for rec in range(0, 2048, 32):
            e = cat[rec:rec + 32]
            if len(e) < 32 or e[0] not in (0x88, 0x00):
                continue
            lba_2k = struct.unpack('<I', e[8:12])[0]
            if lba_2k and is_fat_vbr(rd(f, lba_2k * 2048, 512)):
                fat_start = lba_2k * 4  # FAT image start in 512-sectors
                break
        if fat_start is None:
            print('FAT boot image not found in El Torito catalog')
            return 1
        idx = None
        for i in range(pent_n):
            e = entries[i * 128:(i + 1) * 128]
            tguid, uguid, start, end, attr = struct.unpack('<16s16sQQQ', e[:56])
            if start == fat_start and end > start:
                idx = i
                break
        if idx is None:
            print(f'no GPT entry starts at FAT LBA {fat_start}')
            return 1
        e = bytearray(entries[idx * 128:(idx + 1) * 128])
        e[0:16] = ESP_GUID_LE
        entries = entries[:idx * 128] + bytes(e) + entries[(idx + 1) * 128:]
        new_pcrc = zlib.crc32(entries) & 0xffffffff

        def fix_header(at):
            h = bytearray(rd(f, at, hsz))
            struct.pack_into('<I', h, 16, 0)
            struct.pack_into('<I', h, 88, new_pcrc)
            struct.pack_into('<I', h, 16, zlib.crc32(bytes(h[:hsz])) & 0xffffffff)
            f.seek(at)
            f.write(h)

        f.seek(pent_lba * ss)
        f.write(entries)
        fix_header(ss)
        nsec = (pent_n * pent_sz + ss - 1) // ss
        f.seek((bak - nsec) * ss)
        f.write(entries)
        fix_header(bak * ss)
        # self-verify
        f.seek(0)
        ok = True
        for h_at, e_at in ((ss, pent_lba * ss), (bak * ss, (bak - nsec) * ss)):
            h = rd(f, h_at, hsz)
            if h[:8] != b'EFI PART':
                ok = False
                break
            hc = struct.unpack('<I', h[16:20])[0]
            h0 = bytearray(h)
            struct.pack_into('<I', h0, 16, 0)
            ent = rd(f, e_at, pent_n * pent_sz)
            if (zlib.crc32(bytes(h0[:hsz])) & 0xffffffff) != hc:
                ok = False
            if (zlib.crc32(ent) & 0xffffffff) != struct.unpack('<I', h[88:92])[0]:
                ok = False
        if not ok:
            print('GPT self-verify FAILED')
            return 2
        print(f'entry {idx + 1} (LBA {fat_start}) marked ESP, GPT CRCs updated+verified')
        return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1]))
