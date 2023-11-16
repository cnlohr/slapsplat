Shader "Gaussonomy/Stage1" {
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
		Tags { "RenderType"="Opaque" }
		Cull Off
		CGPROGRAM
		#define _NEED_IDS

		#include "../slapsplat/slapsplat.cginc"
		
		#pragma vertex VertexProgram
		#pragma geometry GeometryProgram
		#pragma fragment frag
		#pragma target 5.0

		sampler2D _Swatch;
		
		struct MRTOut
		{
			float4 targ0 : COLOR0;
			float4 targ1 : COLOR1;
			float4 targ2 : COLOR2;
			float4 targ3 : COLOR3;
			float4 targ4 : COLOR4;
			float4 targ5 : COLOR5;
			float4 targ6 : COLOR6;
			float4 targ7 : COLOR7;
		};

		MRTOut frag(g2f i ) : SV_Target
		{
			UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX( i );
			
			float squircleness = 2.2;
			float2 tc = abs( i.tc.xy );
			float inten = 1.1 - ( pow( tc.x, squircleness ) + pow( tc.y, squircleness ) ) / 1.1;
			inten *= 1.7;
			float fakeAlpha = saturate( inten*10.0 - 3.0 );// - chash13( float3( _Time.y, i.tc.xy ) * 100.0 );
			
			clip( fakeAlpha - 0.5 );
			float4 color = i.id / 2000000.0;
			MRTOut m;
			m.targ0 = color;
			m.targ1 = 1.0;
			m.targ2 = 1.5;
			m.targ3 = color;
			m.targ4 = .1;
			m.targ5 = .2;
			m.targ6 = .3;
			m.targ7 = .4;
			return m;
		}
		ENDCG
	}
}
}