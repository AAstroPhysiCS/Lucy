#  
  9              ?       GLSL.std.450              
       main          #   (   1                LucyPBR  ?    ?     //type fragment
#version 450

#extension GL_EXT_nonuniform_qualifier : require

#define NULL_TEXTURE_SLOT -1

layout (location = 0) out vec4 a_Color;

struct LucyOutput {
	vec2 TextCoords;
};

layout (push_constant) uniform LocalPushConstant {
	layout (offset = 64) float u_MaterialID;
};

layout (location = 0) in LucyOutput r_Output;

struct MaterialAttributes {
	float AlbedoSlot;
	float NormalSlot;
	float RoughnessSlot;
	float MetallicSlot;

	vec4 BaseDiffuseColor;
	float Shininess;
	float Roughness;
	float Reflectivity;
	float AOSlot;
};

layout (set = 0, binding = 1) readonly buffer LucyMaterialAttributes {
	MaterialAttributes b_MaterialAttributes[];
};

layout (set = 1, binding = 0) uniform sampler2D u_Textures[];

void main() {
	float albedo = b_MaterialAttributes[int(u_MaterialID)].AlbedoSlot;

	if (albedo != NULL_TEXTURE_SLOT)
		a_Color = texture(u_Textures[int(albedo)], r_Output.TextCoords);
	else
		a_Color = vec4(albedo);
}
   GL_EXT_nonuniform_qualifier  
 GL_GOOGLE_cpp_style_line_directive    GL_GOOGLE_include_directive      main         MaterialAttributes           AlbedoSlot          NormalSlot          RoughnessSlot           MetallicSlot            BaseDiffuseColor            Shininess           Roughness           Reflectivity            AOSlot       LucyMaterialAttributes   	        b_MaterialAttributes                  LocalPushConstant            u_MaterialID               #   a_Color   (   u_Textures    /   LucyOutput    /       TextCoords    1   r_Output    J entry-point main    J client vulkan100    J target-env spirv1.5 J target-env vulkan1.2    J entry-point main    H         #       H        #      H        #      H        #      H        #      H        #       H        #   $   H        #   (   H        #   ,   G        0   H            H         #       G        G     "       G     !      H         #   @   G        G  #          G  (   "      G  (   !       G  1               !                   
                          
                                           ;                       +                          	      ;        	         	                  +          ??        "      
   ;  "   #       	 $                              %   $     &   %      '       &   ;  '   (          +       %     .           /   .      0      /   ;  0   1         2      .        '      6               ?          (       A              =           n           A                    =                *       ?              ?  !       ?         6   ?           +       n     *      A  +   ,   (   *   =  %   -   ,   A  2   3   1      =  .   4   3   W  
   5   -   4   >  #   5   ?  !   ?  6        -       P  
   8               >  #   8   ?  !   ?  !   ?  8  