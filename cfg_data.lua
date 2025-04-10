local lib_sources = {'core.c', 'coretests.c', 'entropy.c', 'extratests.c',
    'fileio.c', 'lineardep.c', 'hwtests.c', 'specfuncs.c', 'threads_intf.c'}

local bat_sources = {'bat_express.c', 'bat_brief.c', 'bat_default.c',
    'bat_file.c', 'bat_full.c', 'bat_special.c'}

local lib_headers = {'apidefs.h', 'cinterface.h', 'core.h', 'coretests.h',
    'entropy.h', 'extratests.h', 'fileio.h', 'hwtests.h', 'int128defs.h',
    'lineardep.h', 'specfuncs.h', 'threads_intf.h', 'x86exts.h'}


-- List of all generators; some of them are not portable, e.g. can require
-- AVX or AESNI extensions, 128-bit arithmetics etc.
local gen_sources = {'aesni', 'alfib_lux', 'alfib_mod', 'alfib', 'ara32',
    'chacha', 'cmwc4096', 'coveyou64', 'cwg64', 'des',
    'drand48', 'efiix64x48', 'flea32x1', 'hc256', 'isaac64', 'kiss64',
    'kiss93', 'kiss99', 'kuzn', 'lcg128',
    'lcg32prime', 'lcg64prime',  'lcg64', 'lcg69069',
    'lcg96', 'lea', 'lfib4', 'lfib4_u64', 'lfib_par', 'lfib_ranmar', 'lfsr113',
    'lfsr258', 'loop_7fff_w64', 'lrnd64_255', 'lrnd64_1023',
    'lxm_64x128', 'macmarsa', 'magma', 'minstd', 'mixmax',
    'mlfib17_5', 'mrg32k3a', 'msws_ctr', 'msws', 'msws64', 'mt19937', 'mulberry32',
    'mwc128x', 'mwc128', 'mwc128xxa32', 'mwc256xxa64', 'mwc1616x', 'mwc1616',
    'mwc3232x', 'mwc32x', 'mwc32xxa8', 'mwc40xxa8', 'mwc4691', 'mwc48xxa16',
    'mwc64x', 'mwc64', 'pcg32', 'pcg32_xsl_rr', 'pcg64_64', 'pcg64_xsl_rr',
    'philox2x32', 'philox32', 'philox', 'r1279', 'randu', 'ranlux48', 'ranluxpp',
    'ranq1', 'ranq2', 'ranrot32', 'ranrot_bi', 'ranshi', 'ranval', 'ran',
    'rc4ok', 'rc4', 'romutrio', 'rrmxmx', 'sapparot2', 'sapparot', 'sezgin63',
    'sfc16', 'sfc32', 'sfc64', 'sfc8', 'shr3', 'speck64_128',
    'speck128', 'speck128sc', 'splitmix32', 'splitmix', 'sqxor32',
    'sqxor', 'stormdrop', 'superduper64',
    'superduper73', 'swblarge', 'swblux', 'swbw', 'swb', 'taus88',
    'threefry2x64', 'threefry', 'tinymt32',
    'tinymt64', 'well1024a', 'wyrand', 'xorgens', 'xoroshiro1024stst',
    'xoroshiro1024st', 'xoroshiro128pp', 'xoroshiro128pp_avx',
    'xoroshiro128pp_vec', 'xoroshiro128p', 'xorshift128pp_avx', 'xorshift128p',
    'xorshift128', 'xorwow', 'xoshiro128p', 'xoshiro128pp', 'xsh',
    'xtea', 'xxtea'}

-- List of non-portable generators that use compiler-specific extensions
local gen_not_portable_sources = {'xoroshiro128pp_avx', 'xorshift128pp_avx'}

local function exclude_non_portables()
    local np_inv, filtered = {}, {}
    for _, v in pairs(gen_not_portable_sources) do
        np_inv[v] = true
    end
    for _, v in pairs(gen_sources) do
        if np_inv[v] == nil then
            table.insert(filtered, v)
        end
    end
    return filtered
end


return {
    get_lib_headers = function() return lib_headers end,
    get_lib_sources = function() return lib_sources end,
    get_bat_sources = function() return bat_sources end,
    get_gen_sources = function(is_portable)
        if is_portable then
            return exclude_non_portables()
        else
            return gen_sources
        end
    end
}
