#  
  8                 GLSL.std.450                      main                '   .        LucyPBR  ?    ?     //type vertex
#version 450

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec4 a_ID;
layout (location = 3) in vec3 a_Normals;
layout (location = 4) in vec3 a_Tangents;
layout (location = 5) in vec3 a_BiTangents;

struct LucyOutput {
	vec2 TextCoords;
};

layout (location = 0) out LucyOutput r_Output;

layout (set = 0, binding = 0) uniform Camera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
};

layout (push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
	float u_MaterialID;
};

void main() {
	r_Output.TextCoords = a_TextureCoords;

	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

     
 GL_GOOGLE_cpp_style_line_directive    GL_GOOGLE_include_directive      main      	   LucyOutput    	       TextCoords       r_Output         a_TextureCoords      gl_PerVertex             gl_Position         gl_PointSize            gl_ClipDistance         gl_CullDistance               Camera           u_ViewMatrix            u_ProjMatrix               %   LocalPushConstant     %       u_ModelMatrix     %      u_MaterialID      '         .   a_Pos   J entry-point main    J client vulkan100    J target-env spirv1.5 J target-env vulkan1.2    J entry-point main    G            G           H                H              H              H              G        H            H         #       H               H           H        #   @   H              G        G     "       G     !       H  %          H  %       #       H  %             H  %      #   @   G  %      G  .               !                              	         
      	   ;  
                     +                        ;                                               +                                                   ;                                             ;           +                         %            &   	   %   ;  &   '   	      (   	        ,            -      ,   ;  -   .      +     0     ??   6                    6               ?                 =           A              >                    A               =     !       A     "         =     #   "   ?     $   !   #   A  (   )   '      =     *   )   ?     +   $   *   =  ,   /   .   Q     1   /       Q     2   /      Q     3   /      P     4   1   2   3   0   ?     5   +   4   A  6   7         >  7   5   ?  8  