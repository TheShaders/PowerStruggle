#include <cstdlib>
#include <cmath>
#include <array>

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include <config.h>
#include <gfx.h>
#include <platform.h>
#include <ecs.h>
#include <input.h>
#include <pc_gfx.h>
#include <camera.h>

#include <DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp>
#include <DiligentTools/TextureLoader/interface/TextureLoader.h>
#include <DiligentTools/TextureLoader/interface/TextureUtilities.h>

using namespace Diligent;

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;
using float4x4 = glm::mat4;

struct Vertex
{
    float3 pos;
    float2 color;
};

IBuffer* m_CubeVertexBuffer;
IBuffer* m_CubeIndexBuffer;
IBuffer* m_VSConstants;
IShaderResourceBinding* m_pSRB;
float4x4 m_WorldViewProjMatrix;
ITextureView* m_TextureSRV;

// clang-format off
Vertex CubeVerts[] =
{
        {float3(+50,-50,-50), float2(0,1)},
        {float3(+50,+50,-50), float2(0,0)},
        {float3(-50,+50,-50), float2(1,0)},
        {float3(-50,-50,-50), float2(1,1)},

        {float3(+50,-50,-50), float2(0,1)},
        {float3(+50,-50,+50), float2(0,0)},
        {float3(-50,-50,+50), float2(1,0)},
        {float3(-50,-50,-50), float2(1,1)},

        {float3(-50,-50,-50), float2(0,1)},
        {float3(-50,-50,+50), float2(1,1)},
        {float3(-50,+50,+50), float2(1,0)},
        {float3(-50,+50,-50), float2(0,0)},

        {float3(-50,+50,-50), float2(0,1)},
        {float3(-50,+50,+50), float2(0,0)},
        {float3(+50,+50,+50), float2(1,0)},
        {float3(+50,+50,-50), float2(1,1)},

        {float3(+50,+50,-50), float2(1,0)},
        {float3(+50,+50,+50), float2(0,0)},
        {float3(+50,-50,+50), float2(0,1)},
        {float3(+50,-50,-50), float2(1,1)},

        {float3(+50,-50,+50), float2(1,1)},
        {float3(-50,-50,+50), float2(0,1)},
        {float3(-50,+50,+50), float2(0,0)},
        {float3(+50,+50,+50), float2(1,0)}
};

uint32_t Indices[] =
{
    2,0,1,    2,3,0,
    4,6,5,    4,7,6,
    8,10,9,   8,11,10,
    12,14,13, 12,15,14,
    16,18,17, 16,19,18,
    20,21,22, 20,22,23
};

glm::mat4 proj;

#include <glm/gtc/type_ptr.hpp>

void print_matrix(glm::mat4& mat)
{
    float *data = glm::value_ptr(mat);
    for (int i = 0; i < 15; i++)
    {
        std::cout << data[i] << ",";
    }
    std::cout << data[15] << std::endl;
}

void create_verts()
{
    // Create a vertex buffer that stores cube vertices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name          = "Cube vertex buffer";
    VertBuffDesc.Usage         = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags     = BIND_VERTEX_BUFFER;
    VertBuffDesc.uiSizeInBytes = sizeof(CubeVerts);
    BufferData VBData;
    VBData.pData    = CubeVerts;
    VBData.DataSize = sizeof(CubeVerts);
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_CubeVertexBuffer);
}

void create_indices()
{
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name          = "Cube index buffer";
    IndBuffDesc.Usage         = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags     = BIND_INDEX_BUFFER;
    IndBuffDesc.uiSizeInBytes = sizeof(Indices);
    BufferData IBData;
    IBData.pData    = Indices;
    IBData.DataSize = sizeof(Indices);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_CubeIndexBuffer);
}

uint32_t fillColor = GPACK_RGBA5551(0, 61, 8, 1) << 16 | GPACK_RGBA5551(0, 61, 8, 1);


static int cur_context = 0;
static std::array<std::array<MtxF, matf_stack_len>, 2> matrix_stacks;
int32_t firstFrame;

MtxF *g_curMatFPtr;


void gfx::load_perspective(float fov, float aspect, float n, float f, UNUSED float scale)
{
    proj = glm::perspective(glm::radians(fov), aspect, n, f);
}

extern SDL_Window* window;

void get_window_size(int *width, int *height)
{
    SDL_GetWindowSize(window, width, height);
}

float get_aspect_ratio()
{
    int width, height;
    get_window_size(&width, &height);
    return static_cast<float>(width) / static_cast<float>(height);
}

void initGfx()
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name = "Simple triangle PSO";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = m_pSwapChain->GetDesc().ColorBufferFormat;
    PSOCreateInfo.GraphicsPipeline.DSVFormat     = m_pSwapChain->GetDesc().DepthBufferFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.UseCombinedTextureSamplers = true;
    
    // In this tutorial, we will load shaders from file. To be able to do that,
    // we need to create a shader source stream factory
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube VS";
        ShaderCI.FilePath        = "shaders/cube.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        BufferDesc CBDesc;
        CBDesc.Name           = "VS constants CB";
        CBDesc.uiSizeInBytes  = sizeof(float4x4);
        CBDesc.Usage          = USAGE_DYNAMIC;
        CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(CBDesc, nullptr, &m_VSConstants);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = "shaders/cube.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }
    LayoutElement LayoutElems[] =
    {
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - texture coords
        LayoutElement{1, 0, 2, VT_FLOAT32, False}
    };
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);
    // Set up the texture as a shader resource binding
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);
    // Set up the sampler as a shader resource binding
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SamLinearClampDesc}
    };
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
    // change and are bound directly through the pipeline state object.
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

    // Create a shader resource binding object and bind all static resources in it
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    // Load the test texture
    TextureLoadInfo loadInfo;
    loadInfo.IsSRGB = true;
    RefCntAutoPtr<ITexture> Tex;
    CreateTextureFromFile("img/Test.png", loadInfo, m_pDevice, &Tex);
    // Get shader resource view from the texture
    m_TextureSRV = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    // Set texture SRV in the SRB
    m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV);

    create_verts();
    create_indices();
}

#include <unordered_map>
std::unordered_map<SDL_Scancode, uint32_t> keymap;

void initInput()
{
    int mappingsAdded = SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
    keymap = {
        {SDL_SCANCODE_SPACE, A_BUTTON},
        {SDL_SCANCODE_RETURN, B_BUTTON},
        {SDL_SCANCODE_LEFT, L_CBUTTONS},
        {SDL_SCANCODE_RIGHT, R_CBUTTONS},
        {SDL_SCANCODE_UP, U_CBUTTONS},
        {SDL_SCANCODE_DOWN, D_CBUTTONS}
    };

    if (mappingsAdded == -1)
    {
        std::cout << "Failed to add mappings" << std::endl;
    }
    else
    {
        std::cout << "Added " << mappingsAdded << " mappings" << std::endl;
    }

    int numJoysticks = SDL_NumJoysticks();

    std::cout << numJoysticks << " controllers connected!" << std::endl;
    for (int i = 0; i < numJoysticks; i++)
    {
        std::cout << "  " << SDL_GameControllerNameForIndex(i) << std::endl;
    }
}

uint32_t pressed_keys;

void beginInputPolling()
{

}

glm::vec2 map_square_to_circle(const glm::vec2& input)
{
    if (input.x == 0 && input.y == 0)
    {
        return glm::vec2{0, 0};
    }
    return glm::vec2{
        input.x * std::sqrt(1.0f - input.y * input.y / 2.0f), 
        input.y * std::sqrt(1.0f - input.x * input.x / 2.0f)
    };
}

void readInput()
{
    const uint8_t *keystates = SDL_GetKeyboardState(nullptr);
    uint32_t held_keys = 0;
    for (auto& [key, input]: keymap)
    {
        if (keystates[key])
        {
            held_keys |= input;
        }
    }
    glm::vec2 raw_input = glm::vec2{
        keystates[SDL_SCANCODE_D] - keystates[SDL_SCANCODE_A],
        keystates[SDL_SCANCODE_W] - keystates[SDL_SCANCODE_S]
    };
    glm::vec2 mapped_input = map_square_to_circle(raw_input);
    g_PlayerInput.buttonsPressed = pressed_keys;
    g_PlayerInput.buttonsHeld = held_keys;
    g_PlayerInput.magnitude = glm::length(mapped_input);
    g_PlayerInput.angle = atan2s(mapped_input.x, mapped_input.y);
    pressed_keys = 0;
}

#if FPS30
constexpr uint32_t ms_per_frame = 1000 / 30;
#else
constexpr uint32_t ms_per_frame = 1000 / 60;
#endif

void startFrame(void)
{
    SDL_Event currentEvent;
    static uint32_t prevFrame = 0;
    static uint32_t curFrame;
    while (curFrame = SDL_GetTicks(), curFrame - prevFrame < ms_per_frame)
    {
        SDL_Delay(1);
    }
    prevFrame = curFrame;
    while (SDL_PollEvent(&currentEvent) != 0)
    {
        switch (currentEvent.type)
        {
            case SDL_QUIT:
                SDL_DestroyWindow(window);
                SDL_Quit();
                exit(EXIT_SUCCESS);
                break;
            case SDL_KEYDOWN:
                auto find_result = keymap.find(currentEvent.key.keysym.scancode);
                if (find_result != keymap.end())
                {
                    pressed_keys |= find_result->second;
                }
                break;
        }
    }

    cur_context ^= 1;
    g_curMatFPtr = matrix_stacks[cur_context].data();
    gfx::load_identity();

    // Resize swap chain
    int width, height;
    get_window_size(&width, &height);
    m_pSwapChain->Resize(width, height);
}

void endFrame(void)
{
    Entity player;
    void *components[NUM_COMPONENTS(ARCHETYPE_PLAYER)];

    player.archetype = ARCHETYPE_PLAYER;
    player.archetypeArrayIndex = 0;
    getEntityComponents(&player, &components[0]);

    auto pos = get_component<Bit_Position, Vec3>(components, ARCHETYPE_PLAYER);
        
    static UNUSED float prevTime = 0.0f;
    float time = SDL_GetTicks() / 1000.0f;

    // Calc MVP matrix
    float4x4 CubeModelTransform = glm::rotate(glm::identity<float4x4>(), time * 1.0f, glm::vec3(0, 1, 0))
     * glm::rotate(glm::identity<float4x4>(), -glm::pi<float>() * 0.1f, glm::vec3(1, 0, 0));

    // View matrix is the first matrix in the stack
    glm::mat4 *view = reinterpret_cast<glm::mat4*>(&matrix_stacks[cur_context][0]);
    
    m_WorldViewProjMatrix = proj * *view * CubeModelTransform;

    // Set render targets before issuing any draw command.
    // Note that Present() unbinds the back buffer if it is set as render target.
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Clear the back buffer
    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    // Let the engine perform required state transitions
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        // Map the buffer and write current world-view-projection matrix
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = glm::transpose(m_WorldViewProjMatrix);
    }

    // Bind vertex and index buffers
    Uint32   offset   = 0;
    IBuffer* pBuffs[] = {m_CubeVertexBuffer};
    m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pImmediateContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state in the immediate context
    m_pImmediateContext->SetPipelineState(m_pPSO);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    m_pImmediateContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs;     // This is an indexed draw call
    DrawAttrs.IndexType  = VT_UINT32; // Index type
    DrawAttrs.NumIndices = sizeof(Indices) / sizeof(uint32_t);
    // Verify the state of vertex and index buffers
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    m_pImmediateContext->DrawIndexed(DrawAttrs);

    DrawIndexedIndirectAttribs DrawAttribs2;
    DrawAttribs2.IndexType = VT_UINT32;
    DrawAttribs2.IndirectDrawArgsOffset = 0;
    DrawAttribs2.IndirectAttribsBufferStateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    DrawAttribs2.Flags = DRAW_FLAG_VERIFY_ALL;

    m_pSwapChain->Present();
    
    prevTime = time;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void setLightDirection(Vec3 lightDir) {}

void drawModel(Model *toDraw, Animation* anim, uint32_t frame) {}
void drawAABB(DrawLayer layer, AABB *toDraw, uint32_t color) {}
void drawLine(DrawLayer layer, Vec3 start, Vec3 end, uint32_t color) {}
void drawColTris(DrawLayer layer, ColTri *tris, uint32_t count, uint32_t color) {}

void scrollTextures() {}
void animateTextures() {}
void shadeScreen(float alphaPercent) {}
#pragma GCC diagnostic pop

extern "C" void guLookAtF(float mf[4][4], float xEye, float yEye, float zEye,
		      float xAt,  float yAt,  float zAt,
		      float xUp,  float yUp,  float zUp)
{
    glm::mat4 *mat_out = reinterpret_cast<glm::mat4*>(&mf[0][0]);
    *mat_out = glm::lookAt(
        glm::vec3(xEye, yEye, zEye),
        glm::vec3(xAt, yAt, zAt),
        glm::vec3(xUp, yUp, zUp)
    );
}

extern "C" void guPositionF(float mf[4][4], float r, float p, float h, float s,
			float x, float y, float z)
{
    (void)mf;
    (void)r;
    (void)p;
    (void)h;
    (void)s;
    (void)x;
    (void)y;
    (void)z;
    // TODO
}

extern "C" void guTranslateF(float mf[4][4], float x, float y, float z)
{
    glm::mat4 *mat_out = reinterpret_cast<glm::mat4*>(&mf[0][0]);
    *mat_out = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x, y, z));
}

extern "C" void guScaleF(float mf[4][4], float x, float y, float z)
{
    glm::mat4 *mat_out = reinterpret_cast<glm::mat4*>(&mf[0][0]);
    *mat_out = glm::scale(glm::identity<glm::mat4>(), glm::vec3(x, y, z));
}

extern "C" void guRotateF(float mf[4][4], float a, float x, float y, float z)
{
    glm::mat4 *mat_out = reinterpret_cast<glm::mat4*>(&mf[0][0]);
    *mat_out = glm::rotate(glm::identity<glm::mat4>(), glm::radians(a), glm::vec3(x, y, z));
}

extern "C" void guMtxIdentF(float mf[4][4])
{
    glm::mat4 *mat_out = reinterpret_cast<glm::mat4*>(&mf[0][0]);
    *mat_out = glm::identity<glm::mat4>();
}
