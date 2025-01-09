#if __VERSION__ >= 130
   #define COMPAT_VARYING out
   #define COMPAT_ATTRIBUTE in
   #define COMPAT_TEXTURE texture
#else
   #define COMPAT_VARYING varying
   #define COMPAT_ATTRIBUTE attribute
   #define COMPAT_TEXTURE texture2D
#endif
#ifdef GL_ES
   #define COMPAT_PRECISION mediump
#else
   #define COMPAT_PRECISION
#endif
COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec2 TexCoord;
COMPAT_VARYING vec2 v_texCoord;
void main() {
   gl_Position = VertexCoord;
   v_texCoord = TexCoord;
};