NAMESPACE_ENTER(MFX)

#include MFX_SETTINGS_DEF

#if USE_RBM == 1

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//LICENSE AGREEMENT AND DISTRIBUTION RULES:
//1 Copyrights of the Master Effect exclusively belongs to author - Gilcher Pascal aka Marty McFly.
//2 Master Effect (the SOFTWARE) is DonateWare application, which means you may or may not pay for this software to the author as donation.
//3 If included in ENB presets, credit the author (Gilcher Pascal aka Marty McFly).
//4 Software provided "AS IS", without warranty of any kind, use it on your own risk. 
//5 You may use and distribute software in commercial or non-commercial uses. For commercial use it is required to warn about using this software (in credits, on the box or other places). Commercial distribution of software as part of the games without author permission prohibited.
//6 Author can change license agreement for new versions of the software.
//7 All the rights, not described in this license agreement belongs to author.
//8 Using the Master Effect means that user accept the terms of use, described by this license agreement.
 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//For more information about license agreement contact me:
//https://www.facebook.com/MartyMcModding
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Copyright (c) 2009-2015 Gilcher Pascal aka Marty McFly
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

texture   texHDR { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT;  Format = RGBA16; };

sampler2D SamplerHDR
{
	Texture = texHDR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

float GetProDepth(float2 coords) //not really linear but better for normal map generation
{
	return 202.0 / (-99.0 * tex2Dlod(RFX_depthColor, float4(coords.xy,0,0)).x + 101.0);
}

float3 GetBumpNormals ( in sampler2D colorsampler,float2 texCoord) {

   	float4 lightness = float4(0.333,0.333,0.333,0);
   	// Take all neighbor samples
	float2 offset = PixelSize.xy * 1.5;


   	float4 s00 = tex2D(colorsampler, texCoord + float2(-offset.x, -offset.y));
   	float4 s01 = tex2D(colorsampler, texCoord + float2( 0,   -offset.y));
   	float4 s02 = tex2D(colorsampler, texCoord + float2( offset.x, -offset.y));

  	float4 s10 = tex2D(colorsampler, texCoord + float2(-offset.x,  0));
   	float4 s12 = tex2D(colorsampler, texCoord + float2( offset.x,  0));

   	float4 s20 = tex2D(colorsampler, texCoord + float2(-offset.x,  offset.y));
   	float4 s21 = tex2D(colorsampler, texCoord + float2( 0,    offset.y));
   	float4 s22 = tex2D(colorsampler, texCoord + float2( offset.x,  offset.y));

   	float4 sobelX = s00 + 2 * s10 + s20 - s02 - 2 * s12 - s22;
   	float4 sobelY = s00 + 2 * s01 + s02 - s20 - 2 * s21 - s22;

   	float sx = dot(sobelX, lightness);
   	float sy = dot(sobelY, lightness);

   	return normalize(float3(sx, sy, 1));
}

float3 GetScreenNormals(float2 tex)//get normal vector from depthmap
{
   	float deltax = GetProDepth(float2(tex.x + PixelSize.x, tex.y)) - GetProDepth(float2(tex.x - PixelSize.x, tex.y));
   	float deltay = GetProDepth(float2(tex.x, tex.y + PixelSize.y)) - GetProDepth(float2(tex.x, tex.y - PixelSize.y));   
      
   	return normalize(float3( -(deltax / 2 / PixelSize.x), (deltay / 2 / PixelSize.y) , 1));
}

void PS_RBM_Setup(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdrR : SV_Target0)
{
	float4 res = tex2D(RFX_backbufferColor, texcoord.xy);
	float lineardepth = tex2D(RFX_depthTexColor, texcoord.xy).r;
	res.a = lineardepth;
	hdrR = res;
}

float4 PS_RBM_Execute(float4 vpos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	float4 res = tex2D(SamplerHDR, texcoord.xy);

	if (res.w == 1.0) return res;

	float3 screenNormals = GetScreenNormals(texcoord.xy);
	float3 bumpNormals   = GetBumpNormals(RFX_backbufferColor, texcoord.xy);
	float3 finalNormals  = (screenNormals + bumpNormals*fReflectionReliefHeight);
	finalNormals.y = -finalNormals.y;

	float3 bump = 0;
	float weight = 0;

	for (int i=1; i<=fReflectionSamples; i++)
	{
		float4 tap = tex2Dlod(SamplerHDR, float4(texcoord.xy - finalNormals.xy * PixelSize.xy * (float)i/fReflectionSamples * fReflectionWideness,0,0));
		float diff = smoothstep(0.005,0.0,res.w-tap.w);
		bump += tap.xyz*diff;	
		weight += diff;	
	}

	bump /= weight;


	float3 viewDirection = float3(0.0,0.0,1.0);
	float fresnel = saturate(1-dot(viewDirection,screenNormals));
	fresnel = pow(fresnel,fReflectionFresnelCurve);
	fresnel = lerp(1.0,fresnel, fReflectionFresnelFactor);

	bump.xyz -= res.xyz;
	res.xyz += bump.xyz*fReflectionAmount*fresnel;

	return res;
}

technique RBM_Tech <bool enabled = RFX_Start_Enabled; int toggle = RBM_ToggleKey; >
{
	pass RBMSetupPass
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_RBM_Setup;
		RenderTarget = texHDR;
	}

	pass RBMExecutePass
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_RBM_Execute;
	}
}

#endif

#include MFX_SETTINGS_UNDEF

NAMESPACE_LEAVE()
