funcs = require('cfg_prngens_funcs')

function find_generators()
    local generators = {}
    if package.config:sub(1,1) == "\\" then
        local lines = io.popen([[dir "generators\*.c" /b]]):lines()
        for line in lines do
            local _, _, gen = string.find(line, "([%w_]+)")
            table.insert(generators, gen)
        end
    else
        local lines = io.popen([[ls -1d ./generators/*.c | xargs -n 1 basename]]):lines()
        for line in lines do
            local _, _, gen = string.find(line, "(%w+)")
            table.insert(generators, gen)
        end
    end
    return generators
end

local finline = ""
local gen_src = find_generators()
print(funcs.gens_to_txt(gen_src, finline))
print(funcs.gens_to_lua_txt(gen_src, finline))
print("Number of generators: ", #gen_src)
