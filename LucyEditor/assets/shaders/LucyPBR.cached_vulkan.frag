#  
  %              �       GLSL.std.450              	       main    
                         LucyPBR  �    �     //type fragment
#version 450

#extension GL_EXT_nonuniform_qualifier : require

#define NULL_TEXTURE_SLOT -1

layout (location = 0) out vec4 a_Color;

struct LucyRendererOutput {
	vec2 TextCoords;
};

layout(location = 0) in LucyRendererOutput r_Output;

//Max Size: 128 bytes
layout(push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix; //unused in fragment shader
	int u_MaterialIndex;
};

layout (set = 1, binding = 0) uniform sampler2D u_Textures[];

void main() {
	//if (u_TextureIndices.AlbedoTextureIndex != NULL_TEXTURE_SLOT)
		a_Color = texture(u_Textures[u_MaterialIndex], r_Output.TextCoords);
	//else
	//a_Color = vec4(u_MaterialIndex, 0.0f, 0.0f, 1.0f);
}
   GL_EXT_nonuniform_qualifier  
 GL_GOOGLE_cpp_style_line_directive    GL_GOOGLE_include_directive      main      
   a_Color      u_Textures       LocalPushConstant            u_ModelMatrix           u_MaterialIndex               LucyRendererOutput           TextCoords       r_Output    J entry-point main    J client vulkan100    J target-env spirv1.5 J target-env vulkan1.2    J entry-point main    G  
          G     "      G     !       H            H         #       H               H        #   @   G        G                 !                               	         ;  	   
       	                                                          ;                                                    	      ;        	   +                 	                                                  ;           +                !                    6               �                 A              =           A              =           A  !   "          =     #   "   W     $      #   >  
   $   �  8  