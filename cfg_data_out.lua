cfg = require('cfg_data')


function gens_to_txt(gens, finline)
    local cur_line, out_line = "", ""
    for i, e in pairs(gens) do
        cur_line = cur_line .. string.format("%s ", e)
        if #cur_line > 60 then
            out_line = out_line .. "    " .. cur_line .. finline .. "\n"
            cur_line = ""
        end
    end
    out_line = out_line .. "    " .. cur_line
    return out_line
end


function gens_to_lua_txt(gens, finline)
    local cur_line, out_line = "", ""
    for i, e in pairs(gens) do
        if i % 4 == 0 then
            cur_line= cur_line .. string.format("'%s', ", e)
        else
            cur_line = cur_line .. string.format("%-22s", string.format("'%s', ", e))
        end
        if i % 4 == 0 then
            out_line = out_line .. "    " .. cur_line .. finline .. "\n"
            cur_line = ""
        end
    end
    out_line = out_line .. "    " .. cur_line
    return out_line
end


local finline = ""
local gen_src = cfg.get_gen_sources()
print(gens_to_txt(gen_src, finline))
print(gens_to_lua_txt(gen_src, finline))
print("Number of generators: ", #gen_src)

