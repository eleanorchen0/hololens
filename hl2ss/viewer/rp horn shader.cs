Shader "Custom/HornPattern_MultiColor_LR"
{
    Properties
    {
        _ColorLeft   ("Left Color (Red)",    Color) = (1, 0, 0, 1)
        _Color2      ("Second Color (Yellow)", Color) = (1, 1, 0, 1)
        _Color3      ("Third Color (Green)", Color) = (0, 1, 0, 1)
        _Color4      ("Third Color (Green)", Color) = (0, 1, 1, 1)
        _ColorRight  ("Right Color (Blue)",  Color) = (0, 0, 1, 1)

        _GainTex     ("Gain Texture (R)",    2D)    = "white" {}
        _GainScale   ("Gain Scale",          Float) = 1.0
    }

    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"

            sampler2D _GainTex;
            float    _GainScale;
            float4   _ColorLeft;
            float4   _Color2;
            float4   _Color3;
            float4   _Color4;
            float4   _ColorRight;

            struct appdata
            {
                float4 vertex : POSITION;
                float3 normal : NORMAL;
                float2 uv     : TEXCOORD0;
            };

            struct v2f
            {
                float4 pos : SV_POSITION;
                float2 uv  : TEXCOORD0;
            };

            v2f vert(appdata v)
            {
                v2f o;
                o.pos = UnityObjectToClipPos(v.vertex);
                o.uv  = v.uv;
                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                float t = saturate(1.0 - i.uv.x);

                float segment = t * 4.0;
                fixed3 col;

                if (segment < 1.0)
                {
                    float ft = segment;
                    col = lerp(_ColorLeft.rgb, _Color2.rgb, ft);
                }
                else if (segment < 2.0)
                {
                    float ft = segment - 1.0;
                    col = lerp(_Color2.rgb, _Color3.rgb, ft);
                }
                else if (segment < 3.0)
                {
                    float ft = segment - 2.0;
                    col = lerp(_Color3.rgb, _Color4.rgb, ft);
                }
                else
                {
                    float ft = segment - 3.0;
                    col = lerp(_Color4.rgb, _ColorRight.rgb, ft);
                }

                float gain = tex2D(_GainTex, i.uv).r * _GainScale;
                col *= gain;

                return fixed4(col, 1.0);
            }
            ENDCG
        }
    }
}