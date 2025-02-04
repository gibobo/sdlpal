#if defined(VERTEX)

#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#define COMPAT_TEXTURE texture2D
#endif

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec2 TexCoord;
COMPAT_VARYING vec2 v_texCoord;

void main()
{
    gl_Position = VertexCoord;
    v_texCoord = TexCoord;
}

#elif defined(FRAGMENT)

#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
out vec4 FragColor;
#else
#define COMPAT_VARYING varying
#define FragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
precision mediump float;
#endif

COMPAT_VARYING vec2 v_texCoord;
uniform sampler2D tex0;

void main()
{
	FragColor = vec4(COMPAT_TEXTURE(tex0 , v_texCoord.xy).rgb, 1.0);
}

#endif
