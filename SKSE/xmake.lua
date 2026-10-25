-- LazyCamping SKSE plugin
-- Build: xmake config -m release && xmake
set_xmakever("2.8.2")

includes("lib/commonlibsse-ng")

set_project("LazyCamping")
set_version("1.0.0")
set_license("GPL-3.0")

set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.release")

target("LazyCamping")
    add_deps("commonlibsse-ng")

    add_rules("commonlibsse-ng.plugin", {
        name = "LazyCamping",
        author = "LazyCamping",
        description = "Native ground placement for the Lazy Camping power"
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/PCH.h")

    -- Copy the built DLL into a ready-to-zip mod folder.
    after_build(function(target)
        local out = path.join(os.projectdir(), "dist", "SKSE", "Plugins")
        os.mkdir(out)
        os.cp(target:targetfile(), out)
        if os.isfile(target:symbolfile()) then
            os.cp(target:symbolfile(), out)
        end
    end)
