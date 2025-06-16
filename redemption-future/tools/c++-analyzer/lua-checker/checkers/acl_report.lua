local utils = require'utils'

local report_ids = {}
local report_funcs = {}
local report_has_params = {}
local sesman_by_id = {}
local verbose = false

function init(args)
    verbose = args == 'verbose'
    local i = 0

    -- extracte id declared in cpp
    for id, func, param in utils.readall('src/acl/acl_report.hpp')
        :gmatch('\n    f%(([_A-Z0-9]+),[ \\,\n]+([_a-zA-Z0-9]+)[ \\,\n]+(.)')
    do
        i = i + 1
        report_ids[id] = 0
        report_funcs[func] = 0
        report_has_params[id] = param == 'P'
        if verbose then
            utils.print_error('acl_report.hpp: id: ' .. id .. ' | '
                              .. func .. (report_has_params[id] and '(param)\n' or '()\n'))
        end
    end

    assert(i > 10) -- random magic number for check detection

    -- extracte id used in sesman
    local body_func = utils.readall('tools/sesman/sesmanworker/sesman.py')
        :match('def handle_reporting%(.-\n    def')
    for id in body_func:gmatch('==%s*[\'"]([_A-Z0-9]+)[\'"]')
    do
        sesman_by_id[id] = body_func
            :match('[\'"]' .. id .. '.-\n        [^ ]')
            :match('[^%w]message[^%w]') and 1 or 0 -- 1 or 0 for concatenation with verbose
        if verbose then
            utils.print_error('sesman.py: ' .. id .. ' | has message: ' .. sesman_by_id[id] .. '\n')
        end
    end
end

function file(content, filename)
    for func in content:gmatch('AclReport::([_a-zA-Z0-9]+)%(') do
        if verbose then
            utils.print_error('found AclReport::' .. func .. '(...)\n')
        end
        report_funcs[func] = report_funcs[func] + 1
    end
end

function extract_unreferenced(t)
    local unreferenced_list = {}
    for k,v in pairs(t) do
        if v == 0 then
            table.insert(unreferenced_list, k)
        end
    end
    return unreferenced_list
end

function terminate()
    local not_found = {}
    local unused_message = {}
    local missing_message = {}
    for k, has_message in pairs(sesman_by_id) do
        if not report_ids[k] then
            utils.print_error('AclReport::' .. k .. '() not used\n')
        else
            report_ids[k] = 1
            if (has_message == 1) ~= report_has_params[k] then
                table.insert(has_message == 1 and missing_message or unused_message, k)
            end
        end
    end

    local not_referenced_by_sesman = extract_unreferenced(report_ids)
    local unused_acl_funcs = extract_unreferenced(report_funcs)

    local nb_error = 0
    for _, t in pairs({
        {'AclReport::', unused_acl_funcs, '() not used\n'},
        {'sesman refer an unknown AclReport::ReasonId::', not_found, '\n'},
        {'AclReport::ReasonId::', not_referenced_by_sesman, ' is not used in sesman.py\n'},
        {'unused message variable for ', unused_message, ' in sesman\n'},
        {'sesman uses a message not initialized by AclReport for ', missing_message, '\n'},
    }) do
        for _,k in pairs(t[2]) do
            utils.print_error(t[1] .. k .. t[3])
            nb_error = nb_error + 1
        end
    end

    return nb_error
end

return {init=init, file=file, terminate=terminate}
