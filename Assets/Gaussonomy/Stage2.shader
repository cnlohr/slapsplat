Shader "Gaussonomy/Stage2"
{
    Properties
    {
        _Stage_0 ("_Stage_0", 2D) = "black" {}
        _Stage_1 ("_Stage_1", 2D) = "black" {}
        _Stage_2 ("_Stage_2", 2D) = "black" {}
        _Stage_3 ("_Stage_3", 2D) = "black" {}
        _Stage_4 ("_Stage_4", 2D) = "black" {}
        _Stage_5 ("_Stage_5", 2D) = "black" {}
        _Stage_6 ("_Stage_6", 2D) = "black" {}
        _Stage_7 ("_Stage_7", 2D) = "black" {}
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 200

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            // make fog work
            #pragma multi_compile_fog

            #include "UnityCG.cginc"

			#pragma target 5.0

			texture2D<float4> _Stage_0;
			texture2D<float4> _Stage_1;
			texture2D<float4> _Stage_2;
			texture2D<float4> _Stage_3;
			texture2D<float4> _Stage_4;
			texture2D<float4> _Stage_5;
			texture2D<float4> _Stage_6;
			texture2D<float4> _Stage_7;

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct v2f
            {
                float2 uv : TEXCOORD0;
                UNITY_FOG_COORDS(1)
                float4 vertex : SV_POSITION;
            };

            sampler2D _MainTex;
            float4 _MainTex_ST;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = v.uv;
                UNITY_TRANSFER_FOG(o,o.vertex);
                return o;
            }
			
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


            MRTOut frag (v2f i ) : SV_Target
            {
				uint2 p = i.vertex;
				float2 uvsec = i.uv;
				uvsec.y = 1.0 - uvsec.y;
				MRTOut ret;
				ret.targ0 = _Stage_0.Load( int3( p, 0 ) )+0.5;
				ret.targ1 = _Stage_1.Load( int3( p, 0 ) )-0.5;
				ret.targ2 = _Stage_2.Load( int3( p, 0 ) )+0.5;
				ret.targ3 = _Stage_3.Load( int3( p, 0 ) )*2.0;
				ret.targ4 = _Stage_4.Load( int3( p, 0 ) )*float4(1.0,0.0,1.0,1.0);
				ret.targ5 = _Stage_5.Load( int3( p, 0 ) );
				ret.targ6 = _Stage_6.Load( int3( p, 0 ) );
				ret.targ7 = _Stage_7.Load( int3( p, 0 ) );
                return ret;
            }
            ENDCG
        }
    }
}
