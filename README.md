# TinyRenderer 简介
使用CPP 17及DirectX3D 12编写的实时渲染器

# Features
- 支持gltf格式文件渲染

- PBR metal workflow shading
![csm](/asset/screenshot/pbr1.png)
![csm](/asset/screenshot/pbr2.png)
![csm](/asset/screenshot/pbr3.png)

- 点光源、射灯、平行光 光照及阴影
![csm](/asset/screenshot/pointLight.png)
![csm](/asset/screenshot/spotLight.png)

- IBL

仅直接光照
![csm](/asset/screenshot/withoutIBL.png)
直接光照+IBL
![csm](/asset/screenshot/IBL.png)

- 高质量平行光阴影 CSM + PCF
![csm](/asset/screenshot/csm.png)

- HDR + Tone mapping

- Bloom
![csm](/asset/screenshot/bloom.png)

- SSAO

无SSAO
![csm](/asset/screenshot/withoutAO.png)
开启SSAO
![csm](/asset/screenshot/ssao.png)

- FXAA(decrepited) NOT temporal stable

- TAA temporal stable

无AA
![csm](/asset/screenshot/withoutAA.png)
开启TAA
![csm](/asset/screenshot/taa.png)

# 依赖项
- assimp
- directxtk12
- nlohmann
- imgui

# Build
- 使用VS2022打开TinyRenderer.sln构建工程
- \x64\bin包含已编译的可执行文件
- 运行run.bat打开渲染器

# 配置
- ./config.json中指定渲染的场景
- ./asset/scenes中*.json文件为场景文件，可配置场景模型、光照、相机等参数，可参照来创建你自己的场景文件进行渲染
