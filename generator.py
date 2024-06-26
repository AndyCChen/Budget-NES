def write_chunk1(file, chunk_number: int):
   cycle = chunk_number * 8

   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",             label ="// cycle", cycle = cycle + 1))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&fetch_nametable,",        label ="// cycle", cycle = cycle + 2))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",             label ="// cycle", cycle = cycle + 3))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&fetch_attribute,",        label ="// cycle", cycle = cycle + 4))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",             label ="// cycle", cycle = cycle + 5))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&fetch_pattern_table_lo,", label ="// cycle", cycle = cycle + 6))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&fetch_pattern_table_hi,", label ="// cycle", cycle = cycle + 7))

   if (cycle + 8 != 256):
      file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&increment_v_horizontal,", label ="// cycle", cycle = cycle + 8))
   else:
      file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&increment_v_both,",       label ="// cycle", cycle = cycle + 8))

def write_chunk2(file, chunk_number: int):
   cycle = chunk_number * 8

   if (cycle + 1 == 257):
      file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&transfer_t_horizontal,", label ="// cycle", cycle = cycle + 1))
   else:
      file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",            label ="// cycle", cycle = cycle + 1))
   
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&fetch_nametable,",          label ="// cycle", cycle = cycle + 2))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",               label ="// cycle", cycle = cycle + 3))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&fetch_nametable,",          label ="// cycle", cycle = cycle + 4))
   if (cycle + 5 == 261):
      file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&fetch_sprites,",         label ="// cycle", cycle = cycle + 5))
   else:
      file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",            label ="// cycle", cycle = cycle + 5))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",               label ="// cycle", cycle = cycle + 6))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",               label ="// cycle", cycle = cycle + 7))
   file.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",               label ="// cycle", cycle = cycle + 8))


with open ("src/ppu_renderer_lookup.c", "w", encoding="utf-8") as source:
   source.write("// This file is generated by the generator.py script, it contains a lookup table of function pointers for what to do on each cycle of a ppu scanline.\n\n")
   source.write("#include \"../includes/ppu_renderer_lookup.h\"\n")
   source.write("#include \"../includes/ppu.h\"\n\n")

   # -- lookup table
   source.write ("const render_events_t scanline_lookup[341] =\n")
   source.write ("{\n")
   source.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,", label ="// cycle", cycle = 0))

   for chunk in range(0, 32):
      write_chunk1(source, chunk)
   
   for chunk in range(32, 40):
      write_chunk2(source, chunk)
   
   for chunk in range(40, 42):
      write_chunk1(source, chunk)

   # final 2 unused nametable fetch
   source.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",      label ="// cycle", cycle = 337))
   source.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&fetch_nametable,", label ="// cycle", cycle = 338))
   source.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&rest_cycle,",      label ="// cycle", cycle = 339))
   source.write('\t{func:<30} {label:>30} {cycle}\n'.format(func = "&fetch_nametable,", label ="// cycle", cycle = 340))
   source.write ("};\n\n")