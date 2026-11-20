-- SPDX-License-Identifier: Apache-2.0
-- Copyright (c) 2026 BuzzY_ & Rether
--
-- tasks.lua -- the format and lint entry points, exposed as xmake tasks.
--
-- Taken from gamegine's ci/tasks.lua (branch main): only the format and tidy
-- tasks, since the trainings have no CI of their own. Included from the root
-- xmake.lua.
--
--   xmake format         fix source formatting (overrides xmake's plugin)
--   xmake check-format   check, or fix, source formatting
--   xmake tidy           static analysis (clang-tidy), '--fix' to apply

-- ---------------------------------------------------------- check-format ---

task("check-format")
    set_category("plugin")
    set_menu {
        usage       = "xmake check-format [--fix]",
        description = "Check first-party sources against .clang-format "
                      .. "and the BDE rules it cannot express, and report "
                      .. "the naming rule no formatter can apply.",
        options = {
            {nil, "fix", "k", nil, "Rewrite files in place instead of "
                                   .. "reporting them."},
        }
    }
    on_run(function ()
        import("style", {rootdir = path.join(os.scriptdir(), "lua")})
        import("core.base.option")
        import("sources", {rootdir = path.join(os.scriptdir(), "lua")})
        -- clang-format, then the BDE pass for the rules clang-format cannot
        -- keep. The editor's format-on-save runs clang-format alone and
        -- undoes those; this puts them back.
        import("formatter", {rootdir = path.join(os.scriptdir(), "lua")})

        -- Not 'clang-format' from PATH: the llvm@<n> keg on PATH may carry
        -- another major than the pinned one (see formatter.lua).
        local program, found = formatter.find()
        if not program then
            raise("clang-format is not available: %s", found)
        end
        local version = {value = found}
        -- Checked before any file is: a different version reports files as
        -- unformatted that are fine, which reads as a problem in the code.
        local problem = formatter.version_problem(version.value)
        if problem then
            raise("%s", problem)
        end

        local files = sources.list()
        if #files == 0 then
            style.say("${warn}no first-party sources yet; nothing to "
                   .. "check")
            return
        end

        local fix = option.get("fix")
        local offenders = {}
        local naming = {}

        for _, f in ipairs(files) do
            local original = io.readfile(f)
            local text, reason = formatter.format_text(original, f)
            if text == nil then
                raise("%s: %s", path.relative(f, os.projectdir()), reason)
            end
            if text ~= original then
                if fix then
                    io.writefile(f, text)
                else
                    table.insert(offenders, f)
                end
            end
            -- Looked at in the formatted text, which is what the file holds
            -- once formatted. Nothing rewrites these, in either mode: a
            -- rename has to follow the uses.
            for _, problem in ipairs(formatter.problems(text)) do
                table.insert(naming, {file = f, problem = problem})
            end
        end

        if fix then
            style.say("${ok}formatted %d file(s): clang-format %s, then the "
                   .. "BDE pass", #files, version.value)
        end
        for _, f in ipairs(offenders) do
            style.say("${bad}needs formatting${reset} %s",
                   path.relative(f, os.projectdir()))
        end
        for _, n in ipairs(naming) do
            style.say("${bad}naming${reset} %s:%d: %s",
                   path.relative(n.file, os.projectdir()), n.problem.line,
                   n.problem.message)
        end

        local failures = {}
        if #offenders > 0 then
            table.insert(failures, ("%d of %d file(s) are not formatted; run "
                                    .. "'xmake format'"):format(#offenders,
                                                                #files))
        end
        if #naming > 0 then
            table.insert(failures, ("%d naming problem(s) to fix by hand: "
                                    .. "no formatter renames"):format(#naming))
        end
        if #failures > 0 then
            raise("%s", table.concat(failures, "; "))
        end
        if not fix then
            style.say("${ok}%d file(s) formatted correctly "
                   .. "(clang-format %s)", #files, version.value)
        end
    end)
task_end()

-- ---------------------------------------------------------------- format ---

-- Overrides xmake's built-in 'format' plugin, which runs clang-format ALONE
-- and would undo the BDE tags and banners just before a push. A project
-- task of the same name takes precedence (checked on xmake 3.1.1), so the
-- obvious command does the right thing.
task("format")
    set_category("plugin")
    set_menu {
        usage       = "xmake format",
        description = "Format first-party sources: clang-format, then the "
                      .. "BDE pass. Same as 'xmake check-format --fix'.",
        options = {}
    }
    on_run(function ()
        import("core.base.task")
        task.run("check-format", {fix = true})
    end)
task_end()


-- ------------------------------------------------------------------ tidy ---

task("tidy")
    set_category("plugin")
    set_menu {
        usage       = "xmake tidy",
        description = "Static analysis of first-party C++ (clang-tidy).",
        options = {
            {nil, "fix", "k", nil, "Apply clang-tidy's suggested fixes."},
        }
    }
    on_run(function ()
        import("style", {rootdir = path.join(os.scriptdir(), "lua")})
        import("core.base.option")
        import("probe",   {rootdir = path.join(os.scriptdir(), "lua")})
        import("sources", {rootdir = path.join(os.scriptdir(), "lua")})

        -- Homebrew keeps clang-tidy out of PATH because it would shadow
        -- Apple's clang; look there before giving up.
        local candidates = {
            "clang-tidy",
            "/opt/homebrew/opt/llvm/bin/clang-tidy",
            "/usr/local/opt/llvm/bin/clang-tidy",
        }
        local tidy
        for _, c in ipairs(candidates) do
            if probe.run(c, {"--version"}).status == "OK" then
                tidy = c
                break
            end
        end
        if not tidy then
            raise("clang-tidy not found. Tried: %s",
                  table.concat(candidates, ", "))
        end

        -- clang-tidy needs the real compile flags; xmake keeps
        -- compile_commands.json up to date at the project root.
        local database = path.join(os.projectdir(), "compile_commands.json")
        if not os.isfile(database) then
            raise("compile_commands.json is missing; run 'xmake build' first")
        end

        local units = sources.translation_units()
        if #units == 0 then
            style.say("${warn}no first-party translation units yet; "
                   .. "nothing to analyse")
            return
        end

        local failed = 0
        for _, unit in ipairs(units) do
            local relative = path.relative(unit, os.projectdir())
            local argv = {"--config-file="
                          .. path.join(os.projectdir(), ".clang-tidy"),
                          "-p", os.projectdir(), "--quiet"}
            if option.get("fix") then
                table.insert(argv, "--fix")
            end
            table.insert(argv, unit)

            -- allow_empty because clang-tidy prints nothing when clean;
            -- accept 1 so its findings arrive as output rather than as a
            -- failed probe, which is what lets them be reported per file.
            local record = probe.run(tidy, argv,
                                     {lines = true, stream = "both",
                                      allow_empty = true, accept = {0, 1},
                                      timeout = 300000})
            if record.status ~= "OK" then
                failed = failed + 1
                style.say("${bad}FAIL${reset} %s ${dim}(clang-tidy could not "
                          .. "run)${reset}", relative)
                style.say("${dim}       %s${reset}", record.reason or "")
            else
                -- A finding usually lives in a HEADER the unit included, not
                -- in the unit itself. Reporting only the unit sends the
                -- reader to the wrong file, so each finding names its own.
                local findings = {}
                for line in tostring(record.value):gmatch("[^\n]+") do
                    if line:find("error:") or line:find("warning:") then
                        table.insert(findings, line)
                    end
                end
                if #findings == 0 then
                    style.say("${ok}ok${reset}   %s", relative)
                else
                    failed = failed + 1
                    style.say("${bad}FAIL${reset} %s ${dim}(%d finding(s))"
                              .. "${reset}", relative, #findings)
                    for _, line in ipairs(findings) do
                        style.say("${dim}       %s${reset}", line)
                    end
                end
            end
        end

        if failed > 0 then
            raise("%d translation unit(s) have findings", failed)
        end
        style.say("${ok}%d translation unit(s) clean", #units)
    end)
task_end()
