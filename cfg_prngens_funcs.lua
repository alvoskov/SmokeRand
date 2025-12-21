local mod = {
gens_to_txt = function(gens, finline)
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
end,

gens_to_lua_txt = function(gens, finline)
    local cur_line, out_line = "", ""
    for i, e in pairs(gens) do
        if i % 4 == 0 then
            cur_line= cur_line .. string.format("'%s', ", e)
        else
            cur_line = cur_line .. string.format("%-22s", string.format("'%s',", e))
        end
        if i % 4 == 0 then
            out_line = out_line .. "    " .. cur_line .. finline .. "\n"
            cur_line = ""
        end
    end
    out_line = out_line .. "    " .. cur_line
    return out_line
end
}

return mod
