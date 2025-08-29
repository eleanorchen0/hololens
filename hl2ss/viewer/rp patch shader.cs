//basically the same as the horn
Shader "Custom/PatchShader"
{
    Properties
    {
        _ColorTop   ("Left Color (Red)",    Color) = (1, 0, 0, 1)
        _Color2      ("Second Color (Yellow)", Color) = (1, 1, 0, 1)
        _Color3      ("Third Color (Green)", Color) = (0, 1, 0, 1)
        _Color4      ("Third Color (Green)", Color) = (0, 1, 1, 1)
        _ColorBottom  ("Right Color (Blue)",  Color) = (0, 0, 1, 1)

        _GainTex     ("Gain Texture (R)",    2D)    = "white" {}
        _GainScale   ("Gain Scale",          Float) = 1.0

        _CubeColor     ("Wireframe Color", Color)    = (1, 1, 1, 1)
        _CubeAlpha     ("Wireframe Alpha", Range(0,1)) = 0.25
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
            float4   _ColorTop;
            float4   _Color2;
            float4   _Color3;
            float4   _Color4;
            float4   _ColorBottom;

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
                float t = saturate(1.0 - i.uv.y);

                float segment = t * 4.0;
                fixed3 col;

                if (segment < 1.0)
                {
                    float ft = segment;
                    col = lerp(_ColorTop.rgb, _Color2.rgb, ft);
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
                    col = lerp(_Color4.rgb, _ColorBottom.rgb, ft);
                }

                float gain = tex2D(_GainTex, i.uv).r * _GainScale;
                col *= gain;

                return fixed4(col, 1.0);
            }
            ENDCG
        }

        Pass
        {
            Name "WireframeCube"
            Tags { "LightMode"="Always" }
            Cull Off
            ZWrite Off
            Blend SrcAlpha OneMinusSrcAlpha
            CGPROGRAM
            #pragma target 4.0
            #pragma vertex vertCube
            #pragma geometry geomCube
            #pragma fragment fragCube
            #include "UnityCG.cginc"

            float4 _CubeColor;
            float  _CubeAlpha;

            struct appdata
            {
                float4 vertex : POSITION;
            };

            struct v2g
            {
                float4 pos : POSITION;
            };

            struct g2f
            {
                float4 pos : SV_POSITION;
            };

            v2g vertCube(appdata v)
            {
                v2g o;
                o.pos = UnityObjectToClipPos(v.vertex);
                return o;
            }

            [maxvertexcount(6)]
            void geomCube(triangle v2g input[3], inout LineStream<g2f> lines)
            {
                g2f o;
                for (int i = 0; i < 3; i++)
                {
                    o.pos = input[i].pos;
                    lines.Append(o);
                    o.pos = input[(i + 1) % 3].pos;
                    lines.Append(o);
                }
            }

            fixed4 fragCube(g2f i) : SV_Target
            {
                return float4(_CubeColor.rgb, _CubeAlpha);
            }
            ENDCG
        }
    }
}