-- SPDX-License-Identifier: Apache-2.0
-- Copyright (c) 2026 BuzzY_ & Rether
--
-- formatter.lua -- the formatting pipeline: clang-format, then bdestyle.
--
-- Taken from gamegine's ci/lua/formatter.lua (branch main). One change: the
-- clang-format to run is found by find() rather than taken from PATH, because
-- the llvm@<n> keg that supplies clang++ and clangd here is first on PATH and
-- brings its own, older clang-format, while the pinned major is installed
-- beside it by Homebrew's 'clang-format' formula.
--
-- 'xmake format' and 'xmake check-format' (check and --fix) both go through
-- format_text(), so what is fixed locally is exactly what the check checks.
-- clang-format alone undoes the BDE tags and banners (see bdestyle.lua):
-- the editor's format-on-save does that while you work, and 'xmake format'
-- puts them back.

import("bdestyle")
import("probe")

-- The clang-format major version the sources are formatted with, the same
-- as gamegine's. Two versions disagree on layout (18 and 23 indent the
-- continuation line of a wrapped call differently, for one), so a mismatched
-- tool reports files as "needs formatting" that are fine.
local CLANG_FORMAT_MAJOR = 23

-- Where clang-format is looked for, in order: PATH, then the Homebrew
-- 'clang-format' formula, then Homebrew's unversioned llvm keg.
local CANDIDATES = {
    "clang-format",
    "/opt/homebrew/opt/clang-format/bin/clang-format",
    "/usr/local/opt/clang-format/bin/clang-format",
    "/opt/homebrew/opt/llvm/bin/clang-format",
    "/usr/local/opt/llvm/bin/clang-format",
}

-- The clang-format chosen by find(), once found.
local chosen = nil

-- Return the clang-format major version the sources are formatted with.
-- (A function: an xmake module exports its functions, not its variables.)
function required_major()
    return CLANG_FORMAT_MAJOR
end

-- Return nil if the specified 'version' (the text after "version" in
-- 'clang-format --version', such as "23.1.0" or "18.1.3") is the major
-- version the sources are formatted with, and otherwise a message saying
-- what is wrong and how to fix it.
function version_problem(version)
    local major = tonumber((version or ""):match("^(%d+)"))
    if major == CLANG_FORMAT_MAJOR then
        return nil
    end
    return ("clang-format %s found, but the sources are formatted with "
         .. "clang-format %d, and other versions lay code out differently. "
         .. "Install it with 'brew install clang-format' or "
         .. "'pip install clang-format==%d.*'."):format(
             tostring(version), CLANG_FORMAT_MAJOR, CLANG_FORMAT_MAJOR)
end

-- Return the clang-format to run and its version: the first candidate of
-- the required major or, if none is, the first that runs at all, so that
-- version_problem() names what was found. Return nil and a reason if no
-- candidate runs.
function find()
    if chosen then
        return chosen.program, chosen.version
    end
    local fallback = nil
    local reasons = {}
    for _, candidate in ipairs(CANDIDATES) do
        local record = probe.match(candidate, {"--version"},
                                   "version%s+([%d%.]+)")
        if record.status == "OK" then
            local found = {program = candidate, version = record.value}
            if version_problem(record.value) == nil then
                chosen = found
                return found.program, found.version
            end
            fallback = fallback or found
        else
            table.insert(reasons, candidate .. ": " .. record.reason)
        end
    end
    if fallback then
        return fallback.program, fallback.version
    end
    return nil, table.concat(reasons, "; ")
end

-- Format 'text' as the contents of 'filename' and return the result, or
-- nil and a reason. 'filename' only chooses the language by extension; it
-- does not have to exist, and it is never read or written.
function format_text(text, filename)
    local program, version = find()
    if not program then
        return nil, "clang-format is not available: " .. version
    end
    local extension = path.extension(filename or "")
    if extension == "" then
        extension = ".cpp"
    end
    local input = os.tmpfile() .. extension
    local output = os.tmpfile()
    io.writefile(input, text)
    local style = "--style=file:" .. path.join(os.projectdir(), ".clang-format")
    local code = os.execv(program, {style, input},
                          {stdout = output, try = true})
    local formatted = code == 0 and io.readfile(output) or nil
    os.tryrm(input)
    os.tryrm(output)
    if formatted == nil then
        return nil, "clang-format failed (exit " .. tostring(code) .. ")"
    end
    return bdestyle.apply(formatted)
end

-- Return the problems in 'text' that formatting cannot fix, each as
-- {line = <number>, text = <the line>, message = <what is wrong>}: the naming
-- rule of bdestyle, which a person has to apply because a rename touches the
-- uses too. 'text' is what format_text() returns, so the line numbers are
-- those of the file once 'xmake format' has run.
function problems(text)
    return bdestyle.problems(text)
end
