local lib_sources = {'core.c', 'coretests.c', 'entropy.c', 'extratests.c',
    'fileio.c', 'lineardep.c', 'hwtests.c', 'specfuncs.c', 'threads_intf.c'}

local bat_sources = {'bat_express.c', 'bat_brief.c', 'bat_default.c',
    'bat_file.c', 'bat_full.c', 'bat_special.c'}

local lib_headers = {'apidefs.h', 'cinterface.h', 'core.h', 'coretests.h',
    'entropy.h', 'extratests.h', 'fileio.h', 'hwtests.h', 'int128defs.h',
    'lineardep.h', 'specfuncs.h', 'threads_intf.h', 'x86exts.h'}


-- List of all generators; all of them are portable and have at least some
-- cross-platform variants.
local gen_sources = {
    'aes128', 'alfib', 'alfib_lux', 'alfib_mod', 'ara32', 'chacha', 'cmwc4096',
    'coveyou64', 'cwg64', 'des', 'drand48', 'efiix64x48', 'flea32x1', 'gmwc128',
    'hc256', 'isaac64', 'kiss64', 'kiss93', 'kiss99', 'kuzn', 'lcg128', 'lcg32prime',
    'lcg64', 'lcg64prime', 'lcg69069', 'lcg96', 'lea', 'lfib4', 'lfib4_u64',
    'lfib_par', 'lfib_ranmar', 'lfsr113', 'lfsr258', 'loop_7fff_w64',
    'lrnd64_1023', 'lrnd64_255', 'lxm_64x128', 'macmarsa', 'magma', 'minstd',
    'mixmax', 'mlfib17_5', 'mrg32k3a', 'msws', 'msws64', 'msws64x', 'msws_ctr',
    'mt19937', 'mulberry32', 'mwc128', 'mwc128x', 'mwc128xxa32', 'mwc1616',
    'mwc1616x', 'mwc256xxa64', 'mwc3232x', 'mwc32x', 'mwc32xxa8', 'mwc40xxa8',
    'mwc4691', 'mwc48xxa16', 'mwc64', 'mwc64x', 'pcg32', 'pcg32_xsl_rr',
    'pcg64_64', 'pcg64_xsl_rr', 'philox', 'philox2x32', 'philox32', 'r1279',
    'ran', 'randu', 'ranlux48', 'ranluxpp', 'ranq1', 'ranq2', 'ranrot32',   
    'ranrot_bi', 'ranshi', 'ranval', 'rc4', 'rc4ok', 'romutrio', 'rrmxmx',
    'sapparot', 'sapparot2', 'sezgin63', 'sfc16', 'sfc32', 'sfc64', 'sfc8',
    'shr3', 'speck128', 'speck128sc', 'speck64_128', 'splitmix', 'splitmix32', 'splitmix32cbc',
    'sqxor', 'sqxor32', 'stormdrop', 'superduper64', 'superduper73', 'swb',
    'swblarge', 'swblux', 'swbw', 'taus88', 'threefry', 'threefry2x64',
    'tinymt32', 'tinymt64', 'tylo64', 'well1024a', 'wob2m', 'wyrand', 'xorgens',
    'xoroshiro1024st', 'xoroshiro1024stst', 'xoroshiro128p', 'xoroshiro128pp',
    'xoroshiro128pp_vec', 'xorshift128', 'xorshift128p', 'xorshift128pp',
    'xorwow', 'xoshiro128p', 'xoshiro128pp', 'xsh', 'xtea', 'xxtea'
}

return {
    get_lib_headers = function() return lib_headers end,
    get_lib_sources = function() return lib_sources end,
    get_bat_sources = function() return bat_sources end,
    get_gen_sources = function() return gen_sources end
}
