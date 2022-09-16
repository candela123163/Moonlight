#include "stdafx.h"
#include "GUIPass.h"
#include "descriptor.h"
#include "gameApp.h"
#include "renderOption.h"

void GUIPass::PreparePass(const GraphicContext& context)
{
    ImGui::StyleColorsDark();

    DescriptorData descriptor = context.descriptorHeap->GetNextnSrvDescriptor(1);
    ImGui_ImplWin32_Init(GameApp::GetApp()->MainWnd());
    ImGui_ImplDX12_Init(
        context.device,
        SWAP_CHAIN_COUNT,
        context.backBufferFormat,
        context.descriptorHeap->GetSrvHeap(),
        descriptor.CPUHandle,
        descriptor.GPUHandle
    );
}

void GUIPass::PreprocessPass(const GraphicContext& context)
{

}

void GUIPass::DrawPass(const GraphicContext& context)
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Option");

    RenderOption* option = context.renderOption;

    ImGui::Text("Light & Shadow");
    ImGui::DragFloat("Sun Intensity", &option->SunIntensity, 0.05f, 0.0f, 100.0f);
    ImGui::DragFloat("Sun X", &option->SunDirection.x, 0.005f, -1.0f, 1.0f);
    ImGui::DragFloat("Sun Y", &option->SunDirection.y, 0.005f, -1.0f, 1.0f);
    ImGui::DragFloat("Sun Z", &option->SunDirection.z, 0.005f, -1.0f, 1.0f);

    ImGui::Text("SSAO");
    ImGui::Checkbox("SSAO Enable", &option->SSAOEnable);
    ImGui::DragFloat("Radius", &option->SSAORadius, 0.005f, 0.1f, 3.0f);
    ImGui::DragFloat("Power", &option->SSAOPower, 0.005f, 1.0f, 10.0f);

    ImGui::Text("Bloom");
    ImGui::Checkbox("Bloom Enable", &option->BloomEnable);
    ImGui::DragFloat("Threshold", &option->BloomThreshold, 0.005f, 0.1f, 10.0f);
    ImGui::DragFloat("Curve Knee", &option->BloomCurveKnee, 0.005f, 0.01f, 1.0f);
    ImGui::DragFloat("Intensity", &option->BloomIntensity, 0.005f, 0.0f, 2.0f);

    ImGui::Text("TAA");
    ImGui::Checkbox("TAA Enable", &option->TAAEnable);


    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context.commandList);
}

void GUIPass::ReleasePass(const GraphicContext& context)
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}