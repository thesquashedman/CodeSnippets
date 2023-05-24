Shader "Unlit/BoneShaderColored"
{
    Properties
    {
       
    }
    SubShader
    {
        Tags { "RenderType"="Transparent" "Queue"="Transparent"}
        LOD 100

        ZWrite Off
        Blend SrcAlpha OneMinusSrcAlpha

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
           

            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
                fixed4 mColor : COLOR0; //Holds the weights and the vertex. Weight is unused as is pointless without single vertex to multiple bone parenting, which we never implemented.
            };

            struct v2f
            {
                float2 uv : TEXCOORD0;
                UNITY_FOG_COORDS(1)
                float4 vertex : SV_POSITION;
                fixed4 theColor : COLOR0;
            };
            sampler2D _MainTex;
            float4 _MainTex_ST;
            float4x4 MyXformMat[23]; //Holds all the bone matrices

            v2f vert (appdata v)
            {
                v2f o;
               
                float4x4 temp = MyXformMat[v.mColor.x];     //Get the matrix of the bone that should be applied.

                temp[0][3] *= v.mColor.y;                   //Multiplies the last row of the matrix by the weight. Unused since we don’t support vertex to multiple bone parenting.
                temp[1][3] *= v.mColor.y;                  
                temp[2][3] *= v.mColor.y;                  
                temp = mul(temp, unity_ObjectToWorld);      //Multiplies the bone matrix times the object's transformation matrix          
                o.vertex = mul(temp, v.vertex);             //Transforms the vertex by the concatenation of the object's transformation matrix
                o.vertex = mul(UNITY_MATRIX_VP, o.vertex);  //View and projection matrix multiplication
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);       //Default tex and fog stuff
                UNITY_TRANSFER_FOG(o,o.vertex);

                //Hue shift gotten from comments on https://gist.github.com/mairod/a75e7b44f68110e1576d77419d608786
                float hue = v.mColor.x * (2 * 3.1415 / 23);
                float3 newColor = float3(1, 0 , 0);
                float3 k = float3(0.57735, 0.57735, 0.57735);
                float cosAngle = cos(hue);
                newColor = (newColor * cosAngle + cross(k, newColor) * sin(hue) + k * dot(k, newColor) * (1.0 - cosAngle));
                o.theColor.x = newColor.x * v.mColor.y;
                o.theColor.y = newColor.y * v.mColor.y;
                o.theColor.z = newColor.z * v.mColor.y;
                o.theColor.w = 0.5;
                return o;
            }

            fixed4 frag (v2f i) : SV_Target
            {
                fixed4 col = i.theColor;
                UNITY_APPLY_FOG(i.fogCoord, col);
                return col;
            }
            ENDCG
        }
    }
}