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
	[Toggle(_SHLocal)] _SHLocal ("SH Per Pixel", int) = 0
	_SHData ("SH Data", 2D) = "white" {}
	_SHImpact ("SH Impact", float ) = 0.125
	[Toggle(_Obeloid)] _Obeloid( "Obeloid", int ) = 0
	_AlphaCull ("Alpha Cull", float ) = 0.1
	_ColorCull ("Color Cull", float ) = 0.1
	_Squircle( "Square Override", float ) = 0.0
}

SubShader {	

	Pass { 
		Tags { "RenderType"="Opaque" "LightMode"="ForwardBase" }
		Cull Off
		CGPROGRAM

		#pragma multi_compile_local _ _EnablePaintSwatch
		#pragma multi_compile_local _ _UseSH
		#pragma multi_compile_local _ _Obeloid
		#pragma multi_compile_local _ _SHLocal
		
		#include "slapsplat.cginc"
		
		#pragma vertex VertexProgram
		#pragma geometry GeometryProgram
		
		// earlydepthsensor needed for obeloids, note: We could do edge fuzz with the 'alpha' flag.
		#pragma fragment frag earlydepthstencil

		#pragma target 5.0
		#pragma multi_compile_fwdadd_fullshadows

		sampler2D _Swatch;
		
				
		#ifdef _Obeloid
		float4 frag(g2f i, out float outDepth : SV_DepthLessEqual ) : SV_Target
		#else
		float4 frag(g2f i ) : SV_Target
		#endif
		{
			UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX( i );
			
			float4 swatch = 1.0;
			float fakeAlpha = 1.0;
			
			#if !_Obeloid		
				#if _EnablePaintSwatch
							swatch = tex2D( _Swatch, i.tc.xy / 2.0 + 0.5 );
							fakeAlpha = 0.58 - swatch.b;
				#else
							float squircleness = 2.2;
							float2 tc = abs( i.tc.xy );
							float inten = 1.1 - ( pow( tc.x, squircleness ) + pow( tc.y, squircleness ) ) / 1.1;
							inten *= i.color.a;
							inten *= 1.7;
							fakeAlpha = saturate( inten*10.0 - 3.0 );// - chash13( float3( _Time.y, i.tc.xy ) * 100.0 );
				#endif
			#endif


			float3 color;
			#if _Obeloid
				float3 hitworld;
				float3 hitlocal;
				fakeAlpha = ObeloidCalc( i, hitlocal, hitworld );
				color.rgb = i.color;
				clip( fakeAlpha - 0.5 );

				#if _SHLocal
					uint width = 1, height;
					_GeoData.GetDimensions( width, height );
					uint2 coord = uint2( i.id % width, i.id / width );
					float3 shalt = shEvaluate( hitlocal, coord );
					color.rgb += shalt;
				#endif
				
				//Charles way.
				float4 clipPos = mul(UNITY_MATRIX_VP, float4(hitworld, 1.0));
				outDepth = clipPos.z / clipPos.w;
			#else
			clip( fakeAlpha - 0.5 );
			color = pow( i.color.rgb, 1.4 );
			float4 clipPos = mul(UNITY_MATRIX_VP, i.hitworld );
			#endif
			
			
			#if 1 // Shadows, etc.
			
			float attenuation = 1.0;

			struct shadowonly
			{
				float4 pos;
				float4 _LightCoord;
				float4 _ShadowCoord;
			} so;
			so._LightCoord = 0.;
			so.pos = clipPos;
			so._ShadowCoord = 0;
			UNITY_TRANSFER_SHADOW( so, 0. );
			attenuation = LIGHT_ATTENUATION( so );
		

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
			
#if _EnablePaintSwatch && !_Obeloid
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

		#pragma multi_compile_local _ _EnablePaintSwatch
		#pragma multi_compile_local _ _UseSH
		#pragma multi_compile_local _ _Obeloid


		#include "slapsplat.cginc"

		#pragma vertex VertexProgram
		#pragma geometry GeometryProgram
		#pragma fragment frag
		#pragma target 5.0

		sampler2D _Swatch;
		
		#if _Obeloid
		
			struct shadowHelper
			{
				float4 vertex;
				float3 normal;
				V2F_SHADOW_CASTER;
			};

			float4 colOut(shadowHelper data)
			{
				SHADOW_CASTER_FRAGMENT(data);
			}

		#endif

#if _Obeloid
		float4 frag(g2f i, out float outDepth : SV_DepthLessEqual ) : SV_Target
#else
		float4 frag(g2f i) : SV_Target
#endif
		{
			UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX( i );
			float4 swatch = 0.0;
			float fakeAlpha;
			#if _Obeloid
				float3 hitworld;
				float3 hitlocal;
				fakeAlpha = ObeloidCalc( i, hitlocal, hitworld );
				clip( fakeAlpha - 0.5 );
				float4 clipPos = mul(UNITY_MATRIX_VP, float4(hitworld, 1.0));
				
				//D4rkPl4y3r's shadow technique		
				float3 s0 = i.localpos; // Center of obeloid.
				shadowHelper v;
				v.vertex = mul(unity_WorldToObject, float4(hitworld, 1));
				v.normal = normalize(mul((float3x3)unity_WorldToObject, hitworld - s0));
				TRANSFER_SHADOW_CASTER_NOPOS(v, clipPos);

				outDepth = clipPos.z / clipPos.w;
			#else			
				#if _EnablePaintSwatch && !_Obeloid
					swatch = tex2D( _Swatch, i.tc.xy / 2.0 + 0.5 );
					fakeAlpha = .58 - swatch.b;
				#else
					float squircleness = 2.2;
					float2 tc = abs( i.tc.xy );
					float inten = 1.1 - ( pow( tc.x, squircleness ) + pow( tc.y, squircleness ) ) / 1.1;
					inten *= i.color.a;
					inten *= 1.7;
					fakeAlpha = saturate( inten*10.0 - 3.0 );// - chash13( float3( _Time.y, i.tc.xy ) * 100.0 );
				#endif
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