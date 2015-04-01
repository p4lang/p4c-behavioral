import sys

crc_ccitt = {"polynomial": 0x1021,
             "width": 16,
             "initial_remainder": 0xFFFF,
             "final_xor_value": 0x0000,
             "reflect_data": False,
             "reflect_remaider": False,
             "check_value": 0x29B1}

crc_32 = {"polynomial": 0x04C11DB7,
          "width": 32,
          "initial_remainder": 0xFFFFFFFF,
          "final_xor_value": 0xFFFFFFFF,
          "reflect_data": True,
          "reflect_remaider": True,
          "check_value": 0xCBF43926}

crc_16 = {"polynomial": 0x8005,
          "width": 16,
          "initial_remainder": 0x0000,
          "final_xor_value": 0x0000,
          "reflect_data": True,
          "reflect_remaider": True,
          "check_value": 0xBB3D}

CRCs = {"crc16": crc_16, "crc32": crc_32, "crcCCITT": crc_ccitt}

BYTES_PER_LINE = 16

def get_c_type(width):
    if width == 16: return "uint16_t"
    elif width == 32: return "uint32_t"
    else: assert(False)

def get_format_string(width):
    if width == 16: return "0x%0.4X"
    elif width == 32: return "0x%0.8X"
    else: assert(False);

for crc, info in CRCs.items():
    out = sys.stdout
    out.write("***************************************\n")
    out.write(crc + "\n")
    out.write("**********\n")

    table = [0 for i in xrange(256)]
    for dividend in xrange(256):
        remainder = dividend << (info["width"] - 8)
        for bit in xrange(8):
            if (remainder & (1 << (info["width"] - 1))):
                remainder = (remainder << 1) ^ info["polynomial"]
            else:
                remainder = (remainder << 1)
        table[dividend] = remainder % (1 << info["width"])
    
    type_ = get_c_type(info["width"])
    format_ = get_format_string(info["width"])
    WORDS_PER_LINE = BYTES_PER_LINE / (info["width"] / 8)
    out.write(type_ + " table_" + crc + "[256] = {\n")
    for i in xrange(256):
        if i % WORDS_PER_LINE == 0:
            out.write("  ")
        out.write(format_ % table[i])
        if i != 255:
            out.write(", ")
        if (i + 1) % WORDS_PER_LINE == 0:
            out.write("\n")
    out.write("};\n")
    out.write("***************************************\n")
