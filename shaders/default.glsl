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

const float SRGB_ALPHA = 0.055;
float linear_to_srgb(float channel) {
    if(channel <= 0.0031308)
        return 12.92 * channel;
    else
        return (1.0 + SRGB_ALPHA) * pow(channel, 1.0/2.4) - SRGB_ALPHA;
    }

vec3 rgb_to_srgb(vec3 rgb) {
    return vec3(linear_to_srgb(rgb.r), linear_to_srgb(rgb.g), linear_to_srgb(rgb.b)    );
}

float srgb_to_linear(float channel) {
    if (channel <= 0.04045)
        return channel / 12.92;
    else
        return pow((channel + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);
}

vec3 srgb_to_rgb(vec3 srgb) {
    return vec3(srgb_to_linear(srgb.r), srgb_to_linear(srgb.g), srgb_to_linear(srgb.b));
}

void main() {
    vec4 srgb = COMPAT_TEXTURE(tex0 , v_texCoord.xy);
    FragColor = vec4(srgb_to_rgb(srgb.rgb), srgb.a);
#ifdef GL_ES
    FragColor.rgb = FragColor.bgr;
#endif
    vec3 color = FragColor.rgb;
    color = rgb_to_srgb(color);
    FragColor.rgb = color;
}

#endif
