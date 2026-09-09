// ROADSAFE_NATIVE_GLTF_RENDERER_V1

#include "roadsafe_renderer.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace roadsafe
{
namespace
{

constexpr float kPi=3.14159265358979323846f;

struct Vec3f
{
    float x=0.0f;
    float y=0.0f;
    float z=0.0f;
};

static Vec3f operator+(
    const Vec3f& a,
    const Vec3f& b)
{
    return {
        a.x+b.x,
        a.y+b.y,
        a.z+b.z
    };
}

static Vec3f operator-(
    const Vec3f& a,
    const Vec3f& b)
{
    return {
        a.x-b.x,
        a.y-b.y,
        a.z-b.z
    };
}

static Vec3f operator*(
    const Vec3f& v,
    float scalar)
{
    return {
        v.x*scalar,
        v.y*scalar,
        v.z*scalar
    };
}

static float dot(
    const Vec3f& a,
    const Vec3f& b)
{
    return
        a.x*b.x+
        a.y*b.y+
        a.z*b.z;
}

static Vec3f cross(
    const Vec3f& a,
    const Vec3f& b)
{
    return {
        a.y*b.z-a.z*b.y,
        a.z*b.x-a.x*b.z,
        a.x*b.y-a.y*b.x
    };
}

static float length(
    const Vec3f& v)
{
    return std::sqrt(
        std::max(
            0.0f,
            dot(v,v)
        )
    );
}

static Vec3f normalize(
    const Vec3f& v)
{
    const float l=length(v);

    if (l<=1.0e-7f)
        return {0.0f,1.0f,0.0f};

    return v*(1.0f/l);
}

struct Mat4
{
    std::array<float,16> m{};
};

static Mat4 identity()
{
    Mat4 r;
    r.m={
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    return r;
}

static Mat4 multiply(
    const Mat4& a,
    const Mat4& b)
{
    Mat4 r{};

    for (int column=0;column<4;++column)
    {
        for (int row=0;row<4;++row)
        {
            float value=0.0f;

            for (int k=0;k<4;++k)
            {
                value+=
                    a.m[k*4+row]*
                    b.m[column*4+k];
            }

            r.m[column*4+row]=value;
        }
    }

    return r;
}

static Mat4 translate(
    const Vec3f& t)
{
    Mat4 r=identity();
    r.m[12]=t.x;
    r.m[13]=t.y;
    r.m[14]=t.z;
    return r;
}

static Mat4 scaleMatrix(
    const Vec3f& s)
{
    Mat4 r=identity();
    r.m[0]=s.x;
    r.m[5]=s.y;
    r.m[10]=s.z;
    return r;
}

static Mat4 rotateX(
    float radians)
{
    Mat4 r=identity();
    const float c=std::cos(radians);
    const float s=std::sin(radians);

    r.m[5]=c;
    r.m[6]=s;
    r.m[9]=-s;
    r.m[10]=c;

    return r;
}

static Mat4 rotateY(
    float radians)
{
    Mat4 r=identity();
    const float c=std::cos(radians);
    const float s=std::sin(radians);

    r.m[0]=c;
    r.m[2]=-s;
    r.m[8]=s;
    r.m[10]=c;

    return r;
}

static Mat4 rotateZ(
    float radians)
{
    Mat4 r=identity();
    const float c=std::cos(radians);
    const float s=std::sin(radians);

    r.m[0]=c;
    r.m[1]=s;
    r.m[4]=-s;
    r.m[5]=c;

    return r;
}

static Mat4 quaternionMatrix(
    float x,
    float y,
    float z,
    float w)
{
    const float xx=x*x;
    const float yy=y*y;
    const float zz=z*z;
    const float xy=x*y;
    const float xz=x*z;
    const float yz=y*z;
    const float wx=w*x;
    const float wy=w*y;
    const float wz=w*z;

    Mat4 r=identity();

    r.m[0]=1.0f-2.0f*(yy+zz);
    r.m[1]=2.0f*(xy+wz);
    r.m[2]=2.0f*(xz-wy);

    r.m[4]=2.0f*(xy-wz);
    r.m[5]=1.0f-2.0f*(xx+zz);
    r.m[6]=2.0f*(yz+wx);

    r.m[8]=2.0f*(xz+wy);
    r.m[9]=2.0f*(yz-wx);
    r.m[10]=1.0f-2.0f*(xx+yy);

    return r;
}

static Mat4 perspective(
    float fovRadians,
    float aspect,
    float nearPlane,
    float farPlane)
{
    Mat4 r{};

    const float f=
        1.0f/
        std::tan(
            fovRadians*0.5f
        );

    r.m[0]=f/aspect;
    r.m[5]=f;
    r.m[10]=
        (farPlane+nearPlane)/
        (nearPlane-farPlane);
    r.m[11]=-1.0f;
    r.m[14]=
        (2.0f*farPlane*nearPlane)/
        (nearPlane-farPlane);

    return r;
}

static Mat4 lookAt(
    const Vec3f& eye,
    const Vec3f& center,
    const Vec3f& up)
{
    const Vec3f forward=
        normalize(
            center-eye
        );

    const Vec3f side=
        normalize(
            cross(
                forward,
                up
            )
        );

    const Vec3f cameraUp=
        cross(
            side,
            forward
        );

    Mat4 r=identity();

    r.m[0]=side.x;
    r.m[4]=side.y;
    r.m[8]=side.z;

    r.m[1]=cameraUp.x;
    r.m[5]=cameraUp.y;
    r.m[9]=cameraUp.z;

    r.m[2]=-forward.x;
    r.m[6]=-forward.y;
    r.m[10]=-forward.z;

    r.m[12]=-dot(side,eye);
    r.m[13]=-dot(cameraUp,eye);
    r.m[14]=dot(forward,eye);

    return r;
}

static Vec3f transformPoint(
    const Mat4& m,
    const Vec3f& p)
{
    return {
        m.m[0]*p.x+
        m.m[4]*p.y+
        m.m[8]*p.z+
        m.m[12],

        m.m[1]*p.x+
        m.m[5]*p.y+
        m.m[9]*p.z+
        m.m[13],

        m.m[2]*p.x+
        m.m[6]*p.y+
        m.m[10]*p.z+
        m.m[14]
    };
}

static Mat4 nodeMatrix(
    const tinygltf::Node& node)
{
    if (node.matrix.size()==16)
    {
        Mat4 r{};

        for (std::size_t i=0;i<16;++i)
            r.m[i]=
                static_cast<float>(
                    node.matrix[i]
                );

        return r;
    }

    Vec3f t{0.0f,0.0f,0.0f};
    Vec3f s{1.0f,1.0f,1.0f};

    if (node.translation.size()==3)
    {
        t={
            static_cast<float>(
                node.translation[0]
            ),
            static_cast<float>(
                node.translation[1]
            ),
            static_cast<float>(
                node.translation[2]
            )
        };
    }

    if (node.scale.size()==3)
    {
        s={
            static_cast<float>(
                node.scale[0]
            ),
            static_cast<float>(
                node.scale[1]
            ),
            static_cast<float>(
                node.scale[2]
            )
        };
    }

    Mat4 rotation=identity();

    if (node.rotation.size()==4)
    {
        rotation=
            quaternionMatrix(
                static_cast<float>(
                    node.rotation[0]
                ),
                static_cast<float>(
                    node.rotation[1]
                ),
                static_cast<float>(
                    node.rotation[2]
                ),
                static_cast<float>(
                    node.rotation[3]
                )
            );
    }

    return multiply(
        translate(t),
        multiply(
            rotation,
            scaleMatrix(s)
        )
    );
}

static Mat4 entityMatrix(
    const SceneEntityRecord& entity)
{
    const float radians=
        kPi/180.0f;

    const Mat4 rotation=
        multiply(
            rotateZ(
                entity.rotationDegrees[2]*
                radians
            ),
            multiply(
                rotateY(
                    entity.rotationDegrees[1]*
                    radians
                ),
                rotateX(
                    entity.rotationDegrees[0]*
                    radians
                )
            )
        );

    const float sourceScale=
        std::max(
            0.000001f,
            entity.asset.metersPerUnit
        );

    const Vec3f scaleValue{
        entity.scale[0]*sourceScale,
        entity.scale[1]*sourceScale,
        entity.scale[2]*sourceScale
    };

    return multiply(
        translate({
            entity.position[0],
            entity.position[1],
            entity.position[2]
        }),
        multiply(
            rotation,
            scaleMatrix(scaleValue)
        )
    );
}

static GLuint compileShader(
    GLenum type,
    const char* source,
    std::string& error)
{
    const GLuint shader=
        glCreateShader(type);

    glShaderSource(
        shader,
        1,
        &source,
        nullptr
    );

    glCompileShader(shader);

    GLint ok=GL_FALSE;
    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &ok
    );

    if (ok==GL_TRUE)
        return shader;

    GLint lengthValue=0;

    glGetShaderiv(
        shader,
        GL_INFO_LOG_LENGTH,
        &lengthValue
    );

    std::string log(
        static_cast<std::size_t>(
            std::max(1,lengthValue)
        ),
        '\0'
    );

    glGetShaderInfoLog(
        shader,
        lengthValue,
        nullptr,
        log.data()
    );

    error=log;

    glDeleteShader(shader);
    return 0;
}

static GLuint createProgram(
    std::string& error)
{
    static const char* vertexSource=R"GLSL(
#version 330 core

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec2 vTexCoord;

void main()
{
    vec4 world=
        uModel*
        vec4(
            aPosition,
            1.0
        );

    vWorldPosition=
        world.xyz;

    mat3 normalMatrix=
        transpose(
            inverse(
                mat3(uModel)
            )
        );

    vNormal=
        normalize(
            normalMatrix*
            aNormal
        );

    vTexCoord=
        aTexCoord;

    gl_Position=
        uProjection*
        uView*
        world;
}
)GLSL";

    static const char* fragmentSource=R"GLSL(
#version 330 core

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uCameraPosition;
uniform vec4 uBaseColorFactor;
uniform float uMetallic;
uniform float uRoughness;
uniform sampler2D uBaseColorTexture;
uniform int uHasBaseColorTexture;
uniform int uSelected;
uniform int uAnalysis;
uniform float uAlphaCutoff;

out vec4 FragColor;

const float PI=3.14159265359;

float distributionGGX(
    vec3 N,
    vec3 H,
    float roughness)
{
    float a=
        roughness*
        roughness;

    float a2=a*a;

    float NdotH=
        max(
            dot(N,H),
            0.0
        );

    float NdotH2=
        NdotH*
        NdotH;

    float denominator=
        NdotH2*
        (a2-1.0)+
        1.0;

    return
        a2/
        max(
            PI*
            denominator*
            denominator,
            0.0001
        );
}

float geometrySchlickGGX(
    float NdotV,
    float roughness)
{
    float r=
        roughness+
        1.0;

    float k=
        (r*r)/
        8.0;

    return
        NdotV/
        max(
            NdotV*
            (1.0-k)+
            k,
            0.0001
        );
}

float geometrySmith(
    vec3 N,
    vec3 V,
    vec3 L,
    float roughness)
{
    return
        geometrySchlickGGX(
            max(dot(N,V),0.0),
            roughness
        )*
        geometrySchlickGGX(
            max(dot(N,L),0.0),
            roughness
        );
}

vec3 fresnelSchlick(
    float cosTheta,
    vec3 F0)
{
    return
        F0+
        (1.0-F0)*
        pow(
            clamp(
                1.0-cosTheta,
                0.0,
                1.0
            ),
            5.0
        );
}

void main()
{
    vec4 sampled=
        uHasBaseColorTexture!=0
            ? texture(
                uBaseColorTexture,
                vTexCoord
              )
            : vec4(1.0);

    vec4 base=
        sampled*
        uBaseColorFactor;

    if (base.a<uAlphaCutoff)
        discard;

    vec3 albedo=
        pow(
            max(
                base.rgb,
                vec3(0.0)
            ),
            vec3(2.2)
        );

    float metallic=
        clamp(
            uMetallic,
            0.0,
            1.0
        );

    float roughness=
        clamp(
            uRoughness,
            0.045,
            1.0
        );

    vec3 N=
        normalize(vNormal);

    vec3 V=
        normalize(
            uCameraPosition-
            vWorldPosition
        );

    vec3 L=
        normalize(
            vec3(
                -0.55,
                0.90,
                0.35
            )
        );

    vec3 H=
        normalize(
            V+L
        );

    vec3 radiance=
        vec3(
            4.25,
            4.10,
            3.85
        );

    vec3 F0=
        mix(
            vec3(0.04),
            albedo,
            metallic
        );

    float NDF=
        distributionGGX(
            N,
            H,
            roughness
        );

    float G=
        geometrySmith(
            N,
            V,
            L,
            roughness
        );

    vec3 F=
        fresnelSchlick(
            max(
                dot(H,V),
                0.0
            ),
            F0
        );

    vec3 numerator=
        NDF*
        G*
        F;

    float denominator=
        max(
            4.0*
            max(dot(N,V),0.0)*
            max(dot(N,L),0.0),
            0.0001
        );

    vec3 specular=
        numerator/
        denominator;

    vec3 kS=F;

    vec3 kD=
        (vec3(1.0)-kS)*
        (1.0-metallic);

    float NdotL=
        max(
            dot(N,L),
            0.0
        );

    vec3 direct=
        (kD*albedo/PI+specular)*
        radiance*
        NdotL;

    vec3 ambient=
        albedo*
        0.115;

    vec3 color=
        ambient+
        direct;

    if (uSelected!=0)
    {
        color=
            mix(
                color,
                vec3(
                    1.0,
                    0.58,
                    0.08
                ),
                0.10
            );
    }

    if (uAnalysis!=0)
    {
        float diagnostic=
            clamp(
                abs(N.y),
                0.0,
                1.0
            );

        color=
            mix(
                vec3(
                    0.10,
                    0.28,
                    0.42
                ),
                vec3(
                    0.93,
                    0.47,
                    0.08
                ),
                diagnostic
            );
    }

    color=
        color/
        (color+vec3(1.0));

    color=
        pow(
            color,
            vec3(1.0/2.2)
        );

    FragColor=
        vec4(
            color,
            base.a
        );
}
)GLSL";

    std::string vertexError;
    std::string fragmentError;

    const GLuint vertexShader=
        compileShader(
            GL_VERTEX_SHADER,
            vertexSource,
            vertexError
        );

    if (!vertexShader)
    {
        error=
            "Vertex shader: "+
            vertexError;
        return 0;
    }

    const GLuint fragmentShader=
        compileShader(
            GL_FRAGMENT_SHADER,
            fragmentSource,
            fragmentError
        );

    if (!fragmentShader)
    {
        glDeleteShader(
            vertexShader
        );

        error=
            "Fragment shader: "+
            fragmentError;
        return 0;
    }

    const GLuint program=
        glCreateProgram();

    glAttachShader(
        program,
        vertexShader
    );

    glAttachShader(
        program,
        fragmentShader
    );

    glLinkProgram(program);

    glDeleteShader(
        vertexShader
    );

    glDeleteShader(
        fragmentShader
    );

    GLint ok=GL_FALSE;

    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &ok
    );

    if (ok==GL_TRUE)
        return program;

    GLint lengthValue=0;

    glGetProgramiv(
        program,
        GL_INFO_LOG_LENGTH,
        &lengthValue
    );

    std::string log(
        static_cast<std::size_t>(
            std::max(1,lengthValue)
        ),
        '\0'
    );

    glGetProgramInfoLog(
        program,
        lengthValue,
        nullptr,
        log.data()
    );

    error=log;

    glDeleteProgram(program);
    return 0;
}

struct Vertex
{
    float px=0.0f;
    float py=0.0f;
    float pz=0.0f;

    float nx=0.0f;
    float ny=1.0f;
    float nz=0.0f;

    float u=0.0f;
    float v=0.0f;
};

struct GpuPrimitive
{
    GLuint vao=0;
    GLuint vbo=0;
    GLuint ebo=0;

    GLsizei indexCount=0;

    GLuint baseColorTexture=0;

    std::array<float,4> baseColor{
        1.0f,
        1.0f,
        1.0f,
        1.0f
    };

    float metallic=0.0f;
    float roughness=0.7f;

    float alphaCutoff=0.0f;
    bool alphaBlend=false;
    bool doubleSided=false;

    Vec3f minBound{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };

    Vec3f maxBound{
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    };
};

struct DrawCall
{
    std::size_t primitiveIndex=0;
    Mat4 nodeTransform=identity();
};

struct GpuModel
{
    bool valid=false;
    std::string error;

    std::vector<GpuPrimitive> primitives;
    std::vector<DrawCall> draws;
    std::vector<GLuint> ownedTextures;

    Vec3f minBound{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };

    Vec3f maxBound{
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    };
};

static void expandBounds(
    Vec3f& minBound,
    Vec3f& maxBound,
    const Vec3f& point)
{
    minBound.x=
        std::min(
            minBound.x,
            point.x
        );

    minBound.y=
        std::min(
            minBound.y,
            point.y
        );

    minBound.z=
        std::min(
            minBound.z,
            point.z
        );

    maxBound.x=
        std::max(
            maxBound.x,
            point.x
        );

    maxBound.y=
        std::max(
            maxBound.y,
            point.y
        );

    maxBound.z=
        std::max(
            maxBound.z,
            point.z
        );
}

static bool hasValidBounds(
    const Vec3f& minBound,
    const Vec3f& maxBound)
{
    return
        minBound.x<=maxBound.x &&
        minBound.y<=maxBound.y &&
        minBound.z<=maxBound.z;
}

static Vec3f accessorVec3(
    const tinygltf::Model& model,
    const tinygltf::Accessor& accessor,
    std::size_t index)
{
    if (accessor.bufferView<0 ||
        accessor.componentType!=
            TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        return {};
    }

    const auto& view=
        model.bufferViews[
            static_cast<std::size_t>(
                accessor.bufferView
            )
        ];

    const auto& buffer=
        model.buffers[
            static_cast<std::size_t>(
                view.buffer
            )
        ];

    const int stride=
        accessor.ByteStride(view);

    if (stride<=0)
        return {};

    const std::size_t offset=
        view.byteOffset+
        accessor.byteOffset+
        index*
        static_cast<std::size_t>(
            stride
        );

    if (offset+
        sizeof(float)*3>
        buffer.data.size())
    {
        return {};
    }

    const float* values=
        reinterpret_cast<
            const float*
        >(
            buffer.data.data()+
            offset
        );

    return {
        values[0],
        values[1],
        values[2]
    };
}

static std::array<float,2> accessorVec2(
    const tinygltf::Model& model,
    const tinygltf::Accessor& accessor,
    std::size_t index)
{
    if (accessor.bufferView<0 ||
        accessor.componentType!=
            TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        return {0.0f,0.0f};
    }

    const auto& view=
        model.bufferViews[
            static_cast<std::size_t>(
                accessor.bufferView
            )
        ];

    const auto& buffer=
        model.buffers[
            static_cast<std::size_t>(
                view.buffer
            )
        ];

    const int stride=
        accessor.ByteStride(view);

    if (stride<=0)
        return {0.0f,0.0f};

    const std::size_t offset=
        view.byteOffset+
        accessor.byteOffset+
        index*
        static_cast<std::size_t>(
            stride
        );

    if (offset+
        sizeof(float)*2>
        buffer.data.size())
    {
        return {0.0f,0.0f};
    }

    const float* values=
        reinterpret_cast<
            const float*
        >(
            buffer.data.data()+
            offset
        );

    return {
        values[0],
        values[1]
    };
}

static std::uint32_t accessorIndex(
    const tinygltf::Model& model,
    const tinygltf::Accessor& accessor,
    std::size_t index)
{
    if (accessor.bufferView<0)
        return 0;

    const auto& view=
        model.bufferViews[
            static_cast<std::size_t>(
                accessor.bufferView
            )
        ];

    const auto& buffer=
        model.buffers[
            static_cast<std::size_t>(
                view.buffer
            )
        ];

    const int stride=
        accessor.ByteStride(view);

    if (stride<=0)
        return 0;

    const std::size_t offset=
        view.byteOffset+
        accessor.byteOffset+
        index*
        static_cast<std::size_t>(
            stride
        );

    if (offset>=buffer.data.size())
        return 0;

    const unsigned char* bytes=
        buffer.data.data()+
        offset;

    switch (accessor.componentType)
    {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return
                static_cast<
                    std::uint32_t
                >(*bytes);

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            if (offset+sizeof(std::uint16_t)>
                buffer.data.size())
            {
                return 0;
            }

            return
                static_cast<
                    std::uint32_t
                >(
                    *reinterpret_cast<
                        const std::uint16_t*
                    >(bytes)
                );

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            if (offset+sizeof(std::uint32_t)>
                buffer.data.size())
            {
                return 0;
            }

            return
                *reinterpret_cast<
                    const std::uint32_t*
                >(bytes);

        default:
            return 0;
    }
}

static GLuint uploadImage(
    const tinygltf::Image& image)
{
    if (image.image.empty() ||
        image.width<=0 ||
        image.height<=0)
    {
        return 0;
    }

    GLenum format=GL_RGBA;

    switch (image.component)
    {
        case 1:
            format=GL_RED;
            break;

        case 2:
            format=GL_RG;
            break;

        case 3:
            format=GL_RGB;
            break;

        default:
            format=GL_RGBA;
            break;
    }

    GLuint texture=0;

    glGenTextures(
        1,
        &texture
    );

    glBindTexture(
        GL_TEXTURE_2D,
        texture
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        1
    );

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format==GL_RGBA
            ? GL_RGBA8
            : (format==GL_RGB
                ? GL_RGB8
                : format),
        image.width,
        image.height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        image.image.data()
    );

    glGenerateMipmap(
        GL_TEXTURE_2D
    );

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );

    return texture;
}

static void destroyModel(
    GpuModel& model)
{
    for (auto& primitive :
         model.primitives)
    {
        if (primitive.ebo)
        {
            glDeleteBuffers(
                1,
                &primitive.ebo
            );
        }

        if (primitive.vbo)
        {
            glDeleteBuffers(
                1,
                &primitive.vbo
            );
        }

        if (primitive.vao)
        {
            glDeleteVertexArrays(
                1,
                &primitive.vao
            );
        }

        primitive={
        };
    }

    for (GLuint texture :
         model.ownedTextures)
    {
        if (texture)
        {
            glDeleteTextures(
                1,
                &texture
            );
        }
    }

    model.ownedTextures.clear();
    model.primitives.clear();
    model.draws.clear();
    model.valid=false;
}

static bool buildPrimitive(
    const tinygltf::Model& model,
    const tinygltf::Primitive& source,
    const std::vector<GLuint>& imageTextures,
    GpuPrimitive& primitive,
    std::string& error)
{
    const auto positionIt=
        source.attributes.find(
            "POSITION"
        );

    if (positionIt==
        source.attributes.end())
    {
        error=
            "Primitive has no POSITION accessor.";
        return false;
    }

    if (source.mode!=-1 &&
        source.mode!=TINYGLTF_MODE_TRIANGLES)
    {
        error=
            "Only triangle glTF primitives are rendered in V1.";
        return false;
    }

    const int positionAccessorIndex=
        positionIt->second;

    if (positionAccessorIndex<0 ||
        static_cast<std::size_t>(
            positionAccessorIndex
        )>=model.accessors.size())
    {
        error=
            "POSITION accessor index is invalid.";
        return false;
    }

    const auto& positionAccessor=
        model.accessors[
            static_cast<std::size_t>(
                positionAccessorIndex
            )
        ];

    if (positionAccessor.type!=
            TINYGLTF_TYPE_VEC3 ||
        positionAccessor.componentType!=
            TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        error=
            "POSITION must be FLOAT VEC3.";
        return false;
    }

    const tinygltf::Accessor* normalAccessor=
        nullptr;

    const auto normalIt=
        source.attributes.find(
            "NORMAL"
        );

    if (normalIt!=
        source.attributes.end() &&
        normalIt->second>=0 &&
        static_cast<std::size_t>(
            normalIt->second
        )<model.accessors.size())
    {
        const auto& candidate=
            model.accessors[
                static_cast<std::size_t>(
                    normalIt->second
                )
            ];

        if (candidate.type==
                TINYGLTF_TYPE_VEC3 &&
            candidate.componentType==
                TINYGLTF_COMPONENT_TYPE_FLOAT)
        {
            normalAccessor=
                &candidate;
        }
    }

    const tinygltf::Accessor* uvAccessor=
        nullptr;

    const auto uvIt=
        source.attributes.find(
            "TEXCOORD_0"
        );

    if (uvIt!=
        source.attributes.end() &&
        uvIt->second>=0 &&
        static_cast<std::size_t>(
            uvIt->second
        )<model.accessors.size())
    {
        const auto& candidate=
            model.accessors[
                static_cast<std::size_t>(
                    uvIt->second
                )
            ];

        if (candidate.type==
                TINYGLTF_TYPE_VEC2 &&
            candidate.componentType==
                TINYGLTF_COMPONENT_TYPE_FLOAT)
        {
            uvAccessor=
                &candidate;
        }
    }

    std::vector<Vertex> vertices(
        positionAccessor.count
    );

    for (std::size_t i=0;
         i<positionAccessor.count;
         ++i)
    {
        const Vec3f position=
            accessorVec3(
                model,
                positionAccessor,
                i
            );

        vertices[i].px=position.x;
        vertices[i].py=position.y;
        vertices[i].pz=position.z;

        expandBounds(
            primitive.minBound,
            primitive.maxBound,
            position
        );

        if (normalAccessor &&
            i<normalAccessor->count)
        {
            const Vec3f normal=
                accessorVec3(
                    model,
                    *normalAccessor,
                    i
                );

            vertices[i].nx=normal.x;
            vertices[i].ny=normal.y;
            vertices[i].nz=normal.z;
        }

        if (uvAccessor &&
            i<uvAccessor->count)
        {
            const auto uv=
                accessorVec2(
                    model,
                    *uvAccessor,
                    i
                );

            vertices[i].u=uv[0];
            vertices[i].v=uv[1];
        }
    }

    std::vector<std::uint32_t> indices;

    if (source.indices>=0 &&
        static_cast<std::size_t>(
            source.indices
        )<model.accessors.size())
    {
        const auto& indexAccessor=
            model.accessors[
                static_cast<std::size_t>(
                    source.indices
                )
            ];

        indices.resize(
            indexAccessor.count
        );

        for (std::size_t i=0;
             i<indexAccessor.count;
             ++i)
        {
            indices[i]=
                accessorIndex(
                    model,
                    indexAccessor,
                    i
                );
        }
    }
    else
    {
        indices.resize(
            vertices.size()
        );

        for (std::size_t i=0;
             i<indices.size();
             ++i)
        {
            indices[i]=
                static_cast<
                    std::uint32_t
                >(i);
        }
    }

    if (!normalAccessor)
    {
        for (auto& vertex :
             vertices)
        {
            vertex.nx=0.0f;
            vertex.ny=0.0f;
            vertex.nz=0.0f;
        }

        for (std::size_t i=0;
             i+2<indices.size();
             i+=3)
        {
            const std::uint32_t ia=
                indices[i];

            const std::uint32_t ib=
                indices[i+1];

            const std::uint32_t ic=
                indices[i+2];

            if (ia>=vertices.size() ||
                ib>=vertices.size() ||
                ic>=vertices.size())
            {
                continue;
            }

            const Vec3f a{
                vertices[ia].px,
                vertices[ia].py,
                vertices[ia].pz
            };

            const Vec3f b{
                vertices[ib].px,
                vertices[ib].py,
                vertices[ib].pz
            };

            const Vec3f c{
                vertices[ic].px,
                vertices[ic].py,
                vertices[ic].pz
            };

            const Vec3f normal=
                normalize(
                    cross(
                        b-a,
                        c-a
                    )
                );

            for (std::uint32_t index :
                 {ia,ib,ic})
            {
                vertices[index].nx+=
                    normal.x;

                vertices[index].ny+=
                    normal.y;

                vertices[index].nz+=
                    normal.z;
            }
        }

        for (auto& vertex :
             vertices)
        {
            const Vec3f normal=
                normalize({
                    vertex.nx,
                    vertex.ny,
                    vertex.nz
                });

            vertex.nx=normal.x;
            vertex.ny=normal.y;
            vertex.nz=normal.z;
        }
    }

    if (source.material>=0 &&
        static_cast<std::size_t>(
            source.material
        )<model.materials.size())
    {
        const auto& material=
            model.materials[
                static_cast<std::size_t>(
                    source.material
                )
            ];

        const auto& pbr=
            material.pbrMetallicRoughness;

        if (pbr.baseColorFactor.size()==4)
        {
            for (std::size_t i=0;i<4;++i)
            {
                primitive.baseColor[i]=
                    static_cast<float>(
                        pbr.baseColorFactor[i]
                    );
            }
        }

        primitive.metallic=
            static_cast<float>(
                pbr.metallicFactor
            );

        primitive.roughness=
            static_cast<float>(
                pbr.roughnessFactor
            );

        primitive.doubleSided=
            material.doubleSided;

        primitive.alphaBlend=
            material.alphaMode=="BLEND";

        primitive.alphaCutoff=
            material.alphaMode=="MASK"
                ? static_cast<float>(
                    material.alphaCutoff
                  )
                : 0.0f;

        const int textureIndex=
            pbr.baseColorTexture.index;

        if (textureIndex>=0 &&
            static_cast<std::size_t>(
                textureIndex
            )<model.textures.size())
        {
            const int imageIndex=
                model.textures[
                    static_cast<std::size_t>(
                        textureIndex
                    )
                ].source;

            if (imageIndex>=0 &&
                static_cast<std::size_t>(
                    imageIndex
                )<imageTextures.size())
            {
                primitive.baseColorTexture=
                    imageTextures[
                        static_cast<std::size_t>(
                            imageIndex
                        )
                    ];
            }
        }
    }

    glGenVertexArrays(
        1,
        &primitive.vao
    );

    glGenBuffers(
        1,
        &primitive.vbo
    );

    glGenBuffers(
        1,
        &primitive.ebo
    );

    glBindVertexArray(
        primitive.vao
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        primitive.vbo
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertices.size()*
            sizeof(Vertex)
        ),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(
        GL_ELEMENT_ARRAY_BUFFER,
        primitive.ebo
    );

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            indices.size()*
            sizeof(std::uint32_t)
        ),
        indices.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(
            offsetof(
                Vertex,
                px
            )
        )
    );

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(
            offsetof(
                Vertex,
                nx
            )
        )
    );

    glEnableVertexAttribArray(2);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(
            offsetof(
                Vertex,
                u
            )
        )
    );

    glBindVertexArray(0);

    primitive.indexCount=
        static_cast<GLsizei>(
            indices.size()
        );

    return true;
}

static void appendDrawsForNode(
    const tinygltf::Model& source,
    int nodeIndex,
    const Mat4& parentTransform,
    const std::vector<
        std::vector<std::size_t>
    >& meshPrimitiveMap,
    GpuModel& output)
{
    if (nodeIndex<0 ||
        static_cast<std::size_t>(
            nodeIndex
        )>=source.nodes.size())
    {
        return;
    }

    const auto& node=
        source.nodes[
            static_cast<std::size_t>(
                nodeIndex
            )
        ];

    const Mat4 world=
        multiply(
            parentTransform,
            nodeMatrix(node)
        );

    if (node.mesh>=0 &&
        static_cast<std::size_t>(
            node.mesh
        )<meshPrimitiveMap.size())
    {
        for (std::size_t primitiveIndex :
             meshPrimitiveMap[
                static_cast<std::size_t>(
                    node.mesh
                )
             ])
        {
            if (primitiveIndex>=
                output.primitives.size())
            {
                continue;
            }

            output.draws.push_back({
                primitiveIndex,
                world
            });

            const auto& primitive=
                output.primitives[
                    primitiveIndex
                ];

            const std::array<Vec3f,8> corners={
                Vec3f{
                    primitive.minBound.x,
                    primitive.minBound.y,
                    primitive.minBound.z
                },
                Vec3f{
                    primitive.maxBound.x,
                    primitive.minBound.y,
                    primitive.minBound.z
                },
                Vec3f{
                    primitive.minBound.x,
                    primitive.maxBound.y,
                    primitive.minBound.z
                },
                Vec3f{
                    primitive.maxBound.x,
                    primitive.maxBound.y,
                    primitive.minBound.z
                },
                Vec3f{
                    primitive.minBound.x,
                    primitive.minBound.y,
                    primitive.maxBound.z
                },
                Vec3f{
                    primitive.maxBound.x,
                    primitive.minBound.y,
                    primitive.maxBound.z
                },
                Vec3f{
                    primitive.minBound.x,
                    primitive.maxBound.y,
                    primitive.maxBound.z
                },
                Vec3f{
                    primitive.maxBound.x,
                    primitive.maxBound.y,
                    primitive.maxBound.z
                }
            };

            for (const Vec3f& corner :
                 corners)
            {
                expandBounds(
                    output.minBound,
                    output.maxBound,
                    transformPoint(
                        world,
                        corner
                    )
                );
            }
        }
    }

    for (int child :
         node.children)
    {
        appendDrawsForNode(
            source,
            child,
            world,
            meshPrimitiveMap,
            output
        );
    }
}

static std::filesystem::path resolveAssetPath(
    const std::filesystem::path& root,
    const std::string& sourcePath)
{
    if (sourcePath.empty())
        return {};

    std::filesystem::path path(
        sourcePath
    );

    if (path.is_absolute())
        return path.lexically_normal();

    const std::filesystem::path underRoot=
        root/
        path;

    if (std::filesystem::exists(
            underRoot))
    {
        return
            underRoot.lexically_normal();
    }

    return path.lexically_normal();
}

} // namespace

struct RoadSafeRenderer::Impl
{
    GLuint framebuffer=0;
    GLuint colorTexture=0;
    GLuint depthBuffer=0;

    int framebufferWidth=0;
    int framebufferHeight=0;

    GLuint program=0;
    GLuint whiteTexture=0;

    bool initialized=false;

    std::unordered_map<
        std::string,
        GpuModel
    > modelCache;

    std::string status=
        "3D renderer ready";

    std::size_t renderedEntities=0;

    bool ensureInitialized()
    {
        if (initialized)
            return true;

        std::string shaderError;

        program=
            createProgram(
                shaderError
            );

        if (!program)
        {
            status=
                "PBR shader failed: "+
                shaderError;
            return false;
        }

        const unsigned char white[4]={
            255,
            255,
            255,
            255
        };

        glGenTextures(
            1,
            &whiteTexture
        );

        glBindTexture(
            GL_TEXTURE_2D,
            whiteTexture
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            1,
            1,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            white
        );

        glBindTexture(
            GL_TEXTURE_2D,
            0
        );

        initialized=true;
        status=
            "Native GLB/glTF PBR renderer ready";

        return true;
    }

    bool ensureFramebuffer(
        int width,
        int height)
    {
        width=
            std::max(
                1,
                std::min(
                    4096,
                    width
                )
            );

        height=
            std::max(
                1,
                std::min(
                    4096,
                    height
                )
            );

        if (framebuffer &&
            framebufferWidth==width &&
            framebufferHeight==height)
        {
            return true;
        }

        if (depthBuffer)
        {
            glDeleteRenderbuffers(
                1,
                &depthBuffer
            );

            depthBuffer=0;
        }

        if (colorTexture)
        {
            glDeleteTextures(
                1,
                &colorTexture
            );

            colorTexture=0;
        }

        if (framebuffer)
        {
            glDeleteFramebuffers(
                1,
                &framebuffer
            );

            framebuffer=0;
        }

        glGenFramebuffers(
            1,
            &framebuffer
        );

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            framebuffer
        );

        glGenTextures(
            1,
            &colorTexture
        );

        glBindTexture(
            GL_TEXTURE_2D,
            colorTexture
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            colorTexture,
            0
        );

        glGenRenderbuffers(
            1,
            &depthBuffer
        );

        glBindRenderbuffer(
            GL_RENDERBUFFER,
            depthBuffer
        );

        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            width,
            height
        );

        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            depthBuffer
        );

        const bool complete=
            glCheckFramebufferStatus(
                GL_FRAMEBUFFER
            )==
            GL_FRAMEBUFFER_COMPLETE;

        glBindFramebuffer(
            GL_FRAMEBUFFER,
            0
        );

        if (!complete)
        {
            status=
                "3D framebuffer is incomplete.";
            return false;
        }

        framebufferWidth=width;
        framebufferHeight=height;

        return true;
    }

    GpuModel* loadModel(
        const std::filesystem::path& path)
    {
        const std::string key=
            path.string();

        auto existing=
            modelCache.find(key);

        if (existing!=
            modelCache.end())
        {
            return &existing->second;
        }

        GpuModel output;

        if (!std::filesystem::exists(path))
        {
            output.error=
                "Asset not found: "+
                key;

            auto result=
                modelCache.emplace(
                    key,
                    std::move(output)
                );

            return &result.first->second;
        }

        tinygltf::TinyGLTF loader;
        tinygltf::Model source;

        std::string error;
        std::string warning;

        bool ok=false;

        const std::string extension=
            path.extension().string();

        if (extension==".glb" ||
            extension==".GLB")
        {
            ok=
                loader.LoadBinaryFromFile(
                    &source,
                    &error,
                    &warning,
                    key
                );
        }
        else
        {
            ok=
                loader.LoadASCIIFromFile(
                    &source,
                    &error,
                    &warning,
                    key
                );
        }

        if (!ok)
        {
            output.error=
                "glTF load failed: "+
                error;

            if (!warning.empty())
            {
                output.error+=
                    " | "+
                    warning;
            }

            auto result=
                modelCache.emplace(
                    key,
                    std::move(output)
                );

            return &result.first->second;
        }

        std::vector<GLuint> imageTextures(
            source.images.size(),
            0
        );

        for (std::size_t i=0;
             i<source.images.size();
             ++i)
        {
            const GLuint texture=
                uploadImage(
                    source.images[i]
                );

            imageTextures[i]=
                texture;

            if (texture)
            {
                output.ownedTextures.push_back(
                    texture
                );
            }
        }

        std::vector<
            std::vector<std::size_t>
        > meshPrimitiveMap(
            source.meshes.size()
        );

        for (std::size_t meshIndex=0;
             meshIndex<source.meshes.size();
             ++meshIndex)
        {
            const auto& mesh=
                source.meshes[
                    meshIndex
                ];

            for (const auto& sourcePrimitive :
                 mesh.primitives)
            {
                GpuPrimitive primitive;
                std::string primitiveError;

                if (!buildPrimitive(
                        source,
                        sourcePrimitive,
                        imageTextures,
                        primitive,
                        primitiveError))
                {
                    continue;
                }

                const std::size_t primitiveIndex=
                    output.primitives.size();

                output.primitives.push_back(
                    primitive
                );

                meshPrimitiveMap[
                    meshIndex
                ].push_back(
                    primitiveIndex
                );
            }
        }

        if (output.primitives.empty())
        {
            output.error=
                "No renderable triangle primitives in asset.";

            auto result=
                modelCache.emplace(
                    key,
                    std::move(output)
                );

            return &result.first->second;
        }

        int sceneIndex=
            source.defaultScene;

        if (sceneIndex<0 &&
            !source.scenes.empty())
        {
            sceneIndex=0;
        }

        if (sceneIndex>=0 &&
            static_cast<std::size_t>(
                sceneIndex
            )<source.scenes.size())
        {
            for (int nodeIndex :
                 source.scenes[
                    static_cast<std::size_t>(
                        sceneIndex
                    )
                 ].nodes)
            {
                appendDrawsForNode(
                    source,
                    nodeIndex,
                    identity(),
                    meshPrimitiveMap,
                    output
                );
            }
        }

        if (output.draws.empty())
        {
            for (std::size_t meshIndex=0;
                 meshIndex<meshPrimitiveMap.size();
                 ++meshIndex)
            {
                for (std::size_t primitiveIndex :
                     meshPrimitiveMap[
                        meshIndex
                     ])
                {
                    output.draws.push_back({
                        primitiveIndex,
                        identity()
                    });

                    const auto& primitive=
                        output.primitives[
                            primitiveIndex
                        ];

                    expandBounds(
                        output.minBound,
                        output.maxBound,
                        primitive.minBound
                    );

                    expandBounds(
                        output.minBound,
                        output.maxBound,
                        primitive.maxBound
                    );
                }
            }
        }

        output.valid=
            !output.draws.empty();

        if (output.valid)
        {
            output.error.clear();
        }
        else
        {
            output.error=
                "Asset contains no drawable scene nodes.";
        }

        auto result=
            modelCache.emplace(
                key,
                std::move(output)
            );

        return &result.first->second;
    }

    void clearCache()
    {
        for (auto& pair :
             modelCache)
        {
            destroyModel(
                pair.second
            );
        }

        modelCache.clear();
    }
};

RoadSafeRenderer::RoadSafeRenderer()
    : impl_(new Impl())
{
}

RoadSafeRenderer::~RoadSafeRenderer()
{
    delete impl_;
    impl_=nullptr;
}

bool RoadSafeRenderer::render(
    const RoadSafeCase& caseData,
    const RenderSettings& settings,
    const std::filesystem::path& assetRoot)
{
    if (!impl_)
        return false;

    if (!impl_->ensureInitialized())
        return false;

    if (!impl_->ensureFramebuffer(
            settings.width,
            settings.height))
    {
        return false;
    }

    struct VisibleModel
    {
        const SceneEntityRecord* entity=nullptr;
        GpuModel* model=nullptr;
        Mat4 entityTransform=identity();
    };

    std::vector<VisibleModel> visible;

    Vec3f sceneMin{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };

    Vec3f sceneMax{
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    };

    std::string firstError;

    for (const auto& entity :
         caseData.sceneEntities)
    {
        if (!entity.active ||
            !entity.visible ||
            entity.asset.sourcePath.empty())
        {
            continue;
        }

        const auto resolved=
            resolveAssetPath(
                assetRoot,
                entity.asset.sourcePath
            );

        GpuModel* model=
            impl_->loadModel(
                resolved
            );

        if (!model ||
            !model->valid)
        {
            if (firstError.empty() &&
                model)
            {
                firstError=
                    model->error;
            }

            continue;
        }

        const Mat4 transform=
            entityMatrix(entity);

        visible.push_back({
            &entity,
            model,
            transform
        });

        if (hasValidBounds(
                model->minBound,
                model->maxBound))
        {
            const std::array<Vec3f,8> corners={
                Vec3f{
                    model->minBound.x,
                    model->minBound.y,
                    model->minBound.z
                },
                Vec3f{
                    model->maxBound.x,
                    model->minBound.y,
                    model->minBound.z
                },
                Vec3f{
                    model->minBound.x,
                    model->maxBound.y,
                    model->minBound.z
                },
                Vec3f{
                    model->maxBound.x,
                    model->maxBound.y,
                    model->minBound.z
                },
                Vec3f{
                    model->minBound.x,
                    model->minBound.y,
                    model->maxBound.z
                },
                Vec3f{
                    model->maxBound.x,
                    model->minBound.y,
                    model->maxBound.z
                },
                Vec3f{
                    model->minBound.x,
                    model->maxBound.y,
                    model->maxBound.z
                },
                Vec3f{
                    model->maxBound.x,
                    model->maxBound.y,
                    model->maxBound.z
                }
            };

            for (const Vec3f& corner :
                 corners)
            {
                expandBounds(
                    sceneMin,
                    sceneMax,
                    transformPoint(
                        transform,
                        corner
                    )
                );
            }
        }
    }

    Vec3f target{
        0.0f,
        0.5f,
        0.0f
    };

    float radius=5.0f;

    if (hasValidBounds(
            sceneMin,
            sceneMax))
    {
        target={
            (sceneMin.x+sceneMax.x)*0.5f,
            (sceneMin.y+sceneMax.y)*0.5f,
            (sceneMin.z+sceneMax.z)*0.5f
        };

        radius=
            std::max(
                1.0f,
                length(
                    sceneMax-
                    target
                )
            );
    }

    const float distance=
        std::max(
            4.0f,
            radius*2.75f
        );

    Vec3f camera{
        target.x+
            distance*0.72f,
        target.y+
            distance*0.46f,
        target.z+
            distance*0.72f
    };

    Vec3f up{
        0.0f,
        1.0f,
        0.0f
    };

    switch (settings.viewPreset)
    {
        case 1:
            camera={
                target.x,
                target.y+distance,
                target.z+0.001f
            };
            up={
                0.0f,
                0.0f,
                -1.0f
            };
            break;

        case 2:
            camera={
                target.x,
                target.y,
                target.z+distance
            };
            break;

        case 3:
            camera={
                target.x+distance,
                target.y,
                target.z
            };
            break;

        default:
            break;
    }

    const Mat4 view=
        lookAt(
            camera,
            target,
            up
        );

    const float aspect=
        static_cast<float>(
            std::max(1,settings.width)
        )/
        static_cast<float>(
            std::max(1,settings.height)
        );

    const Mat4 projection=
        perspective(
            std::clamp(
                settings.fovDegrees,
                25.0f,
                110.0f
            )*
            kPi/
            180.0f,
            aspect,
            0.05f,
            std::max(
                250.0f,
                distance*20.0f
            )
        );

    glBindFramebuffer(
        GL_FRAMEBUFFER,
        impl_->framebuffer
    );

    glViewport(
        0,
        0,
        impl_->framebufferWidth,
        impl_->framebufferHeight
    );

    glEnable(
        GL_DEPTH_TEST
    );

    glDepthFunc(
        GL_LEQUAL
    );

    glEnable(
        GL_CULL_FACE
    );

    glCullFace(
        GL_BACK
    );

    glDisable(
        GL_BLEND
    );

    glClearColor(
        0.055f,
        0.062f,
        0.074f,
        1.0f
    );

    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );

    glUseProgram(
        impl_->program
    );

    glUniformMatrix4fv(
        glGetUniformLocation(
            impl_->program,
            "uView"
        ),
        1,
        GL_FALSE,
        view.m.data()
    );

    glUniformMatrix4fv(
        glGetUniformLocation(
            impl_->program,
            "uProjection"
        ),
        1,
        GL_FALSE,
        projection.m.data()
    );

    glUniform3f(
        glGetUniformLocation(
            impl_->program,
            "uCameraPosition"
        ),
        camera.x,
        camera.y,
        camera.z
    );

    glUniform1i(
        glGetUniformLocation(
            impl_->program,
            "uBaseColorTexture"
        ),
        0
    );

    glUniform1i(
        glGetUniformLocation(
            impl_->program,
            "uAnalysis"
        ),
        settings.renderMode==2
            ? 1
            : 0
    );

    if (settings.renderMode==1)
    {
        glPolygonMode(
            GL_FRONT_AND_BACK,
            GL_LINE
        );
    }
    else
    {
        glPolygonMode(
            GL_FRONT_AND_BACK,
            GL_FILL
        );
    }

    impl_->renderedEntities=0;

    for (const VisibleModel& visibleModel :
         visible)
    {
        if (!visibleModel.entity ||
            !visibleModel.model)
        {
            continue;
        }

        ++impl_->renderedEntities;

        for (const DrawCall& draw :
             visibleModel.model->draws)
        {
            if (draw.primitiveIndex>=
                visibleModel.model->
                    primitives.size())
            {
                continue;
            }

            const auto& primitive=
                visibleModel.model->
                    primitives[
                        draw.primitiveIndex
                    ];

            const Mat4 modelMatrix=
                multiply(
                    visibleModel.entityTransform,
                    draw.nodeTransform
                );

            glUniformMatrix4fv(
                glGetUniformLocation(
                    impl_->program,
                    "uModel"
                ),
                1,
                GL_FALSE,
                modelMatrix.m.data()
            );

            glUniform4fv(
                glGetUniformLocation(
                    impl_->program,
                    "uBaseColorFactor"
                ),
                1,
                primitive.baseColor.data()
            );

            glUniform1f(
                glGetUniformLocation(
                    impl_->program,
                    "uMetallic"
                ),
                primitive.metallic
            );

            glUniform1f(
                glGetUniformLocation(
                    impl_->program,
                    "uRoughness"
                ),
                primitive.roughness
            );

            glUniform1f(
                glGetUniformLocation(
                    impl_->program,
                    "uAlphaCutoff"
                ),
                primitive.alphaCutoff
            );

            glUniform1i(
                glGetUniformLocation(
                    impl_->program,
                    "uSelected"
                ),
                visibleModel.entity->
                    legacyId==
                    settings.selectedEntityId
                    ? 1
                    : 0
            );

            const GLuint texture=
                primitive.baseColorTexture
                    ? primitive.baseColorTexture
                    : impl_->whiteTexture;

            glActiveTexture(
                GL_TEXTURE0
            );

            glBindTexture(
                GL_TEXTURE_2D,
                texture
            );

            glUniform1i(
                glGetUniformLocation(
                    impl_->program,
                    "uHasBaseColorTexture"
                ),
                primitive.baseColorTexture
                    ? 1
                    : 0
            );

            if (primitive.doubleSided)
            {
                glDisable(
                    GL_CULL_FACE
                );
            }
            else
            {
                glEnable(
                    GL_CULL_FACE
                );
            }

            if (primitive.alphaBlend)
            {
                glEnable(
                    GL_BLEND
                );

                glBlendFunc(
                    GL_SRC_ALPHA,
                    GL_ONE_MINUS_SRC_ALPHA
                );
            }
            else
            {
                glDisable(
                    GL_BLEND
                );
            }

            glBindVertexArray(
                primitive.vao
            );

            glDrawElements(
                GL_TRIANGLES,
                primitive.indexCount,
                GL_UNSIGNED_INT,
                nullptr
            );
        }
    }

    glBindVertexArray(0);

    glPolygonMode(
        GL_FRONT_AND_BACK,
        GL_FILL
    );

    glDisable(
        GL_BLEND
    );

    glEnable(
        GL_CULL_FACE
    );

    glUseProgram(0);

    glBindFramebuffer(
        GL_FRAMEBUFFER,
        0
    );

    if (!firstError.empty())
    {
        impl_->status=
            firstError;
    }
    else if (impl_->renderedEntities==0)
    {
        impl_->status=
            "3D renderer active - assign a free licensed GLB/glTF asset in Properties.";
    }
    else
    {
        impl_->status=
            "Rendered "+
            std::to_string(
                impl_->renderedEntities
            )+
            " RoadSafe scene asset(s).";
    }

    return true;
}

void RoadSafeRenderer::shutdown()
{
    if (!impl_)
        return;

    impl_->clearCache();

    if (impl_->whiteTexture)
    {
        glDeleteTextures(
            1,
            &impl_->whiteTexture
        );

        impl_->whiteTexture=0;
    }

    if (impl_->depthBuffer)
    {
        glDeleteRenderbuffers(
            1,
            &impl_->depthBuffer
        );

        impl_->depthBuffer=0;
    }

    if (impl_->colorTexture)
    {
        glDeleteTextures(
            1,
            &impl_->colorTexture
        );

        impl_->colorTexture=0;
    }

    if (impl_->framebuffer)
    {
        glDeleteFramebuffers(
            1,
            &impl_->framebuffer
        );

        impl_->framebuffer=0;
    }

    if (impl_->program)
    {
        glDeleteProgram(
            impl_->program
        );

        impl_->program=0;
    }

    impl_->framebufferWidth=0;
    impl_->framebufferHeight=0;
    impl_->initialized=false;
    impl_->renderedEntities=0;
    impl_->status=
        "3D renderer shut down";
}

void RoadSafeRenderer::clearAssetCache()
{
    if (!impl_)
        return;

    impl_->clearCache();
    impl_->status=
        "3D asset cache cleared";
}

GLuint RoadSafeRenderer::colorTexture() const
{
    return
        impl_
            ? impl_->colorTexture
            : 0;
}

std::size_t RoadSafeRenderer::loadedAssetCount() const
{
    if (!impl_)
        return 0;

    std::size_t count=0;

    for (const auto& pair :
         impl_->modelCache)
    {
        if (pair.second.valid)
            ++count;
    }

    return count;
}

std::size_t RoadSafeRenderer::renderedEntityCount() const
{
    return
        impl_
            ? impl_->renderedEntities
            : 0;
}

const std::string& RoadSafeRenderer::status() const
{
    static const std::string unavailable=
        "3D renderer unavailable";

    return
        impl_
            ? impl_->status
            : unavailable;
}

} // namespace roadsafe
