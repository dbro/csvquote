READ_BUFFER_SIZE = 4096
NON_PRINTING_FIELD_SEPARATOR = string.char(0x1F)
NON_PRINTING_RECORD_SEPARATOR = string.char(0x1E)

function restore (delimiter, recordsep, c)
    if c == NON_PRINTING_FIELD_SEPARATOR then c = delimiter
    elseif c == NON_PRINTING_RECORD_SEPARATOR then c = recordsep
    end
    return c
end

do
    -- maintain the state of quoting inside this function
    -- this is OK because we need to read the file
    -- sequentially (not in parallel) because the state
    -- at any point depends on all of the previous data
    local isQuoteInEffect = false
    local isMaybeEscapedQuoteChar = false
    function sanitize (delimiter, quotechar, recordsep, c)
        local c2 = c
        if (isMaybeEscapedQuoteChar) then
            if (c ~= quotechar) then
                -- this is the end of a quoted field
                isQuoteInEffect = false
            end
            isMaybeEscapedQuoteChar = false
        elseif (isQuoteInEffect) then
            if (c == quotechar) then
                -- this is either an escaped quote char or the end of a quoted
                -- field. need to read one more character to decide which
                isMaybeEscapedQuoteChar = true
            elseif (c == delimiter) then
                c2 = NON_PRINTING_FIELD_SEPARATOR
            elseif (c == recordsep) then
                c2 = NON_PRINTING_RECORD_SEPARATOR
            end
        else
            -- quote not in effect
            if (c == quotechar) then
                isQuoteInEffect = true
            end
        end
        --print("santized " .. c .." -> " .. c2)
        return c2
    end
end

delimiter = ","
quotechar = '"'
recordsep = "\n"
operation_mode = nil -- default
-- default action is to sanitize
action_function = function(c) return sanitize(delimiter, quotechar, recordsep, c) end
if operation_mode == "restore" then
    action_function = function(c) return restore(delimiter, recordsep, c) end
end

s = io.read(READ_BUFFER_SIZE)
while s do
    -- this routine builds a new string, one character at a time
    --local s2, _ = s:gsub(".", action_function)
    --io.write(s2)
    for i=1, #s do
        local c = s:sub(i,i)
        local c2 = action_function(c)
        if c2 ~= c then
            -- only replace characters that get modified
            s = s:sub(1,i-1) .. c2 .. s:sub(i+1)
        end
    end
    io.write(s)
    s = io.read(READ_BUFFER_SIZE)
end
