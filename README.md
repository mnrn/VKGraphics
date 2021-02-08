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
- [GLI]
- [assimp]
- [imgui]
- [nlohmann-json]
- [fmt]
- [spdlog]
- [boost]
  
リポジトリのルートディレクトリにCMakeLists.txtがあるので詳しくはそちらを参照ください。  

## Features

### 物理ベースレンダリング (Physically Based Rendering)

![PBR](https://github.com/mnrn/ReVK/blob/main/Docs/Images/pbr.png)

BRDFによるMicrofacet Modelの描画を行っています。  
このあたりの理論はGoogleの物理ベースレンダリングエンジンFilamentのドキュメントなどを参考にしました。  
左が金属(Metallic Material)、右が非金属(Dielectric Material)です。

GUIの使用を少し変更したこともあり、細かいところは[移植元](https://github.com/mnrn/ReGL)から移植していませんが準備はしています。  
また、IBLなどはいまのところ実装しておりません。

---

### 遅延シェーディング (Deferred Shading)

![Deferred](https://github.com/mnrn/ReVK/blob/main/Docs/Images/deferred.png)

2次元の画面空間上でシェーディングを行う手法です。
半透明をうまく扱えない点やメモリを大きく消費する点などからゲームではあまり使われないかもしれません。
大量のライトを使う場合は候補に入れても良いかもしれません。

<img src="https://github.com/mnrn/ReVK/blob/main/Docs/Images/deferred_position.png" width=500>

位置情報になります。

<img src="https://github.com/mnrn/ReVK/blob/main/Docs/Images/deferred_normal.png" width=500>

法線情報になります。

<img src="https://github.com/mnrn/ReVK/blob/main/Docs/Images/deferred_albedo.png" width=500>

色情報になります。

## 参考

[OpenGL 4 Shading Language Cookbook - Third Edition](https://www.packtpub.com/product/opengl-4-shading-language-cookbook-third-edition/9781789342253)  
[HLSL Development Cookbook](https://www.packtpub.com/product/hlsl-development-cookbook/9781849694209)  
[Unity 2018 Shaders and Effects Cookbook - Third Edition](https://www.packtpub.com/product/unity-2018-shaders-and-effects-cookbook-third-edition/9781788396233)  
[Physically Based Rendering in Filament](https://google.github.io/filament/Filament.md.html)  
[Advanced-Lighting - SSAO](https://learnopengl.com/Advanced-Lighting/SSAO)  
[Cascaded Shadow Maps](https://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf)  
[GPU Gems3 Chapter 10. Parallel-Split Shadow Maps on Programmable GPUs](https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus)  
[Examples and demos for the new Vulkan API](https://github.com/SaschaWillems/Vulkan)

[GLFW]:<https://www.glfw.org/>
[GLM]:<https://github.com/g-truc/glm>
[GLI]:<https://github.com/g-truc/gli>
[assimp]:<https://github.com/assimp/assimp>
[imgui]:<https://github.com/ocornut/imgui>
[nlohmann-json]:<https://github.com/nlohmann/json>
[fmt]:<https://github.com/fmtlib/fmt>
[spdlog]:<https://github.com/gabime/spdlog>
[boost]:<https://www.boost.org/>

[glad]:<https://github.com/Dav1dde/glad>
[stb]:<https://github.com/nothings/stb>
[tinyobjloader]:<https://github.com/tinyobjloader/tinyobjloader>
[freetype]:<https://www.freetype.org/>
