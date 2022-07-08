#  
  9                 GLSL.std.450                      main                (   /        LucyPBR  �    �     //type vertex
#version 450

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec3 a_ID;
layout (location = 3) in vec3 a_Normals;
layout (location = 4) in vec3 a_Tangents;
layout (location = 5) in vec3 a_BiTangents;

struct LucyRendererOutput {
	vec2 TextCoords;
};

layout(location = 0) out LucyRendererOutput r_Output;

layout(set = 0, binding = 0) uniform Camera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
};

//for fragment shader
struct TextureIndices {
	int AlbedoTextureIndex;
	int NormalTextureIndex;
	int RoughnessTextureIndex;
	int MetallicTextureIndex;
	int AOTextureIndex;
};

//Max Size: 128 bytes
layout(push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
	TextureIndices u_TextureIndices; //for fragment shader
};

void main() {
	r_Output.TextCoords = a_TextureCoords;

	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

   
 GL_GOOGLE_cpp_style_line_directive    GL_GOOGLE_include_directive      main      	   LucyRendererOutput    	       TextCoords       r_Output         a_TextureCoords      gl_PerVertex             gl_Position         gl_PointSize            gl_ClipDistance         gl_CullDistance               Camera           u_ViewMatrix            u_ProjMatrix               %   TextureIndices    %       AlbedoTextureIndex    %      NormalTextureIndex   	 %      RoughnessTextureIndex    	 %      MetallicTextureIndex      %      AOTextureIndex    &   LocalPushConstant     &       u_ModelMatrix     &      u_TextureIndices      (         /   a_Pos   J entry-point main    J client vulkan100    J target-env spirv1.5 J target-env vulkan1.2    J entry-point main    G            G           H                H              H              H              G        H            H         #       H               H           H        #   @   H              G        G     "       G     !       H  %       #       H  %      #      H  %      #      H  %      #      H  %      #      H  &          H  &       #       H  &             H  &      #   @   G  &      G  /               !                              	         
      	   ;  
                     +                        ;                                               +                                                   ;                                             ;           +                         %                    &      %      '   	   &   ;  '   (   	      )   	        -            .      -   ;  .   /      +     1     �?   7              %      6               �          &       =           A              >             (       A               =     !       A     "         =     #   "   �     $   !   #   A  )   *   (      =     +   *   �     ,   $   +   =  -   0   /   Q     2   0       Q     3   0      Q     4   0      P     5   2   3   4   1   �     6   ,   5   A  7   8         >  8   6   �  8  