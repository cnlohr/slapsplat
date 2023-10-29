// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'


Shader "SlapSplat/SlapSplat" {
Properties {
	_GeoData ("Geo Data", 2D) = "white" {}
	_OrderData ("Order Data", 2D) = "white" {}
	_Swatch ("Swatch", 2D) = "white" {}
	_CustomScale( "Custom Scale", float) = 1.0
	[Toggle(_EnablePaintSwatch)] _EnablePaintSwatch ( "Enable Paint Swatch", int ) = 0
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
			#include "Packages/com.llealloo.audiolink/Runtime/Shaders/AudioLink.cginc"

			#include "UnityCG.cginc"
			#include "AutoLight.cginc"
			#include "Lighting.cginc"
			#include "UnityShadowLibrary.cginc"
			#include "UnityPBSLighting.cginc"
			
			uniform float _CustomScale;
			
			#pragma multi_compile_local _ _EnablePaintSwatch

			struct appdata {
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};

			struct v2g {
				int nv : ID;
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};
			
			struct g2f {
				float4 vertex : SV_POSITION;
				sample float2 tc : TEXCOORD0; // "this force multisampling" Thanks, Bgolus https://forum.unity.com/threads/questions-on-multi-sampling.1398895/ 
				float4 color : COLOR;
				float4 hitworld : HIT0;
				float3 emission : EMISSION;
				UNITY_FOG_COORDS(1)
				UNITY_VERTEX_OUTPUT_STEREO
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};
			
			texture2D< float4 > _GeoData;
			texture2D< float > _OrderData;
			
			float3 Rotate( float3 v, float4 rot )
			{
				// Actually more of a counterrotate
				//return v + 2.0 * cross( q.xyz, cross(q.xyz, v) + q.w * v);					
				float3x3  R = {
					1.0 - 2.0 * (rot[2] * rot[2] + rot[3] * rot[3]),
					2.0 * (rot[1] * rot[2] + rot[0] * rot[3]),
					2.0 * (rot[1] * rot[3] - rot[0] * rot[2]),

					2.0 * (rot[1] * rot[2] - rot[0] * rot[3]),
					1.0 - 2.0 * (rot[1] * rot[1] + rot[3] * rot[3]),
					2.0 * (rot[2] * rot[3] + rot[0] * rot[1]),

					2.0 * (rot[1] * rot[3] + rot[0] * rot[2]),
					2.0 * (rot[2] * rot[3] - rot[0] * rot[1]),
					1.0 - 2.0 * (rot[1] * rot[1] + rot[2] * rot[2]),
				};
				return mul( v, R);
			}
			
			float3 Transform( float3 pos, float3 scale, float4 rot, float2 h, inout float opacity )
			{
				// from  https://github.com/antimatter15/splat/blob/main/main.js

				rot = normalize( rot.xyzw );
				float3 fvv = float3( h.x, h.y, 0.0 );

				fvv *= scale;
				float3 hprime = Rotate( fvv, rot );

				return pos + hprime;
			}


			void mathQuatApply(out float4 qout, const float4 q1, const float4 q2)
			{
				// NOTE: Does not normalize - you will need to normalize eventually.
				float tmpw, tmpx, tmpy;
				tmpw    = (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z) - (q1.w * q2.w);
				tmpx    = (q1.x * q2.y) + (q1.y * q2.x) + (q1.z * q2.w) - (q1.w * q2.z);
				tmpy    = (q1.x * q2.z) - (q1.y * q2.w) + (q1.z * q2.x) + (q1.w * q2.y);
				qout.w = (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y) + (q1.w * q2.x);
				qout.z = tmpy;
				qout.y = tmpx;
				qout.x = tmpw;
			}
			
			float dotfloat4(float4 a, float4 b)
			{
				return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
			}

			float4 slerp(float4 v0, float4 v1, float t)
			{
				// Compute the cosine of the angle between the two vectors.
				float dot = dotfloat4(v0, v1);

				const float DOT_THRESHOLD = 0.9995;
				if (abs(dot) > DOT_THRESHOLD)
				{
					// If the inputs are too close for comfort, linearly interpolate
					// and normalize the result.
					float4 result = v0 + t * (v1 - v0);
					normalize(result);
					return result;
				}

				// If the dot product is negative, the quaternions
				// have opposite handed-ness and slerp won't take
				// the shorter path. Fix by reversing one quaternion.
				if (dot < 0.0f)
				{
					v1 = -v1;
					dot = -dot;
				}

				clamp(dot, -1, 1); // Robustness: Stay within domain of acos()
				float theta_0 = acos(dot); // theta_0 = angle between input vectors
				float theta = theta_0 * t; // theta = angle between v0 and result 

				float4 v2 = v1 - v0 * dot;
				normalize(v2); // { v0, v2 } is now an orthonormal basis

				return v0 * cos(theta) + v2 * sin(theta);
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

			[maxvertexcount(4)]
			void GeometryProgram(point v2g p[1], inout TriangleStream<g2f> stream, uint inprim : SV_PrimitiveID )
			{
				g2f o[4];
				int prim = inprim;
				float3 viewDir = -UNITY_MATRIX_IT_MV[2].xyz; // Camera Forward. 
				float3 viewPos = mul(float4( 0., 0., 0., 1. ), UNITY_MATRIX_IT_MV ).xyz;
				
				int dir = 0;
				uint widthOrder = 1, heightOrder;
				_OrderData.GetDimensions( widthOrder, heightOrder );
				if( widthOrder > 1 && true )
				{
					// Figure out cardinal direction.
					float3 absd = abs( viewDir );
					if( absd.x > absd.y )
					{
						if( absd.x > absd.z )
						{
							if( viewDir.x > 0 )
								dir = 0;
							else
								dir = 1;
						}
						else
						{
							if( viewDir.z > 0 )
								dir = 4;
							else
								dir = 5;
						}
					}
					else
					{
						if( absd.y > absd.z )
						{
							if( viewDir.y > 0 )
								dir = 2;
							else
								dir = 3;
						}
						else
						{
							if( viewDir.z > 0 )
								dir = 4;
							else
								dir = 5;
						}
					}
					uint2 coord = uint2( inprim % widthOrder, inprim / widthOrder + dir * heightOrder / 6 );
					prim = asuint( _OrderData.Load( int3( coord, 0 ) ) );
				}

				UNITY_SETUP_INSTANCE_ID(o[0]); UNITY_INITIALIZE_OUTPUT(g2f, o[0]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[0]);
				UNITY_SETUP_INSTANCE_ID(o[1]); UNITY_INITIALIZE_OUTPUT(g2f, o[1]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[1]);
				UNITY_SETUP_INSTANCE_ID(o[2]); UNITY_INITIALIZE_OUTPUT(g2f, o[2]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[2]);
				UNITY_SETUP_INSTANCE_ID(o[3]); UNITY_INITIALIZE_OUTPUT(g2f, o[3]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[3]);

				o[0].tc = float2( -1.0, -1.0 );
				o[1].tc = float2( 1.0, -1.0 );
				o[2].tc = float2( -1.0, 1.0 );
				o[3].tc = float2( 1.0, 1.0 );


				uint width = 1, height;
				_GeoData.GetDimensions( width, height );
				uint2 coord = uint2( prim % width, prim / width );

				uint4 gpdata = asuint( _GeoData.Load( int3( coord, 0 ) ) );

				int3 gpv = int3( gpdata.x << 16, gpdata.x & 0xffff0000, gpdata.y << 16 ) >> 16;
				float3 vp = float3( gpv / 1000.0 );

				// Make the cards fall over time.
				float ntime = _Time.y;
				if (AudioLinkGetVersionMajor() > 0)
				{
				    uint time = AudioLinkDecodeDataAsUInt(ALPASS_GENERALVU_NETWORK_TIME) & 0x7ffffff;
					ntime = float(time / 1000) + float( time % 1000 ) / 1000.; 
				}
				float thisTime = glsl_mod( ntime + glsl_mod( prim, 30000.0 ), 30000.0 );
				float thisFall = 29600-thisTime;
				thisFall *= 0.5;
				thisFall = max( -thisFall, 0 );
				vp.z -= thisFall;
				
				// Cull out splats behind view.
				//if( dot( vp - viewPos, viewDir ) < 0 ) return;

				uint3 scaleraw = uint3( ( gpdata.y & 0xff0000 )>>16, (gpdata.y >> 24), (gpdata.z & 0xff) );
				float3 scale = exp(( float3(scaleraw) - 0.5) / 32.0 - 7.0);
				scale *= _CustomScale;
				//Force overflow to fix signed issue.
				int3 rotraw = int3( ( gpdata.z >> 8 ) & 0xff, (gpdata.z >> 16) & 0xff, gpdata.z >> 24 ) * 16777216;
				float4 rot = float4( 0.0, rotraw / ( 127.0 * 16777216.0 ) );
				rot.x = sqrt(1.0 - dot( rot.yzw, rot.yzw ));
				float4 color = float4( uint3( ( gpdata.w >> 0) & 0xff, (gpdata.w >> 8) & 0xff, (gpdata.w >> 16) & 0xff ) / 255.0, 1.0 );

				// If falling, then rotate.
				float4 nrot = normalize( chash41( prim ) );
				rot = slerp( rot, nrot, thisFall );

				float3 fnorm = float3( 0.0, 0.0, 1.0 );
				float fn = csimplex3( vp*0.9+100.0 );
				fnorm = Rotate( fnorm, rot );
				// Random
				//vp += AudioLinkLerpMultiline(ALPASS_DFT + float2(prim%128, 0)) * fnorm * 0.01;
				
				// Consistent
				//vp += AudioLinkLerpMultiline(ALPASS_DFT + float2(fn*128*0.5, 0)) * fnorm * 0.02;
				
				vp += AudioLinkData(ALPASS_AUTOCORRELATOR + int2( abs( 128 - glsl_mod(fn*128+ntime*8.0, 256.0) ), 0)).xxx * fnorm * 0.002;
				
				
				// Compute emission
				float bnw = ( color.r + color.g + color.b ) / 3.0;
				float3 emission = 0.0;//saturate( ( length( saturate((color.rgb - bnw) * float3( 0.1, 0.1, 2.0 )) ) - .3 ) * 100.0 ) * color;
				emission += color * 0.16;

				o[0].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float2( -1, -1 ), color.a ), 1.0 ) );
				o[1].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float2( -1,  1 ), color.a ), 1.0 ) );
				o[2].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float2(  1, -1 ), color.a ), 1.0 ) );
				o[3].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float2(  1,  1 ), color.a ), 1.0 ) );
				o[0].vertex = mul(UNITY_MATRIX_VP, o[0].hitworld );
				o[1].vertex = mul(UNITY_MATRIX_VP, o[1].hitworld );
				o[2].vertex = mul(UNITY_MATRIX_VP, o[2].hitworld );
				o[3].vertex = mul(UNITY_MATRIX_VP, o[3].hitworld );
				
				o[0].color = color;
				o[1].color = color;
				o[2].color = color;
				o[3].color = color;
				
				o[0].emission = emission;
				o[1].emission = emission;
				o[2].emission = emission;
				o[3].emission = emission;
				
				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(o[0]);
				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(o[1]);
				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(o[2]);
				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(o[3]);

				stream.Append(o[0]);
				stream.Append(o[1]);
				stream.Append(o[2]);
				stream.Append(o[3]);
			}
			
			float4 calcfrag(g2f i, out uint mask, float rands )
			{

			}

	ENDCG



	Pass {
		Tags {"LightMode" = "ShadowCaster"}
		Cull Off
		//AlphaToMask On , out uint mask : SV_Coverage
		CGPROGRAM

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
	

	Pass { 
		Tags { "RenderType"="Opaque" "LightMode"="ForwardBase" }
		Cull Off
		CGPROGRAM
		
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
			return float4( color, 1.0 );
		}
		ENDCG
	}
}
}