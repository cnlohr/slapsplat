
Shader "SlapSplat/SlapSplat" {
Properties {
	_GeoData ("Geo Data", 3D) = "white" {}
}

SubShader {

	CGINCLUDE
			#include "UnityCG.cginc"
			#pragma vertex VertexProgram
			#pragma geometry GeometryProgram
			#pragma fragment frag
			#pragma target 5.0
			#pragma multi_compile_fwdadd_fullshadows

			#include "Assets/cnlohr/Shaders/hashwithoutsine/hashwithoutsine.cginc"

			#include "UnityCG.cginc"
			#include "AutoLight.cginc"
			#include "Lighting.cginc"
			#include "UnityShadowLibrary.cginc"
			#include "UnityPBSLighting.cginc"
			
			struct appdata {
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};

			struct v2g {
				int nv : ID;
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};
			
			struct g2f {
				float4 vertex : SV_POSITION;
				float2 tc : TEXCOORD0;
				float4 color : COLOR;
				float4 hitworld : HIT0;
				UNITY_FOG_COORDS(1)
				UNITY_VERTEX_OUTPUT_STEREO
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};
			
			texture3D< float4 > _GeoData;
			
			float3 Rotate( float3 v, float4 q )
			{
				// Actually more of a counterrotate
				return v + 2.0 * cross( q.xyz, cross(q.xyz, v) + q.w * v);
			}
			
			float3 Transform( float3 pos, float3 scale, float4 rot, float2 h )
			{
				rot = rot.xyzw;
				rot.w *= -1;
				rot = rot.yzwx;
				rot = normalize( rot );
				//scale = 1.0/(1.0+exp( -scale ));
				scale = exp(scale);
				scale *= 3.0;
				float3 hprime = Rotate( float3( h.x, 0.0, h.y ), rot );
				hprime *= scale;
				return pos + hprime;
			}

			v2g VertexProgram ( appdata v, uint vid : SV_VertexID )
			{
				UNITY_SETUP_INSTANCE_ID( v );
				v2g t;
				UNITY_INITIALIZE_OUTPUT(v2g, t);
				UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o);
				t.nv = vid;
				return t;
			}

			[maxvertexcount(6)]
			void GeometryProgram(point v2g p[1], inout TriangleStream<g2f> stream, uint prim : SV_PrimitiveID )
			{
				g2f o[4];

				UNITY_SETUP_INSTANCE_ID(o[0]); UNITY_INITIALIZE_OUTPUT(g2f, o[0]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[0]);
				UNITY_SETUP_INSTANCE_ID(o[1]); UNITY_INITIALIZE_OUTPUT(g2f, o[1]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[1]);
				UNITY_SETUP_INSTANCE_ID(o[2]); UNITY_INITIALIZE_OUTPUT(g2f, o[2]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[2]);
				UNITY_SETUP_INSTANCE_ID(o[3]); UNITY_INITIALIZE_OUTPUT(g2f, o[3]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[3]);

				o[0].tc = float2( -1.0, -1.0 );
				o[1].tc = float2( 1.0, -1.0 );
				o[2].tc = float2( -1.0, 1.0 );
				o[3].tc = float2( 1.0, 1.0 );

				uint width = 1, height, depth, levels;
				_GeoData.GetDimensions( 0, width, height, depth, levels );
				uint2 coord = uint2( prim % height, prim / height );

				float4 vp = _GeoData.Load( uint4( 0, coord, 0 ) );
				float4 scale = _GeoData.Load( uint4( 1, coord, 0 ) );
				float4 color = _GeoData.Load( uint4( 2, coord, 0 ) );
				float4 rot = _GeoData.Load( uint4( 3, coord, 0 ) );
				vp.xy = vp;

				o[0].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float2( -1, -1 ) ), 1.0 ) );
				o[1].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float2( -1,  1 ) ), 1.0 ) );
				o[2].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float2(  1, -1 ) ), 1.0 ) );
				o[3].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float2(  1,  1 ) ), 1.0 ) );
				o[0].vertex = mul(UNITY_MATRIX_VP, o[0].hitworld );
				o[1].vertex = mul(UNITY_MATRIX_VP, o[1].hitworld );
				o[2].vertex = mul(UNITY_MATRIX_VP, o[2].hitworld );
				o[3].vertex = mul(UNITY_MATRIX_VP, o[3].hitworld );
				
				o[0].color = color;
				o[1].color = color;
				o[2].color = color;
				o[3].color = color;

				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(o[0]);
				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(o[1]);
				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(o[2]);
				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(o[3]);

				stream.Append(o[0]);
				stream.Append(o[1]);
				stream.Append(o[2]);
				stream.Append(o[1]);
				stream.Append(o[2]);
				stream.Append(o[3]);
			}
			
			float4 frag(g2f i, out uint mask : SV_Coverage ) : SV_Target
			{
				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX( i );
				float inten = 1.1 - length( i.tc.xy ) / 1.1;
				inten *= i.color.a;
				inten *= 3.0;
				float fakeAlpha = inten - chash13( float3( _Time.y, i.tc.xy ) * 100.0 );
				mask = ( 1u << ((uint)(fakeAlpha*GetRenderTargetSampleCount() + 0.5)) ) - 1;
				
				
				// TODO: Fix shadow.
				
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
					UNITY_TRANSFER_SHADOW( so, 0. );
					attenuation = min( attenuation, LIGHT_ATTENUATION( so ) );
					so.pos = clipPos + float4( 0, 1/_ScreenParams.y, 0.0, 0.0 );
					UNITY_TRANSFER_SHADOW( so, 0. );
					attenuation = min( attenuation, LIGHT_ATTENUATION( so ) );
					so.pos = clipPos + float4( 0, 1/_ScreenParams.y, 0.0, 0.0 );
					UNITY_TRANSFER_SHADOW( so, 0. );
					attenuation = min( attenuation, LIGHT_ATTENUATION( so ) );
				}
				
				float3 color = i.color.rgb;
				//color = i.hitworld.rgb;
				color *= ( attenuation * 0.5 + 0.5); 
				return float4( color, 1.0 );
			}

	ENDCG



	Pass {
		Tags {"LightMode" = "ShadowCaster"}
		Cull Off
		AlphaToMask On
		CGPROGRAM
		ENDCG
	}
	

	Pass { 
		Tags { "RenderType"="Opaque" "LightMode"="ForwardBase" }
		Cull Off
		AlphaToMask On
		CGPROGRAM
		ENDCG
	}
}
}