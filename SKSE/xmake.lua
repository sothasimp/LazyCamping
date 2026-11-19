-- Wayfarer SKSE plugin (LazyCamping.dll)
-- Build: xmake config -m release && xmake
set_xmakever("2.8.2")

add_rules("mode.debug", "mode.release")

includes("lib/commonlibsse-ng")

set_project("LazyCamping")
set_version("1.0.0")
set_license("GPL-3.0")

set_languages("c++23")
set_warnings("allextra")

add_requires("simpleini v4.25")

target("LazyCamping")
    add_deps("commonlibsse-ng")
    add_packages("simpleini")

    add_rules("commonlibsse-ng.plugin", {
        name = "LazyCamping",
        author = "Wayfarer - A Convenient Camping Framework",
        description = "Native camp placement for Wayfarer - A Convenient Camping Framework"
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    -- SKSE Menu Framework's API header (optional at runtime; see its README).
    add_includedirs("extern/SKSEMenuFramework")
    add_defines("_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING")
    -- windows.h (through CommonLib) otherwise defines min and max macros.
    add_defines("NOMINMAX")
    set_pcxxheader("src/PCH.h")

    -- Copy the built DLL into a ready-to-zip mod folder.
    after_build(function(target)
        local out = path.join(os.projectdir(), "dist", "SKSE", "Plugins")
        os.mkdir(out)
        os.cp(target:targetfile(), out)
        os.cp(path.join(os.projectdir(), "Plugins", "LazyCamping.ini"), out)
        if os.isfile(target:symbolfile()) then
            os.cp(target:symbolfile(), out)
        end
    end)

-- Validate INI values without loading Skyrim or the SKSE plugin.
target("settings-tests")
    set_kind("binary")
    set_default(false)
    add_files("tests/SettingsValues.cpp")
