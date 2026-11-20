-- SPDX-License-Identifier: Apache-2.0
-- Copyright (c) 2026 BuzzY_ & Rether
--
-- sources.lua -- enumerate the sources the format and tidy tasks police.
--
-- Taken from gamegine's ci/lua/sources.lua (branch main), with the roots
-- changed: here they are the trainings, found the way xmake.lua finds them.

local EXTENSIONS = {"**.cpp", "**.h", "**.hpp", "**.mm", "**.inl"}

-- Translation units only: what clang-tidy is run on.
local TRANSLATION_UNITS = {"**.cpp", "**.mm"}

-- Return the paths, relative to the project root, of the trees that are
-- ours: every training, that is every directory whose name starts with its
-- number, inside a top-level subject directory
-- ("graphic-programming/01-training"), sorted.
function roots()
    local result = {}
    for _, subject in ipairs(os.dirs(path.join(os.projectdir(), "*"))) do
        local folder = path.filename(subject)
        if not folder:startswith(".") then
            for _, dir in ipairs(os.dirs(path.join(subject, "*"))) do
                local name = path.filename(dir)
                if name:match("^%d+") then
                    table.insert(result, path.join(folder, name))
                end
            end
        end
    end
    table.sort(result)
    return result
end

-- Return the absolute paths of every C++ source and header in the trainings.
function list(opt)
    opt = opt or {}
    local files = {}
    local seen  = {}
    for _, dir in ipairs(opt.roots or roots()) do
        local absolute = path.join(os.projectdir(), dir)
        if os.isdir(absolute) then
            for _, pattern in ipairs(opt.extensions or EXTENSIONS) do
                for _, f in ipairs(os.files(path.join(absolute, pattern))) do
                    if not seen[f] then
                        seen[f] = true
                        table.insert(files, f)
                    end
                end
            end
        end
    end
    table.sort(files)
    return files
end

-- Return the absolute paths of every translation unit in the trainings.
function translation_units(opt)
    opt = opt or {}
    return list({roots = opt.roots, extensions = TRANSLATION_UNITS})
end
