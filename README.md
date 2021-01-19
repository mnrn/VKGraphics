# Vulkan グラフィックメモ

[![License: MIT](https://img.shields.io/badge/License-MIT-lightgrey.svg)](https://opensource.org/licenses/MIT)
![Launguage-C++](https://img.shields.io/badge/Language-C%2B%2B-orange)

グラフィックに関する実装をVulkanで行っていきます。  

## 制作経緯

もともとはOpenGLで動かしていたプログラムですが、

- デバッグをOpenGLよりも楽にしたい点
- もっと低レイヤーな部分に触れたい点
- HLSLが使用したい点
- マルチプラットフォームが可能な点(DirectXではない理由)
- DirectX12よりも難しいと言われている点
  
などからVulkanへ移行することにしました。

## 制作環境

現時点では VULKAN_SDK が環境変数として設定されていないとビルドができないことに注意してください。  

MoltenVKでMacOSXのみの動作確認になります。後にWindowsとLinuxにも対応するつもりです。  
AndroidやiOSの対応は未定になります。

- OS
  - Mac OSX Big Sur 11.1

## ビルド

ビルドツールはCMakeになります。  

グラフィックライブラリはVulkanでバージョン1.0以降がターゲットになります。  
したがって、Vulkanはインストール済みであることが想定されることに注意してください。  
インストールしていない場合は[こちら](https://vulkan.lunarg.com/)からダウンロードができます。

以下はリポジトリにすでにおいてあるのでインストールがされてなくても大丈夫です。

- [GLFW]
- [GLM]
- [boost]
- [fmt]
- [stb]
- [tinyobjloader]
- [freetype]
- [spdlog]
- [imgui]
- [nlohmann-json]

リポジトリのルートディレクトリにCMakeLists.txtがあるので詳しくはそちらを参照ください。  

## 参考

[OpenGL 4 Shading Language Cookbook - Third Edition](https://www.packtpub.com/product/opengl-4-shading-language-cookbook-third-edition/9781789342253)  
[HLSL Development Cookbook](https://www.packtpub.com/product/hlsl-development-cookbook/9781849694209)  
[Unity 2018 Shaders and Effects Cookbook - Third Edition](https://www.packtpub.com/product/unity-2018-shaders-and-effects-cookbook-third-edition/9781788396233)  
[Physically Based Rendering in Filament](https://google.github.io/filament/Filament.md.html)

[boost]:<https://www.boost.org/>
[GLFW]:<https://www.glfw.org/>
[glad]:<https://github.com/Dav1dde/glad>
[GLM]:<https://github.com/g-truc/glm>
[fmt]:<https://github.com/fmtlib/fmt>
[stb]:<https://github.com/nothings/stb>
[tinyobjloader]:<https://github.com/tinyobjloader/tinyobjloader>
[freetype]:<https://www.freetype.org/>
[spdlog]:<https://github.com/gabime/spdlog>
[imgui]:<https://github.com/ocornut/imgui>
[nlohmann-json]:<https://github.com/nlohmann/json>
