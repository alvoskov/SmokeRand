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

local finline = ""--"\\"
print("# Not portable")
print(gens_to_txt(cfg.get_gen_sources(false), finline))
print("# Portable")
print(gens_to_txt(cfg.get_gen_sources(true), finline))

