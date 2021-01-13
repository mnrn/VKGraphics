# Vulkan グラフィックメモ

[![License: MIT](https://img.shields.io/badge/License-MIT-lightgrey.svg)](https://opensource.org/licenses/MIT)
![Launguage-C++](https://img.shields.io/badge/Language-C%2B%2B-orange)

グラフィックに関する実装をVulkanで行っていきます。  

## 制作経緯

もともとはOpenGLで動かしていたプログラムですが、様々な理由からVulkanへ移植することにしました。

## 制作環境

現時点では VULKAN_SDK が環境変数として設定されていないとビルドができないことに注意してください。  

MoltenVKでMacOSXのみの動作確認になります。後にWindowsとLinuxにも対応するつもりです。  
AndroidやiOSの対応は未定になります。

- OS
  - Mac OSX Big Sur 11.1

## ビルド

ビルドツールはCMakeになります。  
グラフィックライブラリはVulkanでバージョン1.0以降がターゲットになります。  
以下はリポジトリにすでにおいてあるのでインストールがされてなくても大丈夫です。

- [GLFW]
  - OpenGLやOpenGL ES, Vulkan用のWindowやInputをどうこうしてくれるライブラリです。
- [GLM]
  - グラフィック用の数学ライブラリ(ヘッダオンリー)です。
- [boost]
  - C++の時期標準ライブラリ。今回はあまり使用していないため依存しない場合も考慮してもいいかもしれません。
- [fmt]
  - C++20のstd::formatの代用になります。今回はヘッダオンリーにしています。
- [stb]
  - 今回使用しているのは画像のローダーになります。
- [tinyobjloader]
  - objファイルの読み込みに使用します。
  - objファイルの簡潔さを考えると自作でも良かったかもしれませんが、今後の拡張性と保守性などを考えてこちらにしました。
- [freetype]
  - フォントライブラリ。フォントをビットマップデータにして描画します。
  - LinuxやBSD, iOSやAndroidはこのライブラリを用いているようです。
- [spdlog]
  - 便利なロガーライブラリです。主にデバッグで使います。
- [imgui]
  - パラメータの調整やデバッグを容易にしてくれるGUIライブラリです。
- [JSON for Modern C++]
  - ヘッダオンリーで使えるJSONライブラリです。現在の用途ではこれが一番用途にあってそうなのでこのライブラリを選択させていただきました。

リポジトリのルートディレクトリにCMakeLists.txtがあるので詳しくはそちらを参照ください。  

## 参考

[OpenGL 4 Shading Language Cookbook - Third Edition](https://www.packtpub.com/product/opengl-4-shading-language-cookbook-third-edition/9781789342253)  
[HLSL Development Cookbook](https://www.packtpub.com/product/hlsl-development-cookbook/9781849694209)  
[Unity 2018 Shaders and Effects Cookbook - Third Edition](https://www.packtpub.com/product/unity-2018-shaders-and-effects-cookbook-third-edition/9781788396233)  
[Physically Based Rendering in Filament](https://google.github.io/filament/Filament.md.html)  
[Vulkan Tutorial](https://vulkan-tutorial.com/)

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
[JSON for Modern C++]:<https://github.com/nlohmann/json>
