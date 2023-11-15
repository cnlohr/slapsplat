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
			#pragma multi_compile_local _ _UseSH

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
			texture2D< float4 > _SHData;
			
			float _Gamma;
			float _EmissionAmount;
			float _Brightness;
			float _SHImpact;
			
			// Only compute 3 levels of SH.
			#define myL 3
			#define myT

			// https://github.com/kayru/Probulator (Copyright (c) 2015 Yuriy O'Donnell, MIT)
			float3 shEvaluate(const float3 p, const float3 shvals[16] )
			{
				// https://github.com/dariomanesku/cmft/blob/master/src/cmft/cubemapfilter.cpp#L130 (BSD)
				// * Copyright 2014-2015 Dario Manesku. All rights reserved.
				// * License: http://www.opensource.org/licenses/BSD-2-Clause

				// From Peter-Pike Sloan's Stupid SH Tricks
				// http://www.ppsloan.org/publications/StupidSH36.pdf

				const float PI  = 3.1415926535897932384626433832795;
				const float PIH = 1.5707963267948966192313216916398;
				const float sqrtPI = 1.7724538509055160272981674833411; //sqrt(PI)

				float3 res = 0;

				float x = -p.x;
				float y = -p.y;
				float z = p.z;

				float x2 = x*x;
				float y2 = y*y;
				float z2 = z*z;

				float z3 = z2*z;

				float x4 = x2*x2;
				float y4 = y2*y2;
				float z4 = z2*z2;

				int i = 0;
				res = 0.0;

				#if (myL >= 1)
					res += myT(-sqrt(3.0f/(4.0f*PI))*y ) * shvals[1];
					res += myT( sqrt(3.0f/(4.0f*PI))*z ) * shvals[2];
					res += myT(-sqrt(3.0f/(4.0f*PI))*x ) * shvals[3];
				#endif

				#if (myL >= 2)
					res += myT( sqrt(15.0f/(4.0f*PI))*y*x ) * shvals[4];
					res += myT(-sqrt(15.0f/(4.0f*PI))*y*z ) * shvals[5];
					res += myT( sqrt(5.0f/(16.0f*PI))*(3.0f*z2-1.0f) ) * shvals[6];
					res += myT(-sqrt(15.0f/(4.0f*PI))*x*z ) * shvals[7];
					res += myT( sqrt(15.0f/(16.0f*PI))*(x2-y2) ) * shvals[8];
				#endif

				#if (myL >= 3)
					res += myT(-sqrt( 70.0f/(64.0f*PI))*y*(3.0f*x2-y2) ) * shvals[9];
					res += myT( sqrt(105.0f/ (4.0f*PI))*y*x*z ) * shvals[10];
					res += myT(-sqrt( 21.0f/(16.0f*PI))*y*(-1.0f+5.0f*z2) ) * shvals[11];
					res += myT( sqrt(  7.0f/(16.0f*PI))*(5.0f*z3-3.0f*z) ) * shvals[12];
					res += myT(-sqrt( 42.0f/(64.0f*PI))*x*(-1.0f+5.0f*z2) ) * shvals[13];
					res += myT( sqrt(105.0f/(16.0f*PI))*(x2-y2)*z ) * shvals[14];
					res += myT(-sqrt( 70.0f/(64.0f*PI))*x*(x2-3.0f*y2) ) * shvals[15];
				#endif

				#if (myL >= 4)
					res += myT( 3.0f*sqrt(35.0f/(16.0f*PI))*x*y*(x2-y2) ) * shvals[16];
					res += myT(-3.0f*sqrt(70.0f/(64.0f*PI))*y*z*(3.0f*x2-y2) ) * shvals[17];
					res += myT( 3.0f*sqrt( 5.0f/(16.0f*PI))*y*x*(-1.0f+7.0f*z2) ) * shvals[18];
					res += myT(-3.0f*sqrt(10.0f/(64.0f*PI))*y*z*(-3.0f+7.0f*z2) ) * shvals[19];
					res += myT( (105.0f*z4-90.0f*z2+9.0f)/(16.0f*sqrtPI) ) * shvals[20];
					res += myT(-3.0f*sqrt(10.0f/(64.0f*PI))*x*z*(-3.0f+7.0f*z2) ) * shvals[21];
					res += myT( 3.0f*sqrt( 5.0f/(64.0f*PI))*(x2-y2)*(-1.0f+7.0f*z2) ) * shvals[22];
					res += myT(-3.0f*sqrt(70.0f/(64.0f*PI))*x*z*(x2-3.0f*y2) ) * shvals[23];
					res += myT( 3.0f*sqrt(35.0f/(4.0f*(64.0f*PI)))*(x4-6.0f*y2*x2+y4) ) * shvals[24];
				#endif
				
				
				res *= _SHImpact;
				//XXX NOTE: The proper way is to use this conversion, but we already have a better base color parameter.
				//res =  myT( 1.0f/(2.0f*sqrtPI) ) * shvals[0];
				res += shvals[0];


				return res;
			}

			
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
				UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(t);
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
				float dotsq = dot( rot.yzw, rot.yzw );
				if( dotsq < 1.0 )
					rot.x = sqrt(1.0 - dotsq);
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
				
				vp += AudioLinkData(ALPASS_AUTOCORRELATOR + int2( abs( 128 - glsl_mod(fn*128+ntime*8.0, 256.0) ), 0)).xxx * fnorm * 0.004;
				
#if _UseSH
				//#if defined(USING_STEREO_MATRICES)
				//	float3 PlayerCenterCamera = ( unity_StereoWorldSpaceCameraPos[0] + unity_StereoWorldSpaceCameraPos[1] ) / 2;
				//#else
					float3 PlayerCenterCamera = _WorldSpaceCameraPos.xyz;
				//#endif

				uint j = 0;
				float3 shdata[16];
				float3 centerWorld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float2( 0, 0 ), color.a ), 1.0 ) );

				for( j = 1; j < 16; j++ )
					shdata[j] = _SHData.Load( int3( coord*4+int2(j%4, j/4), 0 ) ) - 0.5;

				shdata[0] = color; // Use base color.

				float3 view = centerWorld.xyz - PlayerCenterCamera;
				view = normalize( view );
				
				view = mul( unity_WorldToObject, view );
				color.rgb = shEvaluate( view, shdata );
				//color.rgb = (abs( shdata[7] ) );
#endif	

				color = pow( color * _Brightness, _Gamma );
				// Compute emission
				float bnw = ( color.r + color.g + color.b ) / 3.0;
				float3 emission = 0.0;//saturate( ( length( saturate((color.rgb - bnw) * float3( 0.1, 0.1, 2.0 )) ) - .3 ) * 100.0 ) * color;
				emission += color * _EmissionAmount;
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
			return max( float4( color, 1.0 ), 0 );
		}
		ENDCG
	}
}
}