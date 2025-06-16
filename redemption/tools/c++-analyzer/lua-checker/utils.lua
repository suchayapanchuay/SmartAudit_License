-- LPeg utility

local peg = require'lpeg'

local P = peg.P

peg.Until = function(p)
    return (1-P(p))^0
end

peg.After = function(p)
    p = P(p)
    return ((1-p)^0 * p)
end

peg.wordchars = peg.R('az','AZ','09') + '_'
peg.word = peg.wordchars^1
peg.space = peg.S'\n\t '
peg.ws0 = peg.space^0
peg.ws = peg.space^1

local singleLineComment = '//' * peg.After('\n')
local multiLineComment = '/*' * peg.After('*/')

local remove_comments = peg.Cs(((singleLineComment + multiLineComment) / '' + 1)^0)

local format = string.format

local print_error = function(s) io.stderr:write('\27[31m', s, '\27[m') end

local content_by_file = {}

return {
    peg = peg,

    println_error=function(s)
        print_error(s)
        print_error('\n')
    end,

    print_error=print_error,

    readall=function(fname)
        local s = content_by_file[fname]
        if s then
            return s
        end

        local f,e = io.open(fname)
        if e then
            error(e)
        end
        s = f:read('*a')
        f:close()

        s = remove_comments:match(s)
        content_by_file[fname] = s
        return s
    end,


    -- checker utility

    copy_setk=function(from, to)
        for k,b in pairs(from) do
            if b then
                to[k] = b
            end
        end
    end,

    match_and_setk=function(pattern, content, value, kvalues)
        local kvalues = kvalues or {}
        local r = pattern:match(content)
        if r then
            for _,v in ipairs(r) do
                kvalues[v] = value
            end
        end
        return kvalues
    end,

    print_kvalues=function(kvalues, name)
        print_error(format('#%s = %d\n', name or 'values', #kvalues))
        for name,activated in pairs(kvalues) do
            print_error(format('%s = %s\n', name, activated))
        end
    end,

    count_error=function(kvalues, fmt)
        local t = {}
        for name,activated in pairs(kvalues) do
            if not activated then
                t[#t + 1] = fmt:format(name)
            end
        end
        if #t ~= 0 then
            t[#t+1] = ''
            print_error(table.concat(t,'\n'))
        end
        return #t
    end,
}
