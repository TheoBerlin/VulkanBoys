#include "ImguiVK.h"
#include "BufferVK.h"
#include "ShaderVK.h"
#include "SamplerVK.h"
#include "PipelineVK.h"
#include "Texture2DVK.h"
#include "ImageViewVK.h"
#include "RenderPassVK.h"
#include "CommandBufferVK.h"
#include "DescriptorSetVK.h"
#include "DescriptorPoolVK.h"
#include "PipelineLayoutVK.h"
#include "GraphicsContextVK.h"

#include "Core/Application.h"
#include "Common/IWindow.h"

//Dependent on GLFW maybe not so good
#include <GLFW/glfw3.h>
#if defined(_WIN32)
	#define GLFW_EXPOSE_NATIVE_WIN32
	#include <glfw/glfw3native.h>
#endif
#include <imgui/imgui.h>

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;
out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;
void main()
{
	Out.Color = aColor;
	Out.UV = aUV;
	gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/

static uint32_t __glsl_shader_vert_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
	0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
	0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
	0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
	0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
	0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
	0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
	0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
	0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
	0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
	0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
	0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
	0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
	0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
	0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
	0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
	0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
	0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
	0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
	0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
	0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
	0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
	0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
	0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
	0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
	0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
	0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
	0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
	0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
	0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
	0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
	0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
	0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
	0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
	0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
	0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
	0x0000002d,0x0000002c,0x000100fd,0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
	fColor = In.Color * texture(sTexture, In.UV.st);
}
*/

static uint32_t __glsl_shader_frag_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
	0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
	0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
	0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
	0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
	0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
	0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
	0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
	0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
	0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
	0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
	0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
	0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
	0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
	0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
	0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
	0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
	0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
	0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
	0x00010038
};

static const char* ImGuiGetClipboardText(void* user_data)
{
	return glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGuiSetClipboardText(void* user_data, const char* text)
{
	glfwSetClipboardString((GLFWwindow*)user_data, text);
}

static GLFWcursor* g_MouseCursors[ImGuiMouseCursor_COUNT] = {};

ImguiVK::ImguiVK(GraphicsContextVK* pContext)
	: m_pContext(pContext),
	m_pSampler(nullptr),
	m_pPipelineLayout(nullptr),
	m_pPipeline(nullptr),
	m_pRenderPass(nullptr),
	m_pFontTexture(nullptr),
	m_pDescriptorPool(nullptr),
	m_pDescriptorSet(nullptr),
	m_pDescriptorSetLayout(nullptr)
{
}

ImguiVK::~ImguiVK()
{
	ImGui::DestroyContext();

	SAFEDELETE(m_pSampler);
	SAFEDELETE(m_pRenderPass);
	SAFEDELETE(m_pPipelineLayout);
	SAFEDELETE(m_pPipeline);
	SAFEDELETE(m_pFontTexture);
	SAFEDELETE(m_pVertexBuffer);
	SAFEDELETE(m_pIndexBuffer);
	SAFEDELETE(m_pDescriptorPool);
	SAFEDELETE(m_pDescriptorSetLayout);
}

bool ImguiVK::init()
{
	if (!initImgui())
	{
		return false;
	}

	if (!createRenderPass())
	{
		return false;
	}

	if (!createPipelineLayout())
	{
		return false;
	}

	if (!createPipeline())
	{
		return false;
	}

	if(!createFontTexture())
	{
		return false;
	}

	//TODO: Fix resizeable buffers
	if (!createBuffers(MB(16), MB(16)))
	{
		return false;
	}

	//if (!createBuffers(sizeof(ImDrawVert) * 1024, sizeof(ImDrawIdx) * 1024))
	//{
	//	return false;
	//}

	//Write to descriptor sets
	Texture2DVK* pTexture = reinterpret_cast<Texture2DVK*>(m_pFontTexture);
	m_pDescriptorSet->writeCombinedImageDescriptor(pTexture->getImageView()->getImageView(), m_pSampler->getSampler(), 0);

	D_LOG("ImGui initialized successfully");
	return true;
}

void ImguiVK::begin(double deltatime)
{
    ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltatime;

    //Maybe should not be dependent on Application?
    IWindow* pWindow = Application::get()->getWindow();
    io.DisplaySize = ImVec2(float(pWindow->getClientWidth()), float(pWindow->getClientHeight()));
	io.DisplayFramebufferScale = ImVec2(pWindow->getScaleX(), pWindow->getScaleY());

	ImGui::NewFrame();
}

void ImguiVK::end()
{
	ImGui::EndFrame();
	ImGui::Render();
}

void ImguiVK::render(CommandBufferVK* pCommandBuffer)
{
	//Start drawing
	ImGuiIO& io = ImGui::GetIO();
	ImDrawData* pDrawData = ImGui::GetDrawData();

	//uint64 vertexSize	= pDrawData->TotalVtxCount * sizeof(ImDrawVert);
	//uint64 indexSize	= pDrawData->TotalIdxCount * sizeof(ImDrawIdx);
	// pload vertex/index data into a single contiguous GPU buffer
	{
		ImDrawVert* pVtxDst = nullptr;
		ImDrawIdx* pIdxDst = nullptr;

		m_pVertexBuffer->map(reinterpret_cast<void**>(&pVtxDst));
		m_pIndexBuffer->map(reinterpret_cast<void**>(&pIdxDst));

		for (int n = 0; n < pDrawData->CmdListsCount; n++)
		{
			const ImDrawList* pCmdList = pDrawData->CmdLists[n];
			memcpy(pVtxDst, pCmdList->VtxBuffer.Data, pCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(pIdxDst, pCmdList->IdxBuffer.Data, pCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
			pVtxDst += pCmdList->VtxBuffer.Size;
			pIdxDst += pCmdList->IdxBuffer.Size;
		}

		m_pVertexBuffer->unmap();
		m_pIndexBuffer->unmap();
	}

	//Setup pipelinestate
	pCommandBuffer->bindGraphicsPipeline(m_pPipeline);
	//Set shader variable list
	pCommandBuffer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSet, 0, nullptr);

	//Setup vertex and indexbuffer
	VkDeviceSize offset = 0;
	pCommandBuffer->bindVertexBuffers(&m_pVertexBuffer, 1, &offset);
	pCommandBuffer->bindIndexBuffer(m_pIndexBuffer, 0, (sizeof(ImDrawIdx) == 2) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

	//Setup viewport
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width	= io.DisplaySize.x * io.DisplayFramebufferScale.x;
	viewport.height = io.DisplaySize.y * io.DisplayFramebufferScale.y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	pCommandBuffer->setViewports(&viewport, 1);

	// Setup scale and translation:
	// Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		float scale[2];
		scale[0] = 2.0f / pDrawData->DisplaySize.x;
		scale[1] = 2.0f / pDrawData->DisplaySize.y;
		float translate[2];
		translate[0] = -1.0f - pDrawData->DisplayPos.x * scale[0];
		translate[1] = -1.0f - pDrawData->DisplayPos.y * scale[1];

		pCommandBuffer->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
		pCommandBuffer->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
	}

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clipOff		= pDrawData->DisplayPos;       // (0,0) unless using multi-viewports
	ImVec2 clipScale	= pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int32_t globalVtxOffset = 0;
	int32_t globalIdxOffset = 0;
	for (int32_t n = 0; n < pDrawData->CmdListsCount; n++)
	{
		const ImDrawList* pCmdList = pDrawData->CmdLists[n];
		for (int32_t i = 0; i < pCmdList->CmdBuffer.Size; i++)
		{
			const ImDrawCmd* pCmd = &pCmdList->CmdBuffer[i];
			// Project scissor/clipping rectangles into framebuffer space
			ImVec4 clipRect;
			clipRect.x = (pCmd->ClipRect.x - clipOff.x) * clipScale.x;
			clipRect.y = (pCmd->ClipRect.y - clipOff.y) * clipScale.y;
			clipRect.z = (pCmd->ClipRect.z - clipOff.x) * clipScale.x;
			clipRect.w = (pCmd->ClipRect.w - clipOff.y) * clipScale.y;

			if (clipRect.x < viewport.width && clipRect.y < viewport.height && clipRect.z >= 0.0f && clipRect.w >= 0.0f)
			{
				// Negative offsets are illegal for vkCmdSetScissor
				if (clipRect.x < 0.0f)
					clipRect.x = 0.0f;
				if (clipRect.y < 0.0f)
					clipRect.y = 0.0f;

				// Apply scissor/clipping rectangle	
				VkRect2D scissor = {};
				scissor.offset.x = clipRect.x;
				scissor.offset.y = clipRect.y;
				scissor.extent.width	= clipRect.z - clipRect.x;
				scissor.extent.height	= clipRect.w - clipRect.y;
				pCommandBuffer->setScissorRects(&scissor, 1);

				// Draw
				pCommandBuffer->drawIndexInstanced(pCmd->ElemCount, 1, pCmd->IdxOffset + globalIdxOffset, pCmd->VtxOffset + globalVtxOffset, 0);
			}
		}
		globalIdxOffset += pCmdList->IdxBuffer.Size;
		globalVtxOffset += pCmdList->VtxBuffer.Size;
	}
}

void ImguiVK::onWindowClose()
{
}

void ImguiVK::onWindowResize(uint32_t width, uint32_t height)
{
}

void ImguiVK::onWindowFocusChanged(IWindow* pWindow, bool hasFocus)
{
}

void ImguiVK::onMouseMove(uint32_t x, uint32_t y)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(float(x), float(y));
}

void ImguiVK::onMousePressed(int32_t button)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDown[button] = true;
}

void ImguiVK::onMouseScroll(double x, double y)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheelH	+= x;
	io.MouseWheel	+= y;
}

void ImguiVK::onMouseReleased(int32_t button)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDown[button] = false;
}

void ImguiVK::onKeyTyped(uint32_t character)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharacter(character);
}

void ImguiVK::onKeyPressed(EKey key)
{
	ImGuiIO& io = ImGui::GetIO();
	io.KeysDown[key] = true;
	io.KeyCtrl	= io.KeysDown[EKey::KEY_LEFT_CONTROL]	|| io.KeysDown[EKey::KEY_RIGHT_CONTROL];
	io.KeyShift	= io.KeysDown[EKey::KEY_LEFT_SHIFT]		|| io.KeysDown[EKey::KEY_RIGHT_SHIFT];
	io.KeyAlt	= io.KeysDown[EKey::KEY_LEFT_ALT]		|| io.KeysDown[EKey::KEY_RIGHT_ALT];
	io.KeySuper	= io.KeysDown[EKey::KEY_LEFT_SUPER]		|| io.KeysDown[EKey::KEY_RIGHT_SUPER];
}

void ImguiVK::onKeyReleased(EKey key)
{
	ImGuiIO& io = ImGui::GetIO();
	io.KeysDown[key] = false;
	io.KeyCtrl	= io.KeysDown[EKey::KEY_LEFT_CONTROL]	|| io.KeysDown[EKey::KEY_RIGHT_CONTROL];
	io.KeyShift = io.KeysDown[EKey::KEY_LEFT_SHIFT]		|| io.KeysDown[EKey::KEY_RIGHT_SHIFT];
	io.KeyAlt	= io.KeysDown[EKey::KEY_LEFT_ALT]		|| io.KeysDown[EKey::KEY_RIGHT_ALT];
	io.KeySuper = io.KeysDown[EKey::KEY_LEFT_SUPER]		|| io.KeysDown[EKey::KEY_RIGHT_SUPER];
}

bool ImguiVK::initImgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.BackendPlatformName = "VulkanBoys";

	io.KeyMap[ImGuiKey_Tab]			= EKey::KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow]	= EKey::KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow]	= EKey::KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow]		= EKey::KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow]	= EKey::KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp]		= EKey::KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown]	= EKey::KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home]		= EKey::KEY_HOME;
	io.KeyMap[ImGuiKey_End]			= EKey::KEY_END;
	io.KeyMap[ImGuiKey_Insert]		= EKey::KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete]		= EKey::KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace]	= EKey::KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space]		= EKey::KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter]		= EKey::KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape]		= EKey::KEY_ESCAPE;
	io.KeyMap[ImGuiKey_KeyPadEnter] = EKey::KEY_KP_ENTER;
	io.KeyMap[ImGuiKey_A]			= EKey::KEY_A;
	io.KeyMap[ImGuiKey_C]			= EKey::KEY_C;
	io.KeyMap[ImGuiKey_V]			= EKey::KEY_V;
	io.KeyMap[ImGuiKey_X]			= EKey::KEY_X;
	io.KeyMap[ImGuiKey_Y]			= EKey::KEY_Y;
	io.KeyMap[ImGuiKey_Z]			= EKey::KEY_Z;

	//TODO: Not based on glfw
    IWindow* pWindow = Application::get()->getWindow();
	GLFWwindow* pNativeWindow = (GLFWwindow*)pWindow->getNativeHandle();
#if defined(_WIN32)
	io.ImeWindowHandle = (void*)glfwGetWin32Window(pNativeWindow);
#endif
	
	//TODO: Not based on glfw
	io.SetClipboardTextFn	= ImGuiSetClipboardText;
	io.GetClipboardTextFn	= ImGuiGetClipboardText;
	io.ClipboardUserData	= pWindow;

	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding	= 0.0f;
	ImGui::GetStyle().ChildRounding		= 0.0f;
	ImGui::GetStyle().FrameRounding		= 0.0f;
	ImGui::GetStyle().GrabRounding		= 0.0f;
	ImGui::GetStyle().PopupRounding		= 0.0f;
	ImGui::GetStyle().ScrollbarRounding = 0.0f;

	//TODO: Not based on glfw
	GLFWerrorfun prev_error_callback = glfwSetErrorCallback(NULL);
	g_MouseCursors[ImGuiMouseCursor_Arrow]		= glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_TextInput]	= glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeNS]	= glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeEW]	= glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_Hand]		= glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
	g_MouseCursors[ImGuiMouseCursor_ResizeAll]	= glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
	g_MouseCursors[ImGuiMouseCursor_ResizeAll]	= glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	g_MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
	glfwSetErrorCallback(prev_error_callback);

	return true;
}

bool ImguiVK::createRenderPass()
{
	m_pRenderPass = DBG_NEW RenderPassVK(m_pContext->getDevice());

	VkAttachmentDescription description = {};
	description.format	= VK_FORMAT_B8G8R8A8_UNORM;
	description.samples	= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;				
	description.storeOp	= VK_ATTACHMENT_STORE_OP_STORE;				
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;			
	description.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		
	m_pRenderPass->addAttachment(description);

	description.format	= VK_FORMAT_D24_UNORM_S8_UINT;
	description.samples	= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp	= VK_ATTACHMENT_STORE_OP_STORE;	
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;	
	description.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment = 1;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pRenderPass->addSubpass(&colorAttachmentRef, 1, &depthStencilAttachmentRef);
	m_pRenderPass->addSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	m_pRenderPass->finalize();

	return true;
}

bool ImguiVK::createPipeline()
{
	std::vector<char> vertexByteCode(sizeof(__glsl_shader_vert_spv));
	memcpy(vertexByteCode.data(), __glsl_shader_vert_spv, sizeof(__glsl_shader_vert_spv));

	IShader* pVertexShader = m_pContext->createShader();
	pVertexShader->initFromByteCode(EShader::VERTEX_SHADER, "main", vertexByteCode);
	pVertexShader->finalize();

	std::vector<char> pixelByteCode(sizeof(__glsl_shader_frag_spv));
	memcpy(pixelByteCode.data(), __glsl_shader_frag_spv, sizeof(__glsl_shader_frag_spv));

	IShader* pPixelShader = m_pContext->createShader();
	pPixelShader->initFromByteCode(EShader::PIXEL_SHADER, "main", pixelByteCode);
	pPixelShader->finalize();

	std::vector<IShader*> shaders = { pVertexShader, pPixelShader };
	m_pPipeline = DBG_NEW PipelineVK(m_pContext->getDevice());
	m_pPipeline->addVertexAttribute(0, VK_FORMAT_R32G32_SFLOAT, 0, IM_OFFSETOF(ImDrawVert, pos));
	m_pPipeline->addVertexAttribute(0, VK_FORMAT_R32G32_SFLOAT, 1, IM_OFFSETOF(ImDrawVert, uv));
	m_pPipeline->addVertexAttribute(0, VK_FORMAT_R8G8B8A8_UNORM, 2, IM_OFFSETOF(ImDrawVert, col));

	m_pPipeline->addVertexBinding(0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(ImDrawVert));

	m_pPipeline->addColorBlendAttachment(true, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

	m_pPipeline->setCulling(false);
	m_pPipeline->setWireFrame(false);
	m_pPipeline->setDepthTest(false);

	m_pPipeline->finalize(shaders, m_pRenderPass, m_pPipelineLayout);

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	return true;
}

bool ImguiVK::createPipelineLayout()
{
	m_pSampler = DBG_NEW SamplerVK(m_pContext->getDevice());
	m_pSampler->init(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	const VkSampler immutableSamplers[] = { m_pSampler->getSampler() };
	m_pDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pContext->getDevice());
	m_pDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, immutableSamplers, 0, 1);
	m_pDescriptorSetLayout->finalize();

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.size = sizeof(float) * 4;
	pushConstantRange.offset = 0;
	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };

	std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts = { m_pDescriptorSetLayout };
	m_pPipelineLayout = DBG_NEW PipelineLayoutVK(m_pContext->getDevice());
	m_pPipelineLayout->init(descriptorSetLayouts, pushConstantRanges);

	DescriptorCounts counts;
	counts.m_SampledImages	= 1;
	counts.m_StorageBuffers = 1;
	counts.m_UniformBuffers = 1;

	m_pDescriptorPool = DBG_NEW DescriptorPoolVK(m_pContext->getDevice());
	m_pDescriptorPool->init(counts, 1);
	m_pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayout);

	return true;
}

bool ImguiVK::createFontTexture()
{
	ImGuiIO& io = ImGui::GetIO();

	uint8_t* pPixels = nullptr;
	int32_t width	= 0;
	int32_t height	= 0;
	io.Fonts->GetTexDataAsRGBA32(&pPixels, &width, &height);
	size_t uploadSize = width * height * 4;

	m_pFontTexture = m_pContext->createTexture2D();
	return m_pFontTexture->initFromMemory(pPixels, width, height);
}

bool ImguiVK::createBuffers(uint32_t vertexBufferSize, uint32_t indexBufferSize)
{
	BufferParams vertexBufferparams = {};
	vertexBufferparams.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferparams.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	vertexBufferparams.SizeInBytes = vertexBufferSize;

	m_pVertexBuffer = DBG_NEW BufferVK(m_pContext->getDevice());
	if (!m_pVertexBuffer->init(vertexBufferparams))
	{
		return false;
	}

	BufferParams indexBufferparams = {};
	indexBufferparams.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	indexBufferparams.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	indexBufferparams.SizeInBytes = indexBufferSize;

	m_pIndexBuffer = DBG_NEW BufferVK(m_pContext->getDevice());
	if (!m_pIndexBuffer->init(indexBufferparams))
	{
		return false;
	}

	return true;
}
