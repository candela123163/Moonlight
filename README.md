<h1 align="center">
  Moonlight Renderer
</h1>

**This is a realtime renderer written with cpp 17 and dx12, available for Windows.**

![moonlight](/asset/screenshot/moonlight.png)

# ⭐️Features
- Support **glTF**

- Frustum Culling

- Metal workflow Physically based rendering(PBR)
![pbr1](/asset/screenshot/pbr1.png)
![pbr2](/asset/screenshot/pbr2.png)
![pbr3](/asset/screenshot/pbr3.png)

- Point & Spot & Directional Lighting and shadowing
![csm](/asset/screenshot/pointLight.png)
![csm](/asset/screenshot/spotLight.png)

- Image-based Lighting(IBL)
  - Direct lighting only
  ![withoutIBL](/asset/screenshot/withoutIBL.png)
  - Plus IBL
  ![IBL](/asset/screenshot/IBL.png)

- Percentage closer filtering(PCF)
- Cascade shadow map(CSM) 
![csm](/asset/screenshot/csm.png)

- HDR rendering and Tone mapping

- Bloom
![bloom](/asset/screenshot/bloom.png)

- Screen space ambient occlusion(SSAO)
  - without SSAO
  ![withoutAO](/asset/screenshot/withoutAO.png)
  - with SSAO
  ![ssao](/asset/screenshot/ssao.png)

- ~~FXAA~~ (**NOT** temporal stable, deprecated)

- Temporal anti-aliasing(TAA)
  - without AA
  ![withoutAA](/asset/screenshot/withoutAA.png)
  - Enable TAA
  ![taa](/asset/screenshot/taa.png)

# 📚Dependencies 
- assimp
- directxtk12
- nlohmann json
- imgui

# 🔧Build
- You need Windows 10 and Visual Studio 2022 (maybe lower version is work fine too)
- Double click **MoonlightRenderer.sln**

# 🚀Quick Start
- Take a look at ***./config.json***, set **SceneFile** to specity the scene to render.
- Execute **run.bat**, this is all!
- If you want to render your own scene, add a file to ***./asset/scenes/***, and set it in ***./config.json***

# ⚖️License
[MIT License](LICENSE)
