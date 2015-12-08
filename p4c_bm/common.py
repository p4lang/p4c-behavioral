def get_type(byte_width):
  if byte_width == 1:
    return "uint8_t"
  elif byte_width == 2:
    return "uint16_t"
  elif byte_width <= 4:
    return "uint32_t"
  else:
    return "uint8_t *"

# match_fields is list of tuples (name, type)
def gen_match_params(match_fields, field_info):
  params = []
  for field, type in match_fields:
    if type == "valid":
      params += [(field + "_valid", 1)]
      continue
    f_info = field_info[field]
    bytes_needed = (f_info["bit_width"] + 7 ) / 8
    params += [(field, bytes_needed)]
    if type == "lpm": params += [(field + "_prefix_length", 2)]
    if type == "ternary": params += [(field + "_mask", bytes_needed)]
  return params

def gen_action_params(names, byte_widths, _get_type = get_type):
  params = []
  for name, width in zip(names, byte_widths):
    name = "action_" + name
    params += [(name, width)]
  return params

