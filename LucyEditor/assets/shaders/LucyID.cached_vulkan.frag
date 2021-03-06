#  
                   GLSL.std.450                     main    
                      LucyID   H    ?     //type fragment
#version 460 core

layout (location = 0) out vec3 a_IDBuffer;
layout (location = 1) out vec3 a_Depth;

layout (location = 0) in vec4 o_ID;

void main() {
	a_IDBuffer = (round(o_ID / 255 * 10e4) / 10e4).xyz;
	//we dont need to do anything with the depth
}
  
 GL_GOOGLE_cpp_style_line_directive    GL_GOOGLE_include_directive      main      
   a_IDBuffer       o_ID         a_Depth J entry-point main    J client vulkan100    J target-env spirv1.5 J target-env vulkan1.2    J entry-point main    G  
          G            G                !                               	         ;  	   
                             ;           +         P?G;  	         +        ???;,                    +        ??'7,                         	      6               ?          
       =           ?              ?                               ?              O                        >  
      ?  8  