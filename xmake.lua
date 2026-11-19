set_xmakever("3.0.0")

includes("../CommonLibRE5")

set_project("Vocalizer")
set_version("2.0.0")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")

target("Vocalizer")
    add_rules("commonlibre5.plugin", {
        name = "Vocalizer",
        author = "LuBuCake",
    })
    set_symbols("debug")
    add_files("src/**.cpp")
