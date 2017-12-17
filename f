offset_t read_rv() {
  offset_t soff1 = region.allocate(8, 224);
  in.read_bytes(region, soff1, 1);
  if (!region.load_bit(soff1, 0)) { // pk
    offset_t soff2 = soff1 + 8;
    in.read_bytes(region, soff2, 1);
    if (!region.load_bit(soff2, 0)) { // contig
      int len4 = in.read_int();
      offset_t boff3 = region.allocate(4, 4 + len4);
      region.store_int(boff3, len4);
      in.read_bytes(region, boff3 + 4, len4);
      region.store_offset(soff2 + 8, boff3);
    } // end field 0 
    if (!region.load_bit(soff2, 1)) { // position
      region.store_int(soff2 + 4, in.read_int());
    } // end field 1 
  } // end field 0 
  if (!region.load_bit(soff1, 1)) { // v
    offset_t soff5 = soff1 + 24;
    in.read_bytes(region, soff5, 1);
    if (!region.load_bit(soff5, 0)) { // contig
      int len7 = in.read_int();
      offset_t boff6 = region.allocate(4, 4 + len7);
      region.store_int(boff6, len7);
      in.read_bytes(region, boff6 + 4, len7);
      region.store_offset(soff5 + 8, boff6);
    } // end field 0 
    if (!region.load_bit(soff5, 1)) { // start
      region.store_int(soff5 + 4, in.read_int());
    } // end field 1 
    if (!region.load_bit(soff5, 2)) { // ref
      int len9 = in.read_int();
      offset_t boff8 = region.allocate(4, 4 + len9);
      region.store_int(boff8, len9);
      in.read_bytes(region, boff8 + 4, len9);
      region.store_offset(soff5 + 16, boff8);
    } // end field 2 
    if (!region.load_bit(soff5, 3)) { // altAlleles
      offset_t aoff10;
      int len11 = in.read_int();
      int nmissing12 = (len11 + 7) >> 3;
      int elems_off13 = alignto(4 + nmissing12, 8);
      aoff10 = region.allocate(8, elems_off13 + len11 * 24);
      region.store_int(aoff10, len11);
      in.read_bytes(region, aoff10 + 4, nmissing12);
      for (int i14 = 0; i14 < len11; ++i14) {
	if (!region.load_bit(aoff10 + 4, i14)) {
	  offset_t elem_off15 = aoff10 + elems_off13 + i14 * 24;
	  in.read_bytes(region, elem_off15, 1);
	  if (!region.load_bit(elem_off15, 0)) { // ref
	    int len17 = in.read_int();
	    offset_t boff16 = region.allocate(4, 4 + len17);
	    region.store_int(boff16, len17);
	    in.read_bytes(region, boff16 + 4, len17);
	    region.store_offset(elem_off15 + 8, boff16);
	  } // end field 0 
	  if (!region.load_bit(elem_off15, 1)) { // alt
	    int len19 = in.read_int();
	    offset_t boff18 = region.allocate(4, 4 + len19);
	    region.store_int(boff18, len19);
	    in.read_bytes(region, boff18 + 4, len19);
	    region.store_offset(elem_off15 + 16, boff18);
	  } // end field 1 
	} // end elem missing
      } // end a
      region.store_offset(soff5 + 24, aoff10);
    } // end field 3 
  } // end field 1 
  if (!region.load_bit(soff1, 2)) { // va
    offset_t soff20 = soff1 + 56;
    in.read_bytes(region, soff20, 1);
    if (!region.load_bit(soff20, 0)) { // rsid
      int len22 = in.read_int();
      offset_t boff21 = region.allocate(4, 4 + len22);
      region.store_int(boff21, len22);
      in.read_bytes(region, boff21 + 4, len22);
      region.store_offset(soff20 + 8, boff21);
    } // end field 0 
    if (!region.load_bit(soff20, 1)) { // qual
      region.store_double(soff20 + 16, in.read_double());
    } // end field 1 
    if (!region.load_bit(soff20, 2)) { // filters
      offset_t aoff23;
      int len24 = in.read_int();
      int nmissing25 = (len24 + 7) >> 3;
      int elems_off26 = alignto(4 + nmissing25, 8);
      aoff23 = region.allocate(8, elems_off26 + len24 * 8);
      region.store_int(aoff23, len24);
      in.read_bytes(region, aoff23 + 4, nmissing25);
      for (int i27 = 0; i27 < len24; ++i27) {
	if (!region.load_bit(aoff23 + 4, i27)) {
	  offset_t elem_off28 = aoff23 + elems_off26 + i27 * 8;
	  int len30 = in.read_int();
	  offset_t boff29 = region.allocate(4, 4 + len30);
	  region.store_int(boff29, len30);
	  in.read_bytes(region, boff29 + 4, len30);
	  region.store_offset(elem_off28, boff29);
	} // end elem missing
      } // end a
      region.store_offset(soff20 + 24, aoff23);
    } // end field 2 
    if (!region.load_bit(soff20, 3)) { // info
      offset_t soff31 = soff20 + 32;
      in.read_bytes(region, soff31, 3);
      if (!region.load_bit(soff31, 0)) { // AC
	offset_t aoff32;
	int len33 = in.read_int();
	int nmissing34 = (len33 + 7) >> 3;
	int elems_off35 = alignto(4 + nmissing34, 4);
	aoff32 = region.allocate(4, elems_off35 + len33 * 4);
	region.store_int(aoff32, len33);
	in.read_bytes(region, aoff32 + 4, nmissing34);
	for (int i36 = 0; i36 < len33; ++i36) {
	  if (!region.load_bit(aoff32 + 4, i36)) {
	    offset_t elem_off37 = aoff32 + elems_off35 + i36 * 4;
	    region.store_int(elem_off37, in.read_int());
	  } // end elem missing
	} // end a
	region.store_offset(soff31 + 8, aoff32);
      } // end field 0 
      if (!region.load_bit(soff31, 1)) { // AF
	offset_t aoff38;
	int len39 = in.read_int();
	int nmissing40 = (len39 + 7) >> 3;
	int elems_off41 = alignto(4 + nmissing40, 8);
	aoff38 = region.allocate(8, elems_off41 + len39 * 8);
	region.store_int(aoff38, len39);
	in.read_bytes(region, aoff38 + 4, nmissing40);
	for (int i42 = 0; i42 < len39; ++i42) {
	  if (!region.load_bit(aoff38 + 4, i42)) {
	    offset_t elem_off43 = aoff38 + elems_off41 + i42 * 8;
	    region.store_double(elem_off43, in.read_double());
	  } // end elem missing
	} // end a
	region.store_offset(soff31 + 16, aoff38);
      } // end field 1 
      if (!region.load_bit(soff31, 2)) { // AN
	region.store_int(soff31 + 4, in.read_int());
      } // end field 2 
      if (!region.load_bit(soff31, 3)) { // BaseQRankSum
	region.store_double(soff31 + 24, in.read_double());
      } // end field 3 
      if (!region.load_bit(soff31, 4)) { // ClippingRankSum
	region.store_double(soff31 + 32, in.read_double());
      } // end field 4 
      if (!region.load_bit(soff31, 5)) { // DP
	region.store_int(soff31 + 40, in.read_int());
      } // end field 5 
      if (!region.load_bit(soff31, 6)) { // DS
	region.store_bool(soff31 + 3, in.read_bool());
      } // end field 6 
      if (!region.load_bit(soff31, 7)) { // FS
	region.store_double(soff31 + 48, in.read_double());
      } // end field 7 
      if (!region.load_bit(soff31, 8)) { // HaplotypeScore
	region.store_double(soff31 + 56, in.read_double());
      } // end field 8 
      if (!region.load_bit(soff31, 9)) { // InbreedingCoeff
	region.store_double(soff31 + 64, in.read_double());
      } // end field 9 
      if (!region.load_bit(soff31, 10)) { // MLEAC
	offset_t aoff44;
	int len45 = in.read_int();
	int nmissing46 = (len45 + 7) >> 3;
	int elems_off47 = alignto(4 + nmissing46, 4);
	aoff44 = region.allocate(4, elems_off47 + len45 * 4);
	region.store_int(aoff44, len45);
	in.read_bytes(region, aoff44 + 4, nmissing46);
	for (int i48 = 0; i48 < len45; ++i48) {
	  if (!region.load_bit(aoff44 + 4, i48)) {
	    offset_t elem_off49 = aoff44 + elems_off47 + i48 * 4;
	    region.store_int(elem_off49, in.read_int());
	  } // end elem missing
	} // end a
	region.store_offset(soff31 + 72, aoff44);
      } // end field 10 
      if (!region.load_bit(soff31, 11)) { // MLEAF
	offset_t aoff50;
	int len51 = in.read_int();
	int nmissing52 = (len51 + 7) >> 3;
	int elems_off53 = alignto(4 + nmissing52, 8);
	aoff50 = region.allocate(8, elems_off53 + len51 * 8);
	region.store_int(aoff50, len51);
	in.read_bytes(region, aoff50 + 4, nmissing52);
	for (int i54 = 0; i54 < len51; ++i54) {
	  if (!region.load_bit(aoff50 + 4, i54)) {
	    offset_t elem_off55 = aoff50 + elems_off53 + i54 * 8;
	    region.store_double(elem_off55, in.read_double());
	  } // end elem missing
	} // end a
	region.store_offset(soff31 + 80, aoff50);
      } // end field 11 
      if (!region.load_bit(soff31, 12)) { // MQ
	region.store_double(soff31 + 88, in.read_double());
      } // end field 12 
      if (!region.load_bit(soff31, 13)) { // MQ0
	region.store_int(soff31 + 44, in.read_int());
      } // end field 13 
      if (!region.load_bit(soff31, 14)) { // MQRankSum
	region.store_double(soff31 + 96, in.read_double());
      } // end field 14 
      if (!region.load_bit(soff31, 15)) { // QD
	region.store_double(soff31 + 104, in.read_double());
      } // end field 15 
      if (!region.load_bit(soff31, 16)) { // ReadPosRankSum
	region.store_double(soff31 + 112, in.read_double());
      } // end field 16 
      if (!region.load_bit(soff31, 17)) { // set
	int len57 = in.read_int();
	offset_t boff56 = region.allocate(4, 4 + len57);
	region.store_int(boff56, len57);
	in.read_bytes(region, boff56 + 4, len57);
	region.store_offset(soff31 + 120, boff56);
      } // end field 17 
    } // end field 3 
  } // end field 2 
  if (!region.load_bit(soff1, 3)) { // gs
    offset_t aoff58;
    int len59 = in.read_int();
    int nmissing60 = (len59 + 7) >> 3;
    int elems_off61 = alignto(4 + nmissing60, 8);
    aoff58 = region.allocate(8, elems_off61 + len59 * 32);
    region.store_int(aoff58, len59);
    in.read_bytes(region, aoff58 + 4, nmissing60);
    for (int i62 = 0; i62 < len59; ++i62) {
      if (!region.load_bit(aoff58 + 4, i62)) {
	offset_t elem_off63 = aoff58 + elems_off61 + i62 * 32;
	in.read_bytes(region, elem_off63, 1);
	if (!region.load_bit(elem_off63, 0)) { // gt
	  region.store_int(elem_off63 + 4, in.read_int());
	} // end field 0 
	if (!region.load_bit(elem_off63, 1)) { // ad
	  offset_t aoff64;
	  int len65 = in.read_int();
	  int nmissing66 = (len65 + 7) >> 3;
	  int elems_off67 = alignto(4 + nmissing66, 4);
	  aoff64 = region.allocate(4, elems_off67 + len65 * 4);
	  region.store_int(aoff64, len65);
	  in.read_bytes(region, aoff64 + 4, nmissing66);
	  for (int i68 = 0; i68 < len65; ++i68) {
	    if (!region.load_bit(aoff64 + 4, i68)) {
	      offset_t elem_off69 = aoff64 + elems_off67 + i68 * 4;
	      region.store_int(elem_off69, in.read_int());
	    } // end elem missing
	  } // end a
	  region.store_offset(elem_off63 + 8, aoff64);
	} // end field 1 
	if (!region.load_bit(elem_off63, 2)) { // dp
	  region.store_int(elem_off63 + 16, in.read_int());
	} // end field 2 
	if (!region.load_bit(elem_off63, 3)) { // gq
	  region.store_int(elem_off63 + 20, in.read_int());
	} // end field 3 
	if (!region.load_bit(elem_off63, 4)) { // px
	  offset_t aoff70;
	  int len71 = in.read_int();
	  int nmissing72 = (len71 + 7) >> 3;
	  int elems_off73 = alignto(4 + nmissing72, 4);
	  aoff70 = region.allocate(4, elems_off73 + len71 * 4);
	  region.store_int(aoff70, len71);
	  in.read_bytes(region, aoff70 + 4, nmissing72);
	  for (int i74 = 0; i74 < len71; ++i74) {
	    if (!region.load_bit(aoff70 + 4, i74)) {
	      offset_t elem_off75 = aoff70 + elems_off73 + i74 * 4;
	      region.store_int(elem_off75, in.read_int());
	    } // end elem missing
	  } // end a
	  region.store_offset(elem_off63 + 24, aoff70);
	} // end field 4 
	if (!region.load_bit(elem_off63, 5)) { // fakeRef
	  region.store_bool(elem_off63 + 1, in.read_bool());
	} // end field 5 
	if (!region.load_bit(elem_off63, 6)) { // isLinearScale
	  region.store_bool(elem_off63 + 2, in.read_bool());
	} // end field 6 
      } // end elem missing
    } // end a
    region.store_offset(soff1 + 216, aoff58);
  } // end field 3 
  return soff1;
}
