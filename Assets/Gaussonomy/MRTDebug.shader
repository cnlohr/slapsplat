﻿Shader "Gaussonomy/MRTDebug"
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

			sampler2D _Stage_0;
			sampler2D _Stage_1;
			sampler2D _Stage_2;
			sampler2D _Stage_3;
			sampler2D _Stage_4;
			sampler2D _Stage_5;
			sampler2D _Stage_6;
			sampler2D _Stage_7;

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

            fixed4 frag (v2f i) : SV_Target
            {
                // sample the texture
				float2 uvsec = i.uv * float2( 4.0, 2.0 );
                float4 col = 0.0;
				uint sec = uint( uvsec.x ) + uint( uvsec.y ) * 4;
				uvsec = frac( uvsec );
				uvsec.y = 1.0 - uvsec.y;
				switch( sec )
				{
				case 0: col = tex2D(_Stage_0, ( uvsec ) ); break;
				case 1: col = tex2D(_Stage_1, ( uvsec ) ); break;
				case 2: col = tex2D(_Stage_2, ( uvsec ) ); break;
				case 3: col = tex2D(_Stage_3, ( uvsec ) ); break;
				case 4: col = tex2D(_Stage_4, ( uvsec ) ); break;
				case 5: col = tex2D(_Stage_5, ( uvsec ) ); break;
				case 6: col = tex2D(_Stage_6, ( uvsec ) ); break;
				case 7: col = tex2D(_Stage_7, ( uvsec ) ); break;
				}
                // apply fog
                UNITY_APPLY_FOG(i.fogCoord, col);
                return col;
            }
            ENDCG
        }
    }
}
