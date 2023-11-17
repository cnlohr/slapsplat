/*
	_UseSH = Perform SH Calculations (requires SH texture)
	_NEED_IDS = Output IDs, instead of colors in the 
	_Obeloid = Output as Obeloid, use GeometryProgramObeloid instead of GeometryProgram
	
	GeometryProgram is the geo shader.
*/
			#include "UnityCG.cginc"
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
			uniform float _AlphaCull, _ColorCull;
			float _Squircle;

			struct appdata {
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};

			struct v2g {
				int nv : ID;
				int dir : DIR;
				UNITY_VERTEX_OUTPUT_STEREO
			};
			
			struct g2f {
				float4 vertex : SV_POSITION;
				#if _Obeloid
				float3 localpt : LOCALPT;
				float3 campt : CAMPT;
				float3 campd : CAMPD;
				float3 camwithorthoworld : CWOW;
				nointerpolation float3 localscale : LSCALE;
				nointerpolation float4 localrot : LROT;
				nointerpolation float3 localpos : LPOS;
				#else
				/*sample*/ float2 tc : TEXCOORD0; // sample can "force multisampling" Thanks, Bgolus https://forum.unity.com/threads/questions-on-multi-sampling.1398895/ 
				#endif
				
				#if defined( _NEED_IDS  ) || _SHLocal
					nointerpolation uint id : ID;
				#endif

				#ifndef _NEED_IDS 
					float4 color : COLOR;
					float3 emission : EMISSION;
				#endif
				float4 hitworld : HIT0;
				UNITY_FOG_COORDS(1)
				UNITY_VERTEX_OUTPUT_STEREO
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};
			
			texture2D< float4 > _GeoData;
			texture2D< float > _OrderData;
			texture2D< float3 > _SHData;
			
			float _Gamma;
			float _EmissionAmount;
			float _Brightness;
			float _SHImpact;
			
			
			#if 1
			// Only compute 3 levels of SH.
			#define myL 3
			#define myT

			// XXX XXX XXX THIS APPEARS TO NOT BE USED
			// https://github.com/kayru/Probulator (Copyright (c) 2015 Yuriy O'Donnell, MIT)
			float3 shEvaluate(const float3 p, uint2 coord )
			{
				float3 shvals[16];
				uint j = 0;
				for( j = 0; j < 16; j++ )
				{
					float3 v = ( _SHData.Load( int3( coord*4+int2( j%4, j/4), 0 ) ));
					v = v * 1.0;
					v = v - 0.5;
					v = sign(v) * v * v;
					shvals[j] = v * _SHImpact;
				}
				shvals[0] = 0; // DC = 0;

				// https://github.com/dariomanesku/cmft/blob/master/src/cmft/cubemapfilter.cpp#L130 (BSD)
				// * Copyright 2014-2015 Dario Manesku. All rights reserved.
				// * License: http://www.opensource.org/licenses/BSD-2-Clause

				// From Peter-Pike Sloan's Stupid SH Tricks
				// http://www.ppsloan.org/publications/StupidSH36.pdf

				const float PI  = 3.1415926535897932384626433832795;
				const float PIH = 1.5707963267948966192313216916398;
				const float sqrtPI = 1.7724538509055160272981674833411; //sqrt(PI)

				float3 res = 0;

				float x = p.x;
				float y = p.y;
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
				//res += shvals[0];
				
				/*
					// The original code:
					#define readSH( x, y ) shdata[y+1]
					
					//ivec2 shIndex = ivec2((index << 4u) & INDEX_MASK, index >> (INDEX_SHIFT - 4u));
					float3 sh10 = readSH(shIndex, 0);
					float3 sh11 = readSH(shIndex, 1);
					float3 sh12 = readSH(shIndex, 2);
					float3 sh20 = readSH(shIndex, 3);
					float3 sh21 = readSH(shIndex, 4);
					float3 sh22 = readSH(shIndex, 5);
					float3 sh23 = readSH(shIndex, 6);
					float3 sh24 = readSH(shIndex, 7);
					float3 sh30 = readSH(shIndex, 8);
					float3 sh31 = readSH(shIndex, 9);
					float3 sh32 = readSH(shIndex, 10);
					float3 sh33 = readSH(shIndex, 11);
					float3 sh34 = readSH(shIndex, 12);
					float3 sh35 = readSH(shIndex, 13);
					float3 sh36 = readSH(shIndex, 14);
					// Formula and constants from
					// https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/59f5f77e3ddbac3ed9db93ec2cfe99ed6c5d121d/cuda_rasterizer/forward.cu and
					// https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/59f5f77e3ddbac3ed9db93ec2cfe99ed6c5d121d/cuda_rasterizer/auxiliary.h
					// See also https://en.wikipedia.org/wiki/Table_of_spherical_harmonics
					float3 shcontrib = -0.4886025119029199f * ddir.y * sh10                                                                       //
							   + 0.4886025119029199f * ddir.z * sh11                                                                      //
							   + -0.4886025119029199f * ddir.x * sh12                                                                     //
							   + 1.0925484305920792f * ddir.x * ddir.y * sh20                                                              //
							   + -1.0925484305920792f * ddir.y * ddir.z * sh21                                                             //
							   + 0.31539156525252005f * (2.0 * ddir.z * ddir.z - ddir.x * ddir.x - ddir.y * ddir.y) * sh22                     //
							   + -1.0925484305920792f * ddir.x * ddir.z * sh23                                                             //
							   + 0.5462742152960396f * (ddir.x * ddir.x - ddir.y * ddir.y) * sh24                                            //
							   + -0.5900435899266435f * ddir.y * (3.0 * ddir.x * ddir.x - ddir.y * ddir.y) * sh30                             //
							   + 2.890611442640554f * ddir.x * ddir.y * ddir.z * sh31                                                       //
							   + -0.4570457994644658f * ddir.y * (4.0 * ddir.z * ddir.z - ddir.x * ddir.x - ddir.y * ddir.y) * sh32             //
							   + 0.3731763325901154f * ddir.z * (2.0 * ddir.z * ddir.z - 3.0 * ddir.x * ddir.x - 3.0 * ddir.y * ddir.y) * sh33  //
							   + -0.4570457994644658f * ddir.x * (4.0 * ddir.z * ddir.z - ddir.x * ddir.x - ddir.y * ddir.y) * sh34             //
							   + 1.445305721320277f * ddir.z * (ddir.x * ddir.x - ddir.y * ddir.y) * sh35                                     //
							   + -0.5900435899266435f * ddir.x * (ddir.x * ddir.x - 3.0 * ddir.y * ddir.y) * sh36                             //
					  ;
					color.rgb += shcontrib.rgb;
					color.rgb = -(color.rgb - shalt);
					*/


				return res;
			}
			#endif

			// Oddly, quats stored as W XYZ as XYZW
			float3 Rotate( float3 v, float4 rot )
			{
				return v + 2.0 * cross(rot.yzw, cross(rot.yzw, v) + rot.x * v);
			}
			
			float3 Transform( float3 pos, float3 scale, float4 rot, float3 h )
			{
				// from  https://github.com/antimatter15/splat/blob/main/main.js
				float3 hprime = Rotate( h * scale, rot );
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

				float3 viewDir = -UNITY_MATRIX_IT_MV[2].xyz; // Camera Forward. 

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
				}

				t.dir = dir;
				return t;
			}
			
			#if _Obeloid
				//  +---+---+
				//  |1/2|5/6|
				//  +---E---R
				//      |7/8|
				//		+---+
				#define V_OUT  10
			#else
				#define V_OUT 4
			#endif

			[maxvertexcount(V_OUT)]
			void GeometryProgram(point v2g p[1], inout TriangleStream<g2f> stream, uint inprim : SV_PrimitiveID )
			{
				int k;
				g2f o[V_OUT];
				int prim = inprim;
				float3 viewDir = -UNITY_MATRIX_IT_MV[2].xyz; // Camera Forward. 
				float3 viewPos = mul(float4( 0., 0., 0., 1. ), UNITY_MATRIX_IT_MV ).xyz;

				uint widthOrder = 1, heightOrder;
				_OrderData.GetDimensions( widthOrder, heightOrder );
				uint2 orderCoord = uint2( inprim % widthOrder, inprim / widthOrder + p[0].dir * heightOrder / 6 );
				prim = asuint( _OrderData.Load( int3( orderCoord, 0 ) ) );

				UNITY_SETUP_INSTANCE_ID(o[0]); UNITY_INITIALIZE_OUTPUT(g2f, o[0]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[0]);
				UNITY_SETUP_INSTANCE_ID(o[1]); UNITY_INITIALIZE_OUTPUT(g2f, o[1]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[1]);
				UNITY_SETUP_INSTANCE_ID(o[2]); UNITY_INITIALIZE_OUTPUT(g2f, o[2]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[2]);
				UNITY_SETUP_INSTANCE_ID(o[3]); UNITY_INITIALIZE_OUTPUT(g2f, o[3]); UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o[3]);

				#if _Obeloid
				#else
					o[0].tc = float2( -1.0, -1.0 );
					o[1].tc = float2( 1.0, -1.0 );
					o[2].tc = float2( -1.0, 1.0 );
					o[3].tc = float2( 1.0, 1.0 );
				#endif

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
				
				// XXX TODO XXX XXX REMOVE ME AND FIX THE REAL PROBLEM WHAT THE HECK
				scale = max( length (0.03 * scale), scale );
				
				//Force overflow to fix signed issue.
				int3 rotraw = int3( ( gpdata.z >> 8 ) & 0xff, (gpdata.z >> 16) & 0xff, gpdata.z >> 24 ) * 16777216;
				float4 rot = float4( 0.0, rotraw / ( 127.0 * 16777216.0 ) );
				float dotsq = dot( rot.yzw, rot.yzw );
				if( dotsq < 1.0 )
					rot.x = sqrt(1.0 - dotsq);
				float4 color = float4( uint3( ( gpdata.w >> 0) & 0xff, (gpdata.w >> 8) & 0xff, (gpdata.w >> 16) & 0xff ) / 255.0, ((gpdata.w >> 24) & 0xff) / 255.0 );

				if( color.a < _AlphaCull ) return;
				if( length( color.rgb ) < _ColorCull ) return;

				// If falling, then rotate.
				float4 nrot = normalize( chash41( prim ) );
				rot = slerp( rot, nrot, thisFall );
				rot = normalize( rot );

				float3 fnorm = float3( 0.0, 0.0, 1.0 );
				float fn = csimplex3( vp*0.9+100.0 );
				fnorm = Rotate( fnorm, rot );
				
				// Random
				//vp += AudioLinkLerpMultiline(ALPASS_DFT + float2(prim%128, 0)) * fnorm * 0.01;
				
				// Consistent
				//vp += AudioLinkLerpMultiline(ALPASS_DFT + float2(fn*128*0.5, 0)) * fnorm * 0.02;
				
				vp += AudioLinkData(ALPASS_AUTOCORRELATOR + int2( abs( 128 - glsl_mod(fn*128+ntime*8.0, 256.0) ), 0)).xxx * fnorm * 0.004;

				float3 centerWorld = mul( unity_ObjectToWorld, float4( vp.xyz, 1.0 ) );

				float3 PlayerCamera = _WorldSpaceCameraPos.xyz;
				
				float3 ddir = centerWorld.xyz - PlayerCamera;
				ddir = mul( unity_WorldToObject, ddir );
				ddir = normalize( ddir );


				#if _UseSH && !_SHLocal
					float3 shalt = shEvaluate( ddir, coord );
					color.rgb += shalt;
				#endif	

				#if _Obeloid
					float4 irot = rot * float4( 1.0, -1.0, -1.0, -1.0 );
					float3 con = Rotate( ddir, irot );
					float3 boxedge = sign( con ).xyz;
				
					// Corner points of cube.
					const float3 revedge[10] = {
						float3(  1,  -1, -1 ),
						float3(  1,   1, -1 ),
						float3(  -1, -1, -1 ),
						float3(  -1,  1, -1 ),
						float3(  -1,  1,  1 ),
						float3(  -1,  1,  1 ),
						float3(  -1, -1, -1 ),
						float3(  -1, -1,  1 ),
						float3(   1, -1, -1 ),
						float3(   1, -1,  1 ) };
									
					[unroll]
					for( k = 0; k < V_OUT; k++ )
					{
						float3 bx = revedge[k] * boxedge;
						float3 xpt = Transform( vp.xyz, scale, rot, bx );
						o[k].hitworld = mul( unity_ObjectToWorld, float4( xpt, 1.0 ) );


						float howOrtho = UNITY_MATRIX_P._m33; // instead of unity_OrthoParams.w
						float3 worldSpaceCameraPos = UNITY_MATRIX_I_V._m03_m13_m23; // instead of _WorldSpaceCameraPos
						float3 worldPos = o[k].hitworld; // mul(unity_ObjectToWorld, v.vertex); (If in vertex shader)
						float3 cameraToVertex = worldPos - worldSpaceCameraPos;
						float3 orthoFwd = -UNITY_MATRIX_I_V._m02_m12_m22; // often seen: -UNITY_MATRIX_V[2].xyz;
						float3 orthoRayDir = orthoFwd * dot(cameraToVertex, orthoFwd);
						// start from the camera plane (can also just start from o.vertex if your scene is contained within the geometry)
						float3 orthoCameraPos = worldPos - orthoRayDir;
						float3 ro = lerp(worldSpaceCameraPos, orthoCameraPos, howOrtho );
						float3 rd = lerp(cameraToVertex, orthoRayDir, howOrtho );


						// Found by BenDotCom -
						// I saw these ortho shadow substitutions in a few places, but bgolus explains them
						// https://bgolus.medium.com/rendering-a-sphere-on-a-quad-13c92025570c

						o[k].camwithorthoworld = ro;
						float3 camposObject = mul( unity_WorldToObject, float4( ro, 1.0 ) );
						float3 camdirObject = mul( unity_WorldToObject, float4( rd, 1.0 ) );

						// For computing in splat local space, but in object real space.
						o[k].localpt = bx;
						o[k].localscale = scale;
						o[k].localrot = rot;
						o[k].localpos = vp.xyz;
						
						//
						
						// Compute Camera location in splat-local space.
						o[k].campt = Rotate( ( camposObject - vp.xyz), rot * float4( 1.0, -1.0, -1.0, -1.0 ) ) / scale;
						o[k].campd = Rotate( normalize( camdirObject ), rot * float4( 1.0, -1.0, -1.0, -1.0 ) );
						
						o[k].campd = o[k].localpt - o[k].campt;
					}

				#else
					o[0].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float3( -1, -1, 0 ) ), 1.0 ) );
					o[1].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float3( -1,  1, 0 ) ), 1.0 ) );
					o[2].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float3(  1, -1, 0 ) ), 1.0 ) );
					o[3].hitworld = mul( unity_ObjectToWorld, float4( Transform( vp.xyz, scale, rot, float3(  1,  1, 0 ) ), 1.0 ) );
				#endif

				[unroll]
				for( k = 0; k < V_OUT; k++ )
					o[k].vertex = mul(UNITY_MATRIX_VP, o[k].hitworld );

#if defined(_NEED_IDS) || _SHLocal
				[unroll]
				for( k = 0; k < V_OUT; k++ )
					o[k].id = prim;
#endif

#if !defined( _NEED_IDS )
				// Compute emission
				color = pow( color * _Brightness, _Gamma );
				float bnw = ( color.r + color.g + color.b ) / 3.0;
				float3 emission = 0.0;//saturate( ( length( saturate((color.rgb - bnw) * float3( 0.1, 0.1, 2.0 )) ) - .3 ) * 100.0 ) * color;
				emission += color * _EmissionAmount;
				[unroll]
				for( k = 0; k < V_OUT; k++ )
				{
					o[k].color = color;
					o[k].emission = emission;
				}
#endif

				[unroll]
				for( k = 0; k < V_OUT; k++ )
				{
					UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(o[k]);
					stream.Append(o[k]);
				}
			}
			
			float4 calcfrag(g2f i, out uint mask, float rands )
			{

			}





#if _Obeloid
		float ObeloidCalc( g2f i, out float3 Nlocal, out float3 worldHit )
		{
			worldHit = 0.0;

			float3 m = i.campt;
			float3 d = normalize(i.campd);//normalize(i.localpt - i.campt);
			float b = dot( m, d );
			float c = dot( m, m ) - 1.0;
			float discr = b * b - c + _Squircle;
			if( discr < 0.0 ) { return 0.0; };
			float t = -b - sqrt( discr ) + sqrt( _Squircle );
			
			float3 splatLocalHit = m + d * t;
			Nlocal = -Rotate( normalize( splatLocalHit ), i.localrot ) ;
			float3 groupLocalHit = Rotate( splatLocalHit * i.localscale, i.localrot );
			float3 objectLocalHit = groupLocalHit + i.localpos;

			worldHit = (mul( unity_ObjectToWorld, float4( objectLocalHit, 1.0 ) ).xyz);
			return 1.0;
		}

#endif
