// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'


Shader "SlapSplat/SlapSplat" {
Properties {
	_GeoData ("Geo Data", 2D) = "white" {}
	_OrderData ("Order Data", 2D) = "white" {}
	_Swatch ("Swatch", 2D) = "white" {}
	_CustomScale( "Custom Scale", float) = 1.0
	[Toggle(_EnablePaintSwatch)] _EnablePaintSwatch ( "Enable Paint Swatch", int ) = 0
	_Gamma ( "gamma", float ) = 1.0
	_EmissionAmount( "emission amount", float ) = 0.16
	_Brightness( "brightness", float ) = 1.0
	[Toggle(_UseSH)] _UseSH( "Use SH", int ) = 0
	_SHData ("SH Data", 2D) = "white" {}
	_SHImpact ("SH Impact", float ) = 0.125
}

SubShader {	

	Pass { 
		Tags { "RenderType"="Opaque" "LightMode"="ForwardBase" }
		Cull Off
		CGPROGRAM

		#include "slapsplat.cginc"
		
		#pragma vertex VertexProgram
		#pragma geometry GeometryProgram
		#pragma fragment frag
		#pragma target 5.0
		#pragma multi_compile_fwdadd_fullshadows

		sampler2D _Swatch;
		
		float4 frag(g2f i ) : SV_Target
		{

			UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX( i );
			
#if _EnablePaintSwatch
			float4 swatch = tex2D( _Swatch, i.tc.xy / 2.0 + 0.5 );
			float fakeAlpha = 0.58 - swatch.b;
#else
			float squircleness = 2.2;
			float2 tc = abs( i.tc.xy );
			float inten = 1.1 - ( pow( tc.x, squircleness ) + pow( tc.y, squircleness ) ) / 1.1;
			inten *= i.color.a;
			inten *= 1.7;
			float fakeAlpha = saturate( inten*10.0 - 3.0 );// - chash13( float3( _Time.y, i.tc.xy ) * 100.0 );
#endif
			
			clip( fakeAlpha - 0.5 );
			float3 color = pow( i.color.rgb, 1.4 );
			
			#if 1
			
			float4 clipPos = mul(UNITY_MATRIX_VP, i.hitworld );
			struct shadowonly
			{
				float4 pos;
				float4 _LightCoord;
				float4 _ShadowCoord;
			} so;
			so._LightCoord = 0.;
			so.pos = clipPos;
			so._ShadowCoord  = 0;
			UNITY_TRANSFER_SHADOW( so, 0. );
			float attenuation = LIGHT_ATTENUATION( so );

			if( 1 )
			{
				//GROSS: We actually need to sample adjacent pixels. 
				//sometimes we are behind another object but our color.
				//shows through because of the mask.
				so.pos = clipPos + float4( 1/_ScreenParams.x, 0.0, 0.0, 0.0 );
				UNITY_TRANSFER_SHADOW( so, 0. );
				attenuation = min( attenuation, LIGHT_ATTENUATION( so ) );
				so.pos = clipPos + float4( -1/_ScreenParams.x, 0.0, 0.0, 0.0 );
				/*
				UNITY_TRANSFER_SHADOW( so, 0. );
				attenuation = min( attenuation, LIGHT_ATTENUATION( so ) );
				so.pos = clipPos + float4( 0, 1/_ScreenParams.y, 0.0, 0.0 );
				UNITY_TRANSFER_SHADOW( so, 0. );
				attenuation = min( attenuation, LIGHT_ATTENUATION( so ) );
				so.pos = clipPos + float4( 0, 1/_ScreenParams.y, 0.0, 0.0 );
				UNITY_TRANSFER_SHADOW( so, 0. );
				attenuation = min( attenuation, LIGHT_ATTENUATION( so ) );
				*/
			}
			
			color *= ( attenuation ) * _LightColor0 + i.emission.rgb; 
			#endif
			
#if _EnablePaintSwatch
			color *= (1.0-swatch.ggg*6.0); 
#endif
			return max( float4( color, 1.0 ), 0 );
		}
		ENDCG
	}



	Pass {
		Tags {"LightMode" = "ShadowCaster"}
		Cull Off
		//AlphaToMask On , out uint mask : SV_Coverage
		CGPROGRAM

		#include "slapsplat.cginc"

		#pragma vertex VertexProgram
		#pragma geometry GeometryProgram
		#pragma fragment frag
		#pragma target 5.0

		sampler2D _Swatch;

		float4 frag(g2f i ) : SV_Target
		{
			UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX( i );

#if _EnablePaintSwatch
			float4 swatch = tex2D( _Swatch, i.tc.xy / 2.0 + 0.5 );
			float fakeAlpha = .58 - swatch.b;
#else
			float squircleness = 2.2;
			float2 tc = abs( i.tc.xy );
			float inten = 1.1 - ( pow( tc.x, squircleness ) + pow( tc.y, squircleness ) ) / 1.1;
			inten *= i.color.a;
			inten *= 1.7;
			float fakeAlpha = saturate( inten*10.0 - 3.0 );// - chash13( float3( _Time.y, i.tc.xy ) * 100.0 );
#endif

			clip( fakeAlpha - 0.5 );
			//mask = ( 1u << ((uint)(fakeAlpha*GetRenderTargetSampleCount() + 0.5)) ) - 1;
			//if( fakeAlpha > 0.5 ) mask = 0xffff; else mask = 0x0000;
			return 1.0;
		}
		ENDCG
	}
}
}