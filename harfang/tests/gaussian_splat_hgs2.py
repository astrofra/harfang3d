"""Independent stdlib-only HGS2 inspector for converter/integration tests."""
import math
from pathlib import Path
import struct
import zlib


def check(condition, message):
    if not condition:
        raise ValueError(message)


def crc(data):
    return zlib.crc32(data) & 0xffffffff


def header_crc(header):
    copy = bytearray(header[:128])
    copy[104:108] = bytes(4)
    return crc(copy)


def quaternion(word):
    largest = word >> 30
    indices = [i for i in range(4) if i != largest]
    q = [0.] * 4
    for slot, axis in enumerate(reversed(indices)):
        component = (word >> (10 * slot)) & 1023
        q[axis] = (component & 511) / (511 * math.sqrt(2)) * (-1 if component & 512 else 1)
    remainder = 1 - sum(v*v for v in q)
    check(remainder >= 0, 'invalid smallest-three quaternion')
    q[largest] = math.sqrt(remainder)
    return (q[3], q[0], q[1], q[2])


def read_hgs(path):
    raw = Path(path).read_bytes()
    check(len(raw) >= 128 and raw[:4] == b'HGS2', 'expected HGS2 header')
    check(struct.unpack_from('<HHII', raw, 4) == (2, 0, 0x01020304, 128), 'invalid HGS2 version/header')
    total, = struct.unpack_from('<Q', raw, 16)
    flags, degree, count, source_count, storage, block_size, block_count, entry_size = struct.unpack_from('<8I', raw, 24)
    table_offset, payload_offset = struct.unpack_from('<QQ', raw, 56)
    lo, hi = struct.unpack_from('<3f', raw, 72), struct.unpack_from('<3f', raw, 84)
    raw_size, checksum, table_checksum = struct.unpack_from('<QII', raw, 96)
    check(total == len(raw) and total <= 512*1024*1024, 'invalid HGS2 file size')
    check(0 < count <= 2097152 and source_count >= count and degree <= 3 and flags & ~7 == 0, 'invalid HGS2 counts/flags')
    check(storage in (0, 1) and block_size == 256 and entry_size == 32 and block_count == (count+255)//256, 'invalid HGS2 storage/blocks')
    check(table_offset == 128 and payload_offset == 128 + 32*block_count and payload_offset <= len(raw), 'invalid HGS2 table offsets')
    check(raw[112:128] == bytes(16) and checksum == header_crc(raw), 'invalid HGS2 header CRC/reserved')
    check(table_checksum == crc(raw[128:payload_offset]), 'invalid HGS2 table CRC')
    check(all(math.isfinite(x) for x in lo+hi) and all(a <= b for a,b in zip(lo,hi)), 'invalid HGS2 bounds')
    non_dc = (degree+1)**2-1
    check(bool(flags & 4) == bool(non_dc), 'invalid HGS2 SH flag')
    base, higher, blocks = [], [], []
    cursor, accumulated = payload_offset, 0
    for b in range(block_count):
        offset, compressed_len, raw_len, n, block_crc, reserved = struct.unpack_from('<Q4IQ', raw, 128+32*b)
        check(offset == cursor and 0 < compressed_len <= len(raw)-cursor and reserved == 0, 'invalid HGS2 block offset/size/reserved')
        check(n == min(256, count-b*256), 'invalid HGS2 block count')
        expected = n*(64+16*non_dc) if storage == 0 else 72+12*non_dc+n*(17+3*non_dc)
        check(raw_len == expected, 'invalid HGS2 raw length')
        decoder = zlib.decompressobj()
        unpacked = decoder.decompress(raw[cursor:cursor+compressed_len], raw_len+1)
        check(decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail and len(unpacked) == raw_len, 'invalid HGS2 compressed block')
        check(crc(unpacked) == block_crc, 'invalid HGS2 block CRC')
        blocks.append({'data': unpacked, 'count': n, 'offset': offset})
        cursor += compressed_len
        accumulated += raw_len
        if storage == 0:
            base.extend(struct.unpack_from('<16f', unpacked, 64*i) for i in range(n))
            higher.extend([struct.unpack_from('<4f', unpacked, n*64+(i*non_dc+k)*16) for k in range(non_dc)] for i in range(n))
            continue
        ranges = struct.unpack_from('<'+'f'*(18+3*non_dc), unpacked)
        check(all(math.isfinite(x) for x in ranges), 'non-finite HGS2 quantizer')
        for start in (0,6,12):
            check(all(ranges[start+j] <= ranges[start+3+j] for j in range(3)), 'inverted HGS2 quantizer')
        check(all(x > 0 for x in ranges[6:12]) and all(x >= 0 for x in ranges[18:]), 'invalid HGS2 scale/SH quantizer')
        pos = 72+12*non_dc
        alpha, scale, rotation, dc, sh = pos+6*n, pos+7*n, pos+10*n, pos+14*n, pos+17*n
        def lerp(a,b,v,denom):
            return a+(b-a)*(v/denom)
        for i in range(n):
            p = [lerp(ranges[j],ranges[3+j],struct.unpack_from('<H',unpacked,pos+6*i+2*j)[0],65535) for j in range(3)]
            scales = [min(ranges[9+j], max(ranges[6+j], math.exp(lerp(math.log(ranges[6+j]),math.log(ranges[9+j]),unpacked[scale+3*i+j],255)))) for j in range(3)]
            colors = [lerp(ranges[12+j],ranges[15+j],unpacked[dc+3*i+j],255) for j in range(3)]
            q = quaternion(struct.unpack_from('<I',unpacked,rotation+4*i)[0])
            base.append(tuple(p+[unpacked[alpha+i]/255]+scales+[0]+list(q)+colors+[0]))
            coefficients=[]
            for k in range(non_dc):
                values=struct.unpack_from('<3b',unpacked,sh+(i*non_dc+k)*3)
                check(-128 not in values, 'invalid HGS2 signed SH code')
                check(all(ranges[18+3*k+j] != 0 or v == 0 for j,v in enumerate(values)), 'nonzero code for constant SH')
                coefficients.append(tuple(v/127*ranges[18+3*k+j] for j,v in enumerate(values))+(0.,))
            higher.append(coefficients)
    check(cursor == total and accumulated == raw_size, 'invalid HGS2 aggregate lengths')
    for record in base:
        check(all(math.isfinite(v) for v in record) and 0 <= record[3] <= 1 and all(v > 0 for v in record[4:7]), 'invalid HGS2 decoded base')
        check(record[7] == record[15] == 0 and abs(sum(v*v for v in record[8:12])-1)<.002, 'invalid HGS2 decoded quaternion/padding')
    for record in (r for records in higher for r in records):
        check(all(math.isfinite(v) for v in record) and record[3] == 0, 'invalid HGS2 SH')
    for j in range(3):
        for actual, expected in ((min(r[j] for r in base),lo[j]),(max(r[j] for r in base),hi[j])):
            check(abs(actual-expected) <= 1e-5*max(1,abs(actual),abs(expected)), 'HGS2 bounds/payload mismatch')
    return dict(raw=raw, flags=flags, sh_degree=degree, splat_count=count, source_count=source_count,
                total_size=total, storage=storage, block_count=block_count, blocks=blocks,
                raw_size=raw_size, bounds_min=lo, bounds_max=hi, non_dc_coefficient_count=non_dc,
                base=base, higher=higher,
                base_bytes=b''.join(struct.pack('<16f',*r) for r in base),
                higher_bytes=b''.join(struct.pack('<4f',*r) for rows in higher for r in rows))


def repack(raw, blocks):
    """Repack edited raw blocks for negative loader tests; retain intentional corrupt contents."""
    header = bytearray(raw[:128])
    table = bytearray()
    payload = bytearray()
    offset = 128+32*len(blocks)
    raw_size = 0
    for i, block in enumerate(blocks):
        compressed = zlib.compress(block)
        n = struct.unpack_from('<I',raw,128+32*i+16)[0]
        table += struct.pack('<Q4IQ',offset,len(compressed),len(block),n,crc(block),0)
        payload += compressed
        offset += len(compressed)
        raw_size += len(block)
    struct.pack_into('<Q',header,16,offset)
    struct.pack_into('<Q',header,96,raw_size)
    struct.pack_into('<I',header,108,crc(table))
    struct.pack_into('<I',header,104,header_crc(header))
    return bytes(header+table+payload)
