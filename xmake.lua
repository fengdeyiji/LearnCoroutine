set_project("learn_coroutine")
set_version("1.0.0")
-- 设置使用 clang 编译器
set_toolset("cc", "clang")
set_toolset("cxx", "clang++")
set_toolset("ld", "clang++")
set_toolset("sh", "clang++")
-- 基于 LLVM 基本路径设置 include 路径和 lib 路径
llvm_prefix = "/root/.local/LLVM"
add_includedirs("src",
                llvm_prefix .. "/include/c++/v1",
                llvm_prefix .. "/include/aarch64-unknown-linux-gnu/c++/v1")
add_linkdirs(llvm_prefix .. "/lib/aarch64-unknown-linux-gnu")
-- 设置 C++ 标准库为 libc++
add_cxxflags("-std=c++23", "-stdlib=libc++", "-fmodules")
add_ldflags("-static", "-stdlib=libc++", "-nostdlib++", "-lc++", "-lc++abi", "-lpthread")


target("example0")
  -- 设置目标的种类，默认为 binary，表示编译成可执行文件
  set_kind("binary")
  -- 添加源文件
  add_files("src/example0.cpp")
  if is_mode("debug") then
    add_defines("DEBUG")
    add_cxflags("-O0", "-g3")
    set_targetdir("build_debug")
  elseif is_mode("release") then
    add_defines("NDEBUG")
    add_cxflags("-O3")
    set_targetdir("build_release")
  end
target("example1")
  -- 设置目标的种类，默认为 binary，表示编译成可执行文件
  set_kind("binary")
  -- 添加源文件
  add_files("src/example1.cpp")
  if is_mode("debug") then
    add_defines("DEBUG")
    add_cxflags("-O0", "-g3")
    set_targetdir("build_debug")
  elseif is_mode("release") then
    add_defines("NDEBUG")
    add_cxflags("-O3")
    set_targetdir("build_release")
  end
target("example2")
  -- 设置目标的种类，默认为 binary，表示编译成可执行文件
  set_kind("binary")
  -- 添加源文件
  add_files("src/example2.cpp")
  if is_mode("debug") then
    add_defines("DEBUG")
    add_cxflags("-O0", "-g3")
    set_targetdir("build_debug")
  elseif is_mode("release") then
    add_defines("NDEBUG")
    add_cxflags("-O3")
    set_targetdir("build_release")
  end
target("example3")
  -- 设置目标的种类，默认为 binary，表示编译成可执行文件
  set_kind("binary")
  -- 添加源文件
  add_files("src/example3.cpp")
  if is_mode("debug") then
    add_defines("DEBUG")
    add_cxflags("-O0", "-g3")
    set_targetdir("build_debug")
  elseif is_mode("release") then
    add_defines("NDEBUG")
    add_cxflags("-O3")
    set_targetdir("build_release")
  end
target("example4")
  -- 设置目标的种类，默认为 binary，表示编译成可执行文件
  set_kind("binary")
  -- 添加源文件
  add_files("src/example4.cpp")
  if is_mode("debug") then
    add_defines("DEBUG")
    add_cxflags("-O0", "-g3")
    set_targetdir("build_debug")
  elseif is_mode("release") then
    add_defines("NDEBUG")
    add_cxflags("-O3")
    set_targetdir("build_release")
  end
target("example5")
  -- 设置目标的种类，默认为 binary，表示编译成可执行文件
  set_kind("binary")
  -- 添加源文件
  add_files("src/example5.cpp")
  if is_mode("debug") then
    add_defines("DEBUG")
    add_cxflags("-O0", "-g3")
    set_targetdir("build_debug")
  elseif is_mode("release") then
    add_defines("NDEBUG")
    add_cxflags("-O3")
    set_targetdir("build_release")
  end
target("example6")
  -- 设置目标的种类，默认为 binary，表示编译成可执行文件
  set_kind("binary")
  -- 添加源文件
  add_files("src/example6.cpp")
  if is_mode("debug") then
    add_defines("DEBUG")
    add_cxflags("-O0", "-g3")
    set_targetdir("build_debug")
  elseif is_mode("release") then
    add_defines("NDEBUG")
    add_cxflags("-O3")
    set_targetdir("build_release")
  end