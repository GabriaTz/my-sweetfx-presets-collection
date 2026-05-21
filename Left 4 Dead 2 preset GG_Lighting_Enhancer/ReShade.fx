  /*----------------------------.
  | :: Framework Initializer :: |
  '----------------------------*/

// Global Settings
#include "ReShade\PersonalFiles\KeyCodes.h"
#include "ReShade\Common.cfg"

#if RFX_Screenshot_Format != 2
	#pragma reshade screenshot_format bmp
#else
	#pragma reshade screenshot_format png
#endif

#if RFX_ShowFPS == 1
	#pragma reshade showfps
#endif
#if RFX_ShowClock == 1
	#pragma reshade showclock
#endif
#if RFX_ShowStatistics == 1
	#pragma reshade showstatistics
#endif

#if RFX_ShowToggleMessage == 1
	#pragma reshade showtogglemessage
#endif

// Global Variables
#define RFX_pixelSize RFX_PixelSize
#define RFX_PixelSize float2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT)
#define RFX_ScreenSize float2(BUFFER_WIDTH, BUFFER_HEIGHT)
#define RFX_ScreenSizeFull float4(BUFFER_WIDTH, BUFFER_RCP_WIDTH, float(BUFFER_WIDTH) / float(BUFFER_HEIGHT), float(BUFFER_HEIGHT) / float(BUFFER_WIDTH)) // x = Width, y = 1 / Width, z = ScreenScaleY, w = 1 / ScreenScaleY

uniform float RFX_timer < string source = "timer"; >;
uniform float RFX_timeleft < string source = "timeleft"; >;
uniform float RFX_frametime < source = "frametime"; >;

// Global Textures and Samplers
texture RFX_depthBufferTex : DEPTH;
texture RFX_depthTex { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = R32F; };

texture RFX_backbufferTex : COLOR;

#if RFX_InitialStorage
	texture RFX_originalTex { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
#else
	texture RFX_originalTex : COLOR;
#endif

sampler RFX_depthColor { Texture = RFX_depthBufferTex; };
sampler RFX_depthTexColor { Texture = RFX_depthTex; };

sampler RFX_backbufferColor { Texture = RFX_backbufferTex; };
sampler RFX_originalColor { Texture = RFX_originalTex; };

// Fullscreen Triangle Vertex Shader
void RFX_VS_PostProcess(in uint id : SV_VertexID, out float4 pos : SV_Position, out float2 texcoord : TEXCOORD)
{
	texcoord.x = (id == 2) ? 2.0 : 0.0;
	texcoord.y = (id == 1) ? 2.0 : 0.0;
	pos = float4(texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}

#if RFX_InitialStorage
float4 RFX_PS_StoreColor(float4 vpos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	return tex2D(RFX_backbufferColor, texcoord);
}
#endif
#if RFX_DepthBufferCalc
float  RFX_PS_StoreDepth(in float4 position : SV_Position, in float2 texcoord : TEXCOORD0) : SV_Target
{
	float depth = tex2D(RFX_depthColor, texcoord).x;

	// Linearize depth	
#if RFX_LogDepth 
	depth = saturate(1.0f - depth);
	depth = (exp(pow(depth, 150 * pow(depth, 55) + 32.75f / pow(depth, 5) - 1850f * (pow((1 - depth), 2)))) - 1) / (exp(depth) - 1); // Made by LuciferHawk ;-)
#else
  	const float zFarPlane = RFX_Depth_z_far;
  	const float zNearPlane = RFX_Depth_z_near;
	depth = 1.0 / ((depth * ((zFarPlane - zNearPlane) / (-zFarPlane * zNearPlane)) + zFarPlane / (zFarPlane * zNearPlane)));
#endif

	// SweetFX Dither Method #1
	const float dither_bit = 8.0;
	float dither_shift = 0.25 * (1.0 / (pow(2, dither_bit) - 1.0));
	dither_shift = lerp(2.0 * dither_shift, -2.0 * dither_shift, frac(dot(texcoord, RFX_ScreenSize * float2(1.0 / 16.0, 10.0 / 36.0)) + 0.25));

	return depth + dither_shift;
}
#endif

#if RFX_InitialStorage || RFX_DepthBufferCalc
technique RFX_Setup_Tech < enabled = true; >
{
	#if RFX_InitialStorage
	pass StoreColor
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = RFX_PS_StoreColor;
		RenderTarget = RFX_originalTex;	
	}
	#endif
	#if RFX_DepthBufferCalc
	pass StoreDepth
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = RFX_PS_StoreDepth;
		RenderTarget = RFX_depthTex;
	}
	#endif
}
#endif

// Global Defines
#if defined(__RESHADE__) && __RESHADE__ >= 1700
	#define NAMESPACE_ENTER(name) namespace name {
	#define NAMESPACE_LEAVE() }
#else
	#define NAMESPACE_ENTER(name)
	#define NAMESPACE_LEAVE()
#endif

#define STR(name) #name
#define EFFECT(l,n) STR(ReShade/l/##n.h)

  /*----------------------------.
  | ::    Effect Pipeline    :: |
  '----------------------------*/

#include "ReShade\Pipeline.cfg"

  /*----------------------------.
  | ::  Framework Finalizer  :: |
  '----------------------------*/

#if RFX_ShowToggleMessage
float4 RFX_PS_ToggleMessage(in float4 position : SV_Position, in float2 texcoord : TEXCOORD0) : SV_Target
{
	return tex2D(RFX_backbufferColor, texcoord);
}

technique Framework < enabled = RFX_Start_Enabled; toggle = RFX_ToggleKey; >
{
	pass 
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = RFX_PS_ToggleMessage;
	}
}
#endif
