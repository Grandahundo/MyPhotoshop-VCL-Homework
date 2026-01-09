add_rules("mode.debug", "mode.release")

add_requires("imgui", {configs = {glfw = true, opengl3 = true}})
add_requires("glfw")
add_requires("opengl", {system = true})
add_requires("glad")

target("PaintApp")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("imgui", "glfw", "opengl", "glad")
    set_languages("c++17")