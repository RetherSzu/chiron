-- =============================================================================
-- Chiron
-- Build Configuration
--
-- Taken from Gamegine's xmake.lua (branch main): same language, modes,
-- sanitizers and warning rules. Each subject is a top-level directory
-- ("graphic-programming"), named by its first word ("graphic"). Each training
-- is a directory in it whose name starts with its number ("01-training");
-- each .cpp in that is an exercise with its own main(), built as a binary of
-- its own.
--
--   xmake run graphic 01 a     build and run graphic's "01-training/a.cpp"
--   xmake run graphic 02 c1    build and run graphic's "02-training/c1.cpp"
--   xmake run graphic 01       list the exercises of graphic's training 01
--   xmake run graphic          list every training of graphic
--   xmake run graphic-01-a     the same as 'xmake run graphic 01 a'
--   xmake                      build every exercise
-- =============================================================================

set_project("chiron")
set_version("0.1.0")
set_description("C++ engine and systems programming trainings")

set_languages("cxx23")

-- Writes compile_commands.json at the root for clangd, as Gamegine does.
add_rules("plugin.compile_commands.autoupdate",
          {outputdir = ".", lsp = "clangd"})

add_rules("mode.debug", "mode.release")

-- 'xmake run' builds the exercise before running it.
set_policy("run.autobuild", true)

-- Formatting and static analysis, as in Gamegine:
--   xmake format        clang-format (.clang-format), then the BDE pass
--   xmake check-format  check that 'xmake format' has nothing left to do
--   xmake tidy          clang-tidy (.clang-tidy) on every exercise
includes("tools/tasks.lua")


-- =============================================================================
-- Sanitizers Configuration
--
-- Can be enabled via dedicated modes:
--   xmake f -m asan  (AddressSanitizer + UndefinedBehaviorSanitizer)
--   xmake f -m ubsan (UndefinedBehaviorSanitizer)
--   xmake f -m tsan  (ThreadSanitizer: try it on 01 c2, the data race)
--
-- Or via individual configuration flags:
--   xmake f --asan=y
--   xmake f --ubsan=y
--   xmake f --tsan=y

option("asan")
    set_default(false)
    set_showmenu(true)
    set_category("sanitizers")
    set_description("Enable AddressSanitizer (ASAN)")
option_end()

option("ubsan")
    set_default(false)
    set_showmenu(true)
    set_category("sanitizers")
    set_description("Enable UndefinedBehaviorSanitizer (UBSAN)")
option_end()

option("tsan")
    set_default(false)
    set_showmenu(true)
    set_category("sanitizers")
    set_description("Enable ThreadSanitizer (TSAN)")
option_end()

if is_mode("asan") or has_config("asan") then
    set_policy("build.sanitizer.address", true)
    set_symbols("debug")
    set_optimize("none")
end

if is_mode("ubsan") or has_config("ubsan") then
    set_policy("build.sanitizer.undefined", true)
    set_symbols("debug")
    set_optimize("none")
end

if is_mode("tsan") or has_config("tsan") then
    set_policy("build.sanitizer.thread", true)
    set_symbols("debug")
    set_optimize("none")
end


-- =============================================================================
-- Compiler Flags
--
-- The 'tools' filter names the compiler by xmake's tool name, and xmake calls
-- 'g++' "gxx", not "gcc": without "gxx" the filter silently drops every flag
-- of these rules under GCC.

rule("training.warnings")
    on_load(function (target)
        target:add("cxxflags",
            -- Enable common / standard warning diagnostics.
            "-Wall",
            -- Enable additional warning diagnostics not covered by -Wall.
            "-Wextra",
            -- Enforce strict ISO C++ compliance and reject non-standard extensions.
            "-Wpedantic",
            -- Warn when a variable shadows another variable in an outer scope.
            "-Wshadow",
            -- Warn if a class with virtual functions lacks a virtual destructor.
            "-Wnon-virtual-dtor",
            -- Warn on C-style casts in C++.
            "-Wold-style-cast",
            -- Warn when pointer casting increases required memory alignment.
            "-Wcast-align",
            -- Warn on unused variables, functions, parameters, or expressions.
            "-Wunused",
            -- Warn when a function hides a virtual function from a base class.
            "-Woverloaded-virtual",
            -- Warn on implicit type conversions that may alter values or lose precision.
            "-Wconversion",
            -- Warn on implicit conversions between signed and unsigned types.
            "-Wsign-conversion",
            {tools = {"clang", "gcc", "gxx", "apple_clang"}}
        )
    end)
rule_end()

-- Errors are values, as in Gamegine: no code here throws or catches, and
-- '-fno-exceptions' makes that a compile error rather than a convention.
rule("training.no_exceptions")
    on_load(function (target)
        target:add("cxxflags", "-fno-exceptions",
                   {tools = {"clang", "gcc", "gxx", "apple_clang"}})
    end)
rule_end()

rule("training.werror")
    on_load(function (target)
        target:add("cxxflags",
            -- Treat all warnings as compilation errors.
            "-Werror",
            {tools = {"clang", "gcc", "gxx", "apple_clang"}}
        )
    end)
rule_end()


-- =============================================================================
-- Targets: one binary per exercise, one runner per subject
--
-- A subject is a top-level directory that holds numbered trainings. Its name
-- is the first word of the directory's name: "graphic-programming" is
-- "graphic". "<subject>-<nn>-<exercise>" is the binary of one .cpp.
-- "<subject>" is a phony runner: 'xmake run <subject> <nn> <exercise>' builds
-- that one exercise, then runs it with the rest of the arguments. The runner
-- depends on no exercise, so one that does not build never stops the others
-- from running. tools/lua/sources.lua finds the trainings the same way.

-- Return the training directories directly inside 'dir': those whose name
-- starts with a number.
local function trainings_in(dir)
    local result = {}
    for _, d in ipairs(os.dirs(path.join(dir, "*"))) do
        if path.filename(d):match("^%d+") then
            table.insert(result, d)
        end
    end
    return result
end

local subjects = {}  -- subject name -> directory name
local clashes  = {}  -- one message per directory that reuses a subject name
for _, dir in ipairs(os.dirs(path.join(os.scriptdir(), "*"))) do
    local folder   = path.filename(dir)
    local subject  = folder:match("^[^-]+")
    local training = trainings_in(dir)
    local ours     = not folder:startswith(".") and #training > 0
    if ours and subjects[subject] then
        table.insert(clashes,
            ("subjects '%s' and '%s' would both be named '%s'; rename one "
             .. "of them"):format(subjects[subject], folder, subject))
    elseif ours then
        subjects[subject] = folder

        for _, tdir in ipairs(training) do
            local number = path.filename(tdir):match("^(%d+)")
            for _, file in ipairs(os.files(path.join(tdir, "*.cpp"))) do
                target(subject .. "-" .. number .. "-" .. path.basename(file))
                    set_kind("binary")
                    add_files(file)
                    add_rules("training.warnings", "training.werror",
                              "training.no_exceptions")
                target_end()
            end
        end

        target(subject)
            set_kind("phony")
            set_default(false)

            on_run(function (target)
                import("core.base.option")
                import("core.base.task")
                import("core.project.project")

                -- The subject's exercises, by training: {["01"] = {"a"}}.
                local subject   = target:name()
                local prefix    = subject .. "-"
                local exercises = {}
                local numbers   = {}
                for name, _ in pairs(project.targets()) do
                    if name:startswith(prefix) then
                        local number, exercise =
                            name:sub(#prefix + 1):match("^(%d+)%-(.+)$")
                        if number then
                            if not exercises[number] then
                                exercises[number] = {}
                                table.insert(numbers, number)
                            end
                            table.insert(exercises[number], exercise)
                        end
                    end
                end
                table.sort(numbers)
                for _, number in ipairs(numbers) do
                    table.sort(exercises[number])
                end

                local function show(number)
                    cprint("${bright}%s %s${clear}: %s", subject, number,
                           table.concat(exercises[number], ", "))
                end

                local args     = option.get("arguments") or {}
                local number   = args[1]
                local exercise = args[2]
                if not number then
                    for _, n in ipairs(numbers) do
                        show(n)
                    end
                    cprint("usage: xmake run %s <training> <exercise> "
                           .. "[args...]", subject)
                    return
                end
                if not exercises[number] then
                    raise("%s has no training '%s' (it has: %s)", subject,
                          number, table.concat(numbers, ", "))
                end
                if not exercise then
                    show(number)
                    cprint("usage: xmake run %s %s <exercise> [args...]",
                           subject, number)
                    return
                end

                local binary =
                    project.target(prefix .. number .. "-" .. exercise)
                if not binary then
                    raise("%s %s has no exercise '%s' (it has: %s)", subject,
                          number, exercise,
                          table.concat(exercises[number], ", "))
                end

                task.run("build", {target = binary:name()})
                os.execv(binary:targetfile(), table.slice(args, 3))
            end)
        target_end()
    end
end

-- The description scope has no raise(), error() or assert(), so a name clash
-- is reported by a target that fails as soon as the project loads.
if #clashes > 0 then
    target("subject-name-clash")
        set_kind("phony")
        set_values("clashes", clashes)
        on_load(function (target)
            raise(table.concat(table.wrap(target:values("clashes")), "\n"))
        end)
    target_end()
end
