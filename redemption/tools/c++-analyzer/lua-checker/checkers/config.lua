local utils = require'utils'

local patternType, patternTypeSearch, patternVar, patternVarSearch
do
    local peg = utils.peg
    local C = peg.C
    local R = peg.R
    local S = peg.S
    local P = peg.P
    local Ct = peg.Ct
    local After = peg.After
    local Ident = (peg.word + '::')^1

    patternVar = Ct(After(Ct(
        'struct ' * C(Ident)
      * ' {\n        static constexpr unsigned acl_proxy_communication_flags = ' * C(peg.Until(';'))
    ))^0)

    patternVarSearch = Ct(After(
        Ct(C(P'get_mutable_ref' + 'get' + 'set_acl' + 'set' + 'update_acl' + 'update' + 'ask' + 'send')
        * '<cfg::' * C(Ident) * S'>')
      + (P'to_bgr' + 'has_field') * '(cfg::' * C(Ident) * (P'()' + '{}') * ')'
    )^0)

    patternType = Ct((After('.enumeration_') * After('"') * C(peg.Until('"')))^0)
    patternTypeSearch = Ct(After((1-peg.wordchars) * C((R'AZ' * R('AZ','az')^1)))^0)
end

local config_type_path = 'projects/redemption_configs/configs_specs/configs/specs/config_type.hpp'
local variables_configuration_path = 'projects/redemption_configs/autogen/include/configs/autogen/variables_configuration.hpp'

local ask_only = {
    -- TODO check that
    ['context::selector']=true,
    ['context::password']=true,
}

local match_and_setk = utils.match_and_setk

-- {used:bool, acl_to_proxy:bool, proxy_to_acl:bool, get:bool, set:bool, ask:bool}
local values = {}
local types

function init(args)
    types = match_and_setk(patternType, utils.readall(config_type_path), false)
    types['OcrLocale'] = true
    types['ColorDepth'] = true
    types['KeyboardLogFlagsCP'] = true
    types['VncTunnelingType'] = true
    types['VncTunnelingCredentialSource'] = true

    for _,v in ipairs(patternVar:match(utils.readall(variables_configuration_path))) do
        values[v[1]] = {
            used=false,
            is_acl_to_proxy = v[2]:byte(3) == 49 --[[ '1' ]],
            is_proxy_to_acl = v[2]:byte(2) == 49 --[[ '1' ]],
        }
    end
end

function file(content, filename)
    local r = patternVarSearch:match(content)
    local t
    if r then
        for _,v in ipairs(r) do
            if v[1] then
                t = values[v[2]]
                if not t then
                    if v[2] ~= 'debug::name' or filename ~= 'src/core/session.cpp' then
                        utils.print_error("unknown cfg variable: '" .. v[2]
                                        .. "' in " .. filename .. '\n')
                        return 1
                    end
                    t = {}  -- ignore error
                end
                t.used = true
                if v[1] == 'get' then
                    t.get = true
                elseif v[1] == 'ask' then
                    t.ask = true
                elseif v[1] == 'send' then
                    t.get = true
                    t.set = true
                elseif v[1] ~= 'get_mutable_ref' then
                    t.set = true
                end
            else
                t = values[v]
                t.used = true
                t.get = true
            end
        end
    end
    match_and_setk(patternTypeSearch, content, true, types)
end

function terminate()
    local errors = {}
    for name,t in pairs(values) do
        if not t.used then
            errors[#errors+1] = name .. " not used"
        else
            if not t.get and t.is_acl_to_proxy then
                if not ask_only[name] or not t.ask then
                    local real_type = t.is_acl_to_proxy and t.is_proxy_to_acl
                        and 'acl_rw'
                        or 'acl_to_proxy'
                    errors[#errors+1] = 'cfg::' .. name .. " is " .. real_type .. " but never read (should be proxy_to_acl)"
                end
            end
            if not t.set and t.is_proxy_to_acl then
                errors[#errors+1] = 'cfg::' .. name .. " is proxy_to_acl but never write (should be acl_to_proxy)"
            end
        end
    end
    if #errors ~= 0 then
        errors[#errors+1] = ''
        utils.print_error(table.concat(errors,'\n'))
    end
    return utils.count_error(types, "cfg::%s not used") + #errors
end

return {init=init, file=file, terminate=terminate}
