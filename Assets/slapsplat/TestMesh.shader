
Shader "cnlohr/TestPointToTriangles" {
Properties {
	_GeoData ("Geo Data", 3D) = "white" {}
}

SubShader {
	Tags { "RenderType"="Opaque" "Queue"="Transparent+50" }
	Blend SrcAlpha OneMinusSrcAlpha
	Cull Off
	
	Pass {  
		CGPROGRAM
			#include "UnityCG.cginc"
			#pragma vertex VertexProgram
			#pragma geometry GeometryProgram
			#pragma fragment frag
			#pragma target 5.0

			struct appdata {
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};

			struct v2g {
				int nv : ID;
				UNITY_VERTEX_OUTPUT_STEREO
			};
			
			struct g2f {
				float4 vertex : SV_POSITION;
				float2 tc : TEXCOORD0;
				UNITY_FOG_COORDS(1)
				UNITY_VERTEX_OUTPUT_STEREO
				UNITY_VERTEX_INPUT_INSTANCE_ID
			};

			float4 _Color;
			float _TesselationUniform;

			v2g VertexProgram ( appdata v, uint vid : SV_VertexID )
			{
				UNITY_SETUP_INSTANCE_ID( v );
				v2g t;
				UNITY_INITIALIZE_OUTPUT(v2g, t);
				UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(t);
				t.nv = vid; // Not actually used (we only have 1)
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

				o[0].tc = float2( 0.0, 0.0 );
				o[1].tc = float2( 1.0, 0.0 );
				o[2].tc = float2( 0.0, 1.0 );
				o[3].tc = float2( 1.0, 1.0 );
				
				o[0].vertex = UnityObjectToClipPos( float4( prim + 0.0, 0.0, 0.0, 1.0 ) );
				o[1].vertex = UnityObjectToClipPos( float4( prim + 0.5, 0.0, 0.0, 1.0 ) );
				o[2].vertex = UnityObjectToClipPos( float4( prim + 0.0, 1.0, 0.0, 1.0 ) );
				o[3].vertex = UnityObjectToClipPos( float4( prim + 0.5, 1.0, 0.0, 1.0 ) );


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
			
			float4 frag(g2f i) : SV_Target
			{
				UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX( i );
				return 1.0;
			}
		ENDCG
	}
}
}