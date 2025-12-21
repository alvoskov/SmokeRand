cfg = require('cfg_data')
funcs = require('cfg_prngens_funcs')

local finline = ""
local gen_src = cfg.get_gen_sources()
print(funcs.gens_to_txt(gen_src, finline))
print(funcs.gens_to_lua_txt(gen_src, finline))
print("Number of generators: ", #gen_src)
