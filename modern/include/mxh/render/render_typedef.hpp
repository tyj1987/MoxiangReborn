// mxh/render/render_typedef.hpp
// 1:1 port of original 4DyuchiGRX_common/typedef.h.
// All binary layouts preserved so the engine's binary files remain compatible.
#pragma once

#include <cstdint>
#include <windows.h>  // RECT, BOOL

#include "math.hpp"
#include "mesh_flag.hpp"

namespace mxh::gx {

// ============================================================================
// Vector / matrix sizes (kept for compatibility with original code)
// ============================================================================
constexpr int  VECTOR3_SIZE    = 12;
constexpr int  VECTOR4_SIZE    = 16;
constexpr float MIN_UNIT       = 10.0f;
constexpr float ONE_CM         = 1.0f;
constexpr int   MAX_NAME_LEN   = 128;

constexpr float DEFAULT_LIGHT_RADIUS         = 100.0f;
constexpr float DEFAULT_RENDER_ZORDER_UNIT   = -1000.0f;
constexpr float WORLDMAP_DEFAULT_TOP         = 400.0f;
constexpr float WORLDMAP_DEFAULT_BOTTOM      = 0.0f;
constexpr float FOG_DISTANCE_START           = 2000.0f;
constexpr float FOG_DISTANCE_END             = 8000.0f;
constexpr int   DEFAULT_DECAL_TRI_NUM        = 64;
constexpr int   MAX_MODEL_NUM_PER_GXOBJECT   = 8;
constexpr int   MAX_RENDER_OBJECT_NUM        = 8192;
constexpr int   MAX_ATTATCH_OBJECTS_NUM      = 8;
constexpr int   MAX_PICK_OBJECT_NUM          = 256;
constexpr int   ZORDER_LATEST_RENDER         = 1100;

constexpr int   MAX_LOD_MODEL_NUM                   = 3;
constexpr int   MAX_HFINDEX_BUFER_NUM               = 128;
constexpr int   MAX_OBJECTS_NUM_PER_MODEL           = 8192;
constexpr int   MAX_STATIC_RTLIGHT_NUM_PER_SCENE    = 8;
constexpr int   MAX_DYNAMIC_RTLIGHT_NUM_PER_SCENE   = 8;
constexpr int   MAX_SHADOW_LIGHT_NUM_PER_SCENE      = 64;
constexpr int   MAX_PRJIMAGE_LIGHT_NUM_PER_SCENE    = 64;
constexpr int   MAX_SPOT_LIGHT_NUM_PER_SCENE        = MAX_SHADOW_LIGHT_NUM_PER_SCENE + MAX_PRJIMAGE_LIGHT_NUM_PER_SCENE;
constexpr int   MAX_REALTIME_LIGHT_NUM              = MAX_STATIC_RTLIGHT_NUM_PER_SCENE + MAX_DYNAMIC_RTLIGHT_NUM_PER_SCENE;
constexpr int   STATIC_RTLIGHT_START_INDEX          = MAX_DYNAMIC_RTLIGHT_NUM_PER_SCENE;
constexpr int   MAX_DIRECTIONAL_LIGHT_NUM           = 4;
constexpr int   MAX_CAMERA_NUM_PER_SCENE            = 128;
constexpr int   MAX_MOTION_NUM                      = 1024;
constexpr std::uint32_t DEFAULT_AMBIENT_COLOR       = 0xff202020;
constexpr int   MAX_RENDER_MODEL_NUM                = 1024;
constexpr int   MAX_RENDER_STATICMODEL_OBJ_NUM      = 4096;
constexpr int   MAX_RENDER_HFIELD_OBJ_NUM           = 1024;
constexpr float DEFAULT_NEAR                        = 100.0f;
constexpr float DEFAULT_FAR                         = 80000.0f;
constexpr int   MAX_FILEITEM_NUM                    = 8192;
constexpr int   MAX_FILEBUCKET_NUM                  = 256;
constexpr int   MAX_VIEWPORT_NUM                    = 256;
constexpr int   MAX_MATRIX_NUM_IN_POOL              = 4096;
constexpr int   MAX_LIGHT_INDEX_NUM_IN_POOL         = 8192;
constexpr int   MAX_MODEL_REF_INDEX_NUM             = 8192;
constexpr float DEFAULT_RESOURCE_SCHDULE_DISTANCE   = 16.0f;
constexpr int   DEFAULT_OVERLAP_TILES_NUM           = 4;

constexpr std::uint8_t DEFAULT_ALPHA_REF_VALUE      = 200;
constexpr int   DEFAULT_RENDER_TEXTURE_NUM          = 4;
constexpr int   MAX_RENDER_TEXTURE_NUM              = 8;
constexpr int   DEFAULT_RENDER_TEXTURE_SIZE         = 256;
constexpr int   MAX_RENDER_TEXTURE_SIZE             = 1024;
constexpr int   MAX_SPRITE_ZORDER_NUM               = 256;
constexpr int   MAX_HFIELD_DETAIL_NUM               = 8;
constexpr int   MAX_TILE_TEXTURE_NUM                = 65536;
constexpr int   MAX_SPRITE_FRAME_NUM                = 1024;
constexpr int   MAX_TEX_PROJECTION_DYNAMIC_LIGHT_NUM = 256;
constexpr int   MAX_TEXTURE_NUM                     = 25600;
constexpr int   MAX_TEXBUCKET_NUM                   = 256;
constexpr int   MAX_RENDER_MESHOBJ_NUM              = 4096;
constexpr int   MAX_RENDER_TEXTBUFFER_SIZE          = 16384;
constexpr int   MAX_RENDER_TRIBUFFER_SIZE           = 8192;
constexpr int   MAX_RENDER_SPRITE_NUM               = 1024;
constexpr int   DEFULAT_CIRCLE_PIECES_NUM           = 32;
constexpr int   MAX_CIRCLE_PIECES_NUM               = 64;
constexpr int   MAX_MIPMAP_LEVEL_NUM                = 12;
constexpr int   MAX_MIPMAP_SIZE                     = 1;
constexpr int   MAX_MATERIAL_NUM                    = 8192;
constexpr int   MAX_MATERIAL_SET_NUM                = 2048;
constexpr int   MAX_D3DRESOURCE_NUM                 = 65536;
constexpr int   DEFAULT_D3DRESOURCE_NUM             = 128;
constexpr int   DEFAULT_VBCACHE_NUM                 = 1024;
constexpr int   DEFAULT_PHYSIQUE_OBJECT_NUM_PER_SCENE = 32;
constexpr int   DEFAULT_MOST_PHYSIQUE_VERTEX_NUM    = 800;
constexpr int   MAX_PHYSIQUE_VERTEX_NUM             = 32768;
constexpr int   MAX_PRJMESH_INDICES_NUM             = 16384;
constexpr float MAXSHINESTR_TO_SS3D_VAL             = 20.0f;
constexpr float DEFAULT_FREE_VBCACHE_RATE            = 0.084f;
constexpr int   MAX_EFFECT_SHADER_NUM               = 1024;
constexpr int   DEFAULT_LIMITED_VERTEXTBUFFER_INDICES = 65536;
constexpr int   MAX_RESOURCE_POOL_NUM               = 32;

// ============================================================================
// Enums
// ============================================================================
enum FONT_TYPE : std::uint32_t {
    SS3D_FONT = 0,
    D3DX_FONT = 1,
};

enum RENDER_MODE : std::uint32_t {
    RENDER_MODE_SOLID     = 0,
    RENDER_MODE_POINT     = 1,
    RENDER_MODE_WIREFRAME = 2,
};

enum APPLY_PHYSIQUE_TYPE : std::uint32_t {
    APPLY_PHYSIQUE_RECALC_NORMAL = 0x00000001,
    APPLY_PHYSIQUE_WRITE_UV      = 0x00000002,
};

enum DEBUG_DRAW_FLAG : std::uint32_t {
    DEBUG_DRAW_MODEL_COL_MESH          = 0x00000001,
    DEBUG_DRAW_STATIC_MODEL_COL_MESH   = 0x00000002,
    DEBUG_DRAW_BONE_COL_MESH           = 0x00000004,
};

enum LIGHT_FLAG : std::uint32_t {
    DISABLE_LIGHT = 0,
    ENABLE_LIGHT  = 1,
};

enum SHADOW_FLAG : std::uint32_t {
    DISABLE_SHADOW = 0,
    ENABLE_SHADOW  = 1,
};

enum BEGIN_RENDER_FLAG : std::uint32_t {
    BEGIN_RENDER_FLAG_CLEAR_ZBUFFER         = 0x00000000,
    BEGIN_RENDER_FLAG_CLEAR_FRAMEBUFFER     = 0x00000000,
    BEGIN_RENDER_FLAG_DONOT_CLEAR_ZBUFFER   = 0x00000001,
    BEGIN_RENDER_FLAG_DONOT_CLEAR_FRAMEBUFFER = 0x00000002,
    BEGIN_RENDER_FLAG_USE_RENDER_TEXTURE    = 0x00000004,
};

enum MODEL_INITIALIZE_FLAG : std::uint32_t {
    MODEL_INITIALIZE_FLAG_NOT_OPTIMIZE = 0x00000001,
};

enum MAP_LOAD_FLAG : std::uint32_t {
    STATIC_MODEL_LOAD_OPTIMIZE              = 0x00000001,
    STATIC_MODEL_LOAD_DEFFER_COMMIT_DEVICE  = 0x00000002,
    STATIC_MODEL_LOAD_ENABLE_SHADE          = 0x00000004,
    HFIELD_MODEL_LOAD_ENABLE_SHADE          = 0x00000004,
    HFIELD_MODEL_LOAD_OPTIMIZE              = 0x00000008,
    HFIELD_MODEL_LOAD_NOT_RENDER            = 0x00000010,
    HFIELD_MODEL_LOAD_ENABLE_DRAW_ALPHAMAP  = 0x00000020,
};

enum LOAD_MAP_FLAG : std::uint32_t {
    LOAD_MAP_FLAG_DEFAULT_PROC_AUTOANIMATION = 0x00000100,
};

enum EXECUTIVE_RENDER_MODE : std::uint32_t {
    RENDER_MODE_DEFAULT = 0,
    RENDER_MODE_TOOL    = 1,
};

enum EXECUTIVE_PICKING_MODE : std::uint32_t {
    PICKING_MODE_DEFAULT = 0,
    PICKING_MODE_TOOL    = 1,
};

enum MODEL_LOD_USING_MODE : std::uint32_t {
    MODEL_LOD_USING_MODE_DEFAULT  = 0,
    MODEL_LOD_USING_MODE_SET_LEVEL = 1,
    MODEL_LOD_USING_MODE_NOT_USE  = 2,
};

enum SYMBOL_TYPE : std::uint32_t {
    SYMBOL_TYPE_LIGHT   = 0,
    SYMBOL_TYPE_TRIGGER = 1,
};
constexpr int SYMBOL_TYPE_NUM = 2;

enum SCHEDULE_FLAG : std::uint32_t {
    SCHEDULE_FLAG_NOT_SCHEDULE  = 0x00000001,
    SCHEDULE_FLAG_DISABLE_UNLOAD = 0x00000002,
    SCHEDULE_FLAG_NOT_RENDER    = 0x10000000,
};

enum UNLOAD_PRELOADED_RESOURCE_TYPE : std::uint32_t {
    UNLOAD_PRELOADED_RESOURCE_TYPE_ONLY_UNLOAD_ENABLED = 0x00000000,
    UNLOAD_PRELOADED_RESOURCE_TYPE_ALL_PRELOADED       = 0x00000001,
};

enum CHAR_CODE_TYPE : std::uint32_t {
    CHAR_CODE_TYPE_ASCII   = 1,
    CHAR_CODE_TYPE_UNICODE = 2,
};

enum STATIC_MODEL_SHADE_TYPE : std::uint32_t {
    STATIC_MODEL_SHADE_TYPE_ENABLE_SHADOW = 0x00000001,
};

enum LIGHTMAP_FLAG : std::uint32_t {
    LIGHTMAP_FLAG_DISABLE_LIGHTMAP  = 0x00000000,
    LIGHTMAP_FLAG_ENABLE            = 0x00000001,
    LIGHTMAP_FLAG_DISABLE_TEXTURE   = 0x00000002,
    LIGHTMAP_FLAG_DISABLE_MAGFILTER = 0x00000004,
};

enum OBJECT_TYPE : std::uint32_t {
    OBJECT_TYPE_UNKNOWN         = 0xf000000,
    OBJECT_TYPE_LIGHT           = 0xf1000000,
    OBJECT_TYPE_CAMERA          = 0xf2000000,
    OBJECT_TYPE_CAMERA_TARGET   = 0xf3000000,
    OBJECT_TYPE_MESH            = 0xf4000000,
    OBJECT_TYPE_BONE            = 0xf5000000,
    OBJECT_TYPE_ILLUSION_MESH   = 0xf6000000,
    OBJECT_TYPE_COLLISION_MESH  = 0xf7000000,
    OBJECT_TYPE_MATERIAL        = 0x00f00000,
    OBJECT_TYPE_MOTION          = 0x0000f000,
};

enum MODEL_READ_TYPE : std::uint32_t {
    MODEL_READ_TYPE_AS_LOD_SUBMODEL = 0x00000001,
};

enum GX_MAP_OBJECT_TYPE : std::uint32_t {
    GX_MAP_OBJECT_TYPE_OBJECT   = 0,
    GX_MAP_OBJECT_TYPE_STRUCT   = 1,
    GX_MAP_OBJECT_TYPE_HFIELD   = 2,
    GX_MAP_OBJECT_TYPE_DECAL    = 3,
    GX_MAP_OBJECT_TYPE_TRIGGER  = 4,
    GX_MAP_OBJECT_TYPE_LIGHT    = 5,
    GX_MAP_OBJECT_TYPE_INVALID  = 0xffffffff,
};
constexpr int GX_MAP_OBJECT_RENDER_START_INDEX      = 0;
constexpr int GX_MAP_OBJECT_RENDER_END_INDEX        = 3;
constexpr int GX_MAP_OBJECT_RENDER_START_INDEX_TOOL = 4;
constexpr int GX_MAP_OBJECT_RENDER_END_INDEX_TOOL   = 5;
constexpr int GX_MAP_OBJECT_TYPE_NUM                = 6;

enum GXLIGHT_ATTACH_TYPE : std::uint32_t {
    ATTACH_TYPE_ATTACH = 0x00000000,
    ATTACH_TYPE_LINK   = 0x00000001,
};

enum MESH_CONTROL_TYPE : std::uint32_t {
    RESULT_MATRIX_ALIGN_VIEW = 0x00000001,
    UPDATE_VERTEX_NORMAL     = 0x00000002,
};

enum MATERIAL_ILLUNUM_TYPE : std::uint32_t {
    SELF_ILLUNUM = 0x10000000,
};

enum MATERIAL_TRANSP_TYPE : std::uint32_t {
    TRANSP_TYPE_FILTER     = 0x00000001,
    TRANSP_TYPE_SUBTRACTIVE = 0x00000002,
    TRANSP_TYPE_ADDITIVE   = 0x00000004,
};

enum HFIELD_ADJUST_TYPE : std::uint32_t {
    HFIELD_ADJUST_TYPE_SET = 0x00000001,
    HFIELD_ADJUST_TYPE_ADD = 0x00000002,
    HFIELD_ADJUST_TYPE_SUB = 0x00000004,
};

enum VERTEX_TYPE : std::uint32_t {
    VERTEX_TYPE_TEXTURE  = 0x00000001,
    VERTEX_TYPE_PHYSIQUE = 0x01000000,
    VERTEX_TYPE_NORMAL   = 0x00010000,
};

enum TEXTURE_TYPE : std::uint32_t {
    TEXTURE_TYPE_DEFAULT = 0x00000000,
    TEXTURE_TYPE_DETAIL  = 0x00000001,
};

enum RENDER_TEXTURE_TYPE : std::uint32_t {
    RENDER_TEXTURE_TYPE_SHADOW  = 0,
    RENDER_TEXTURE_TYPE_REFLECT = 1,
};

enum CREATE_MATERIAL_TYPE : std::uint32_t {
    CREATE_MATERIAL_TYPE_TEXBORDER = 0x00000001,
};

enum TEXTURE_MAP_TYPE : std::uint32_t {
    TEXTURE_MAP_TYPE_AMBIENT        = 0,
    TEXTURE_MAP_TYPE_DIFFUSE        = 1,
    TEXTURE_MAP_TYPE_SPECULAR       = 2,
    TEXTURE_MAP_TYPE_SHINE          = 3,
    TEXTURE_MAP_TYPE_SHINESTRENGTH  = 4,
    TEXTURE_MAP_TYPE_SELFILLUM      = 5,
    TEXTURE_MAP_TYPE_OPACITY        = 6,
    TEXTURE_MAP_TYPE_FILTERCOLOR    = 7,
    TEXTURE_MAP_TYPE_BUMP           = 8,
    TEXTURE_MAP_TYPE_REFLECT        = 9,
};
constexpr int TEXTURE_MAP_TYPE_MAX_INDEX = 9;
constexpr int TEXTURE_MAP_TYPE_NUM       = 10;

enum REDNER_TYPE : std::uint32_t {
    RENDER_TYPE_ENABLE_LIGHT_ANIMATION       = 0x00000001,
    RENDER_TYPE_ENABLE_CAMERA_ANIMATION      = 0x00000002,
    RENDER_TYPE_ENABLE_MODEL_LIGHT           = 0x00000004,
    RENDER_TYPE_ENABLE_CLIP_PER_OBJECT       = 0x00000008,
    RENDER_TYPE_AS_LOD_SUBMODEL              = 0x00000010,
    RENDER_TYPE_UPDATE_SHADING               = 0x00000020,
    RENDER_TYPE_NOT_DRAW                     = 0x00000040,
    RENDER_TYPE_SELF_ILLUMIN                 = 0x00000080,
    RENDER_TYPE_IGNORE_VIEWVOLUME_CLIP       = 0x00000100,
    RENDER_TYPE_SEND_SHADOW                  = 0x00000200,
    RENDER_TYPE_RECV_SHADOW                  = 0x00000400,
    RENDER_TYPE_DISABLE_ZCLIP                = 0x00000800,
    RENDER_TYPE_DISABLE_TEX_FILTERING        = 0x00001000,
    RENDER_TYPE_USE_PROJECTIONLIGHT          = 0x00002000,
    RENDER_TYPE_ENABLE_ILLUSION              = 0x00004000,
    RENDER_TYPE_SPRITE_ADD                   = 0x00008000,
    RENDER_TYPE_SPRITE_MUL                   = 0x00010000,
    RENDER_TYPE_SPRITE_OPASITY               = 0x00020000,
    RENDER_TYPE_USE_EFFECT                   = 0x00040000,
    RENDER_TYPE_UPDATE_COLLISION_BONEMESH_DESC = 0x10000000,
    RENDER_TYPE_UPDATE_ILLUSION_FRAME        = 0x20000000,
    RENDER_TYPE_UPDATE_ALWAYS                = 0x40000000,
};

enum SHADOW_TYPE : std::uint32_t {
    ENABLE_PROJECTION_SHADOW  = 0x00000001,
    ENABLE_PROJECTION_TEXMAP  = 0x00000002,
};

enum PICK_TYPE : std::uint32_t {
    PICK_TYPE_DEFAULT              = 0x00000000,
    PICK_TYPE_PER_COLLISION_MESH   = 0x00000001,
    PICK_TYPE_PER_FACE             = 0x00000002,
    PICK_TYPE_PER_BONE_OBJECT      = 0x00000004,
    PICK_TYPE_SORT                 = 0x00000010,
};

enum LIGHT_TEXTURE_CREATE_TYPE : std::uint32_t {
    LIGHT_TEXTURE_CREATE_BORDER    = 0x00000001,
    LIGHT_TEXTURE_CREATE_PACK_FILE = 0x00000010,
};

enum GXMAP_OBJECT_COMMON_FLAG : std::uint32_t {
    GXMAP_OBJECT_COMMON_TYPE_NOT_PICKABLE       = 0x01000000,
    GXMAP_OBJECT_COMMON_TYPE_NOT_USE_CLIPPER    = 0x02000000,
    GXMAP_OBJECT_COMMON_TYPE_DISABLE_UNLOAD     = 0x04000000,
    GXMAP_OBJECT_COMMON_TYPE_LOCK_TRANSFORM     = 0x08000000,
};

enum GXOBJECT_CREATE_TYPE : std::uint32_t {
    GXOBJECT_CREATE_TYPE_OPTIMIZE       = 0x00000001,
    GXOBJECT_CREATE_TYPE_NOT_OPTIMIZE   = 0x00000000,
    GXOBJECT_CREATE_TYPE_EFFECT         = 0x00000002,
    GXOBJECT_CREATE_TYPE_APPLY_HFIELD   = 0x00000004,
    GXOBJECT_CREATE_TYPE_DEFAULT_PROC   = 0x00000008,
    GXOBJECT_CREATE_TYPE_NOT_USE_MODEL  = 0x00000010,
};

enum GXLIGHT_TYPE : std::uint32_t {
    GXLIGHT_TYPE_TEX_PROJECTION                    = 0x00000000,
    GXLIGHT_TYPE_ENABLE_SHADOW                     = 0x00000200,
    GXLIGHT_TYPE_ENABLE_SPOT                       = 0x00000400,
    GXLIGHT_TYPE_DISABLE_LIGHT_COLOR               = 0x00000800,
    GXLIGHT_TYPE_STATIC                            = 0x00001000,
    GXLIGHT_TYPE_STATIC_SHADOW_DISABLE             = 0x00002000,
    GXLIGHT_TYPE_ENABLE_DYNAMIC_LIGHT              = 0x00004000,
    GXLIGHT_TYPE_ENABLE_IMAGE_PROJECTION           = 0x00008000,
    GXLIGHT_TYPE_DISABLE_NOT_RENDER_MODEL_IN_TOOL  = 0x00010000,
    GXLIGHT_TYPE_ONLY_USE_TOOL                     = 0x00020000,
};

enum GXTRIGGER_TYPE : std::uint32_t {
    GXTRIGGER_TYPE_MOVABLE = 0x00000001,
};

enum SPRITE_CREATE_FLAG : std::uint32_t {
    SPRITE_CREATE_DEFAULT = 0x00000000,
    SPRITE_CREATE_IMAGE   = 0x00000001,
};

enum TEXTURE_CREATE_FLAG : std::uint32_t {
    TEXTURE_CREATE_RENDER_DEFAULT  = 0x00000000,
    TEXTURE_CREATE_RENDER_TARGET   = 0x00000001,
    TEXTURE_CREATE_SYSTEM_MAMANGED = 0x00000000,
    TEXTURE_CREATE_SYSTEM_MEMEORY  = 0x00000100,
    TEXTURE_CREATE_SYSTEM_VIDEO    = 0x00000200,
};

enum MATERIAL_TYPE : std::uint32_t {
    MATERIAL_TYPE_2SIDE = 0x00000100,
};

enum HEIGHT_FIELD_UPDATE_TYPE : std::uint32_t {
    HEIGHT_FIELD_UPDATE_TYPE_TEXTURE     = 0x00000000,
    HEIGHT_FIELD_UPDATE_TYPE_VERTEX_POS  = 0x00000001,
};

enum EDIT_ALPHA_TEXEL_TYPE : std::uint32_t {
    EDIT_ALPHA_TEXEL_OP_ADD         = 0x01000000,
    EDIT_ALPHA_TEXEL_OP_SUB         = 0x02000000,
    EDIT_ALPHA_TEXEL_BRIGHT_0       = 0x00000000,
    EDIT_ALPHA_TEXEL_BRIGHT_1       = 0x00000001,
    EDIT_ALPHA_TEXEL_BRIGHT_2       = 0x00000002,
    EDIT_ALPHA_TEXEL_BRIGHT_3       = 0x00000004,
};

enum POSITION_STATE : std::uint32_t {
    UP    = 0,
    LEFT  = 1,
    DOWN  = 2,
    RIGHT = 3,
};

enum SPOT_LIGHT_TYPE : std::uint32_t {
    SPOT_LIGHT_TYPE_PRJIMAGE = 0,
    SPOT_LIGHT_TYPE_SHADOW   = 1,
    SPOT_LIGHT_TYPE_MIRROR   = 2,
};

enum DISPLAY_TYPE : std::uint32_t {
    WINDOW_WITH_BLT         = 0x00000000,
    FULLSCREEN_WITH_BLT     = 0x00000001,
    FULLSCREEN_WITH_FLIP    = 0x10000001,
};
constexpr std::uint32_t FLIP_MASK         = 0x10000000;
constexpr std::uint32_t FULLSCREEN_MASK   = 0x00000001;

enum TEXTURE_FORMAT : std::uint32_t {
    TEXTURE_FORMAT_A8R8G8B8 = 0,
    TEXTURE_FORMAT_A4R4G4B4 = 1,
    TEXTURE_FORMAT_R5G6B5   = 2,
    TEXTURE_FORMAT_A1R5G5B5 = 3,
};

enum TEXGEN_METHOD : std::uint32_t {
    TEXGEN_METHOD_REFLECT_SPHEREMAP = 0,
    TEXGEN_METHOD_WAVE              = 1,
};

enum ERROR_TYPE : std::uint32_t {
    ERROR_TYPE_ENGINE_CODE       = 0,
    ERROR_TYPE_PARAMETER_INVALID = 1,
    ERROR_TYPE_DEVICE_NOT_SUPPROT = 2,
    ERROR_TYPE_D3D_ERROR         = 3,
    ERROR_TYPE_RESOURCE_LEAK     = 4,
    ERROR_TYPE_FILE_NOT_FOUND    = 5,
};

// ============================================================================
// Plain data structs (no methods) - binary compatible with original layout
// ============================================================================
struct DWORD_RECT  { std::uint32_t left, top, right, bottom; };
struct FLOAT_RECT  { float fLeft, fTop, fRight, fBottom; };
struct COLORVALUE  { float r, g, b, a; };
struct TVERTEX     { float u, v; };
struct TPVERTEX    { float x, y, z, tu, tv; };

struct NODE_TM {
    float   fRotAng;
    float   fPosX, fPosY, fPosZ;
    float   fRotAxisX, fRotAxisY, fRotAxisZ;
    float   fScaleX, fScaleY, fScaleZ;
    float   fScaleAxisX, fScaleAxisY, fScaleAxisZ;
    float   fScaleAngle;
    MATRIX4 mat4;
    MATRIX4 mat4Inverse;
};

struct LIGHT_DESC {
    std::uint32_t dwAmbient;
    std::uint32_t dwDiffuse;
    std::uint32_t dwSpecular;
    VECTOR3       v3Point;
    float         fRs;
    VECTOR3       v3To;
    VECTOR3       v3Up;
    std::uint32_t dwProjTexIndex;
    float         fFov;
    float         fNear;
    float         fFar;
    float         fWidth;
    float         fHeight;
    void*         pMtlHandle;
    std::uint32_t dwFlag;
    SPOT_LIGHT_TYPE type;
};

struct MIRROR_DESC {
    VECTOR3 v3Point;
    float   fRs;
    VECTOR3 v3To;
    VECTOR3 v3Up;
    float   fFov;
    float   fNear;
    float   fFar;
    float   fWidth;
    float   fHeight;
    std::uint32_t dwFlag;
};

struct DIRECTIONAL_LIGHT_DESC {
    std::uint32_t dwAmbient;
    std::uint32_t dwDiffuse;
    std::uint32_t dwSpecular;
    VECTOR3       v3Dir;
    BOOL          bEnable;
};

struct LIGHT_INDEX_DESC {
    void*  pMtlHandle;
    std::uint8_t bLightIndex;
    std::uint8_t bTexOP;
    std::uint8_t bReserved2;
    std::uint8_t bReserved3;
};

struct INDEX_BUFFER_DESC {
    std::uint32_t dwIndicesNum;
    std::uint16_t* pIndex;
};

struct INDEX_POS {
    std::uint32_t dwX;
    std::uint32_t dwY;
};

struct PLANE {
    VECTOR3 v3Up;
    float   D;
};

struct PATCH {
    std::uint32_t dwColor;
    VECTOR3       v3Point;
    std::uint16_t sx;
    std::uint16_t sy;
    std::uint32_t dwLightColor[4];
};
constexpr int PATCH_SX_OFFSET            = 4 + VECTOR3_SIZE;
constexpr int PATCH_SY_OFFSET            = PATCH_SX_OFFSET + 2;
constexpr int PATCH_LIGHT_COLOR_OFFSET   = PATCH_SY_OFFSET + 2;
constexpr int PATCH_SIZE                 = 36;

#pragma pack(push, 1)
struct TGA_HEADER {
    char  idLength;
    char  ColorMapType;
    char  ImageType;
    std::uint16_t ColorMapFirst;
    std::uint16_t ColorMapLast;
    char  ColorMapBits;
    std::uint16_t FirstX;
    std::uint16_t FirstY;
    std::uint16_t width;
    std::uint16_t height;
    char  Bits;
    char  Descriptor;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BONE_OLD {
    void*  pBone;
    float  fWeight;
    VECTOR3 v3Offset;
    VECTOR3 v3NormalOffset;
};

struct BONE {
    void*  pBone;
    float  fWeight;
    VECTOR3 v3Offset;
    VECTOR3 v3NormalOffset;
    VECTOR3 v3TangentOffset;
};

struct PHYSIQUE_VERTEX {
    std::uint8_t bBonesNum;
    BONE*        pBoneList;
};
#pragma pack(pop)

constexpr int BONE_OLD_SIZE              = 32;
constexpr int BONE_SIZE                  = 32 + 12;
constexpr int PHYSIQUE_VERTEX_SIZE       = 5;
constexpr int POS_OFFSET_IN_BONE         = 8;
constexpr int NORMAL_OFFSET_IN_BONE      = 20;
constexpr int TANGENT_OFFSET_IN_BONE     = 32;

struct COLLISION_FACE {
    VECTOR3 v3Point[3];
    VECTOR3 v3Up;
    float   D;
};

struct MATERIAL {
    std::uint32_t dwTextureNum;
    std::uint32_t dwDiffuse;
    std::uint32_t dwAmbient;
    std::uint32_t dwSpecular;
    float   fTransparency;
    float   fShine;
    float   fShineStrength;
    char    szDiffuseTexmapFileName[MAX_NAME_LEN];
    char    szReflectTexmapFileName[MAX_NAME_LEN];
    char    szBumpTexmapFileName[MAX_NAME_LEN];
    std::uint32_t dwFlag;

    std::uint32_t GetFlag() const              { return dwFlag; }
    const char*   GetDiffuseTexmapName() const { return szDiffuseTexmapFileName; }
    const char*   GetReflectTexmapName() const { return szReflectTexmapFileName; }
    const char*   GetBumpTexmapName() const    { return szBumpTexmapFileName; }
    std::uint32_t GetDiffuse() const           { return dwDiffuse; }
    std::uint32_t GetAmbient() const           { return dwAmbient; }
    std::uint32_t GetSpecular() const          { return dwSpecular; }
    float         GetTransparency() const      { return fTransparency; }
    float         GetShine() const             { return fShine; }
    float         GetShineStrength() const     { return fShineStrength; }
};

struct MATERIAL_TABLE {
    MATERIAL*     pMtl;
    std::uint32_t dwMtlIndex;
};

struct LIGHT_TEXTURE {
    struct TEXTURE_PLANE* pTexPlane;
    std::uint32_t dwTexPlaneNum;
    std::uint32_t dwSurfaceWidth;
    std::uint32_t dwSurfaceHeight;
    std::uint32_t dwBPS;
    char*        pBits;
};

struct VIEW_VOLUME {
    VECTOR3 From;
    PLANE   Plane[4];
    float   fFar;
    BOOL    bIsOrtho;
    float   fWidth;
    VECTOR3 Points[4];
};

struct MESH_DESC {
    std::uint32_t dwVertexNum;
    VECTOR3*      pv3WorldList;
    VECTOR3*      pv3LocalList;
    std::uint32_t dwTexVertexNum;
    TVERTEX*      ptvTexCoordList;
    MATRIX4*      pMatrixWorldInverse;
    std::uint32_t dwFaceGroupNum;
    LIGHT_TEXTURE LightTexture;
    std::uint32_t* pVertexColor;
    VECTOR3*      pv3NormalLocal;
    VECTOR3*      pv3TangentULocal;
    class CMeshFlag meshFlag;
};

struct CAMERA_DESC {
    VECTOR3 v3From;
    VECTOR3 v3To;
    VECTOR3 v3Up;
    VECTOR3 v3EyeDir;
    float   fXRot, fYRot, fZRot;
    float   fFovX, fFovY, fAspect;
    float   fNear, fFar;
};

class CMaterial;

struct FACE_DESC {
    std::uint16_t* pIndex;
    std::uint32_t  dwFacesNum;
    std::uint32_t  dwMtlIndex;
    MATERIAL*      pMaterial;
    TVERTEX*       ptUVLight1;
    TVERTEX*       ptUVLight2;
    std::uint32_t* pdwMtlIndex;
};

struct IVERTEX {
    float x, y, z;
    float u1, v1;
};

struct MOTION_DESC {
    std::uint32_t dwTicksPerFrame;
    std::uint32_t dwFirstFrame;
    std::uint32_t dwLastFrame;
    std::uint32_t dwFrameSpeed;
    std::uint32_t dwKeyFrameStep;
    char    szMotionName[MAX_NAME_LEN];
};

struct MODEL_STATUS {
    std::uint32_t dwFrame;
    std::uint32_t dwMotionIndex;
    std::uint32_t dwLODLevel;
    float   fDistanceFromView;
    float   fDistanceFactor;
    void*   pMotionObject;
    BOOL    bAxisAlignOK;
};

struct MOTION_STATUS {
    std::uint32_t dwFrame;
    std::uint32_t dwMotionIndex;
    void*   pMotionUID;
};

struct BOUNDING_BOX    { VECTOR3 v3Oct[8]; };
struct BOUNDING_SPHERE { VECTOR3 v3Point; float fRs; };
struct BOUNDING_CYLINDER { VECTOR3 v3Point; float fRs; float fAy; };
struct BOUNDING_CAPSULE  { VECTOR3 v3From; VECTOR3 v3To; float fRadius; };

struct COLLISION_MESH_OBJECT_DESC_SAVELOAD {
    BOUNDING_BOX    boundingBox;
    BOUNDING_SPHERE boundingSphere;
    BOUNDING_CYLINDER boundingCylinder;
    std::uint32_t dwObjIndex;
    char    szObjName[MAX_NAME_LEN];
};

struct COLLISION_MESH_OBJECT_DESC {
    BOUNDING_BOX    boundingBox;
    BOUNDING_SPHERE boundingSphere;
    BOUNDING_CYLINDER boundingCylinder;
    std::uint32_t dwObjIndex;
};

struct COLLISION_MODEL_DESC {
    COLLISION_MESH_OBJECT_DESC colMeshModel;
    std::uint32_t dwColMeshObjectDescNum;
    COLLISION_MESH_OBJECT_DESC colMeshObjectDesc[1];
};

struct OBJ_REF_DESC {
    COLLISION_MESH_OBJECT_DESC colMeshDescLocal;
    COLLISION_MESH_OBJECT_DESC colMeshDescWorld;
    MATRIX4        matResult;
    MODEL_STATUS   modelStatus;
};

struct IMAGE_HEADER {
    std::uint32_t dwWidth, dwHeight, dwPitch, dwBPS;
};

struct IMAGE_DESC {
    IMAGE_HEADER imgHeader;
    char* pBits;
};

struct TEXTURE_TABLE {
    std::uint16_t wIndex;
    char    szTextureName[MAX_NAME_LEN];
};

struct PACKFILE_NAME_TABLE {
    char    szFileName[_MAX_PATH];
    std::uint32_t dwFlag;
};

struct HFIELD_CREATE_DESC {
    float   left, top, right, bottom;
    float   fFaceSize;
    std::uint32_t dwFacesNumPerObjAxis;
    std::uint32_t dwObjNumX;
    std::uint32_t dwObjNumZ;
    std::uint32_t dwDetailLevelNum;
    std::uint32_t dwIndexBufferNumLV0;
    TEXTURE_TABLE* pTexTable;
    std::uint32_t dwTileTextureNum;
};

struct HFIELD_DESC {
    float   left, top, right, bottom;
    float   fFaceSize;
    std::uint32_t dwFacesNumPerObjAxis;
    std::uint32_t dwObjNumX, dwObjNumZ;
    std::uint8_t bDetailLevelNum, bReserved0, bReserved1, bBlendEnable;
    std::uint32_t dwIndexBufferNumLV0;
    TEXTURE_TABLE* pTexTable;
    std::uint32_t dwTileTextureNum;
    float*  pyfList;
    std::uint32_t dwYFNumX, dwYFNumZ;
    float   width, height;
    std::uint32_t dwFacesNumX, dwFacesNumZ;
    std::uint32_t dwTriNumPerObj;
    std::uint32_t dwVerticesNumPerObj;
    std::uint16_t* pwTileTable;
    std::uint32_t dwFacesNumPerTileAxis;
    std::uint32_t dwTileNumPerObjAxis;
    std::uint32_t dwTileNumX, dwTileNumZ;
    float   fTileSize;
};

struct HFIELD_OBJECT_DESC {
    std::uint32_t dwPosX, dwPosZ;
    std::uint32_t dwObjPosX, dwObjPosZ;
};

struct SYSTEM_STATUS {
    std::uint32_t dwAvaliableTexMem;
    std::uint32_t dwTotalTexMem;
    char    szDeviceType[MAX_NAME_LEN];
};

struct TILE_ENTRY_DESC {
    std::uint16_t wTilePosEntry;
    std::uint16_t wTilePosNum;
};

struct INDEX_ENTRY_DESC {
    std::uint32_t dwStartIndex;
    std::uint32_t dwTriNum;
};

struct ALPHAMAP_DESC {
    char*   pAlphaMapBits;
    void*   pVoidExt;
    std::uint16_t wWidthHeight;
    std::uint8_t bReserved0, bReserved1;
};

struct TILE_BUFFER_DESC {
    std::uint16_t wTileIndexIntegrated;
    std::uint8_t  bReserved1, bReserved2;
    INDEX_ENTRY_DESC indexEntryDesc[4];
    std::uint32_t dwTilePosNumPri;
    std::uint32_t dwTilePosNumExt;
    ALPHAMAP_DESC  alphaMapDesc;
    INDEX_POS*     pTilePosPri;
    TILE_ENTRY_DESC tileEntryDescPri[4];
    INDEX_POS*     pTilePosExt;
    TILE_ENTRY_DESC tileEntryDescExt[4];

    std::uint16_t GetIntegratedTileIndex() const { return wTileIndexIntegrated; }
    std::uint16_t GetTileIndex() const { return wTileIndexIntegrated & 0x3fff; }
    std::uint32_t GetTileDir() const { return (wTileIndexIntegrated & 0xc000) >> 14; }
    void SetTileIndex(std::uint16_t wTileIndex) { wTileIndexIntegrated = wTileIndex; }
    std::uint16_t IsEqual(std::uint16_t wTileIndex) const { return (wTileIndex & 0x3fff) - (wTileIndexIntegrated & 0x3fff); }
};

struct DISPLAY_INFO {
    DISPLAY_TYPE  dispType;
    std::uint32_t dwWidth;
    std::uint32_t dwHeight;
    std::uint32_t dwRefreshRate;
    std::uint32_t dwBPS;
};

struct TILE_TABLE_DESC {
    float fTop, fLeft, fBottom, fRight;
    std::uint32_t dwTileNumWidth;
    std::uint32_t dwTileNumHeight;
    std::uint32_t dwBytesOfTile;
    float fTileWidth, fTileHeight;
};

using GXOBJECT_HANDLE    = void*;
using GXMAP_HANDLE       = void*;
using GXLIGHT_HANDLE     = void*;
using GXMAP_OBJECT_HANDLE = void*;
using GXTRIGGER_HANDLE   = void*;
using GXDECAL_HANDLE     = void*;
using GXSOUND_HANDLE     = void*;

class I4DyuchiGXExecutive;
class I4DyuchiAudio;
class CBaseObject;
class CCameraObject;
class CFaceGroup;

struct MAABB { VECTOR3 Max; VECTOR3 Min; };

enum BOUNDING_VOLUME_TYPE : std::uint32_t {
    BOUNDING_VOLUME_TYPE_NONE,
    BOUNDING_VOLUME_TYPE_SPHERE,
    BOUNDING_VOLUME_TYPE_AAELLIPSOID,
    BOUNDING_VOLUME_TYPE_AAELLIPSOID2,
};

enum COLLISION_TARGET_FLAG : std::uint32_t {
    COLLISION_TARGET_FLAG_TEST_NONE             = 0x00000000,
    COLLISION_TARGET_FLAG_TEST_STATIC_OBJECT    = 0x00000001,
    COLLISION_TARGET_FLAG_TEST_DYNAMIC_OBJECT   = 0x00000002,
    COLLISION_TARGET_FLAG_TEST_HEIGHT_FIELD     = 0x00000004,
    COLLISION_TARGET_FLAG_TEST_EVENT_TRIGGER    = 0x00000008,
};

struct BOUNDING_VOLUME {
    std::uint32_t dwType;
    VECTOR3 vPivot;
    float   fRadius;
    float   fHeight;
    std::uint32_t dwCollisionTargetFlag;
};

struct DBG_COLLISION_INFO {
    std::uint32_t dwTickFindOctree;
    std::uint32_t dwTickTotalTestCollision;
    std::uint32_t dwMeetingDynamicObjectCount;
};

enum BUILD_TREE_FLAG : std::uint32_t {
    BUILD_TREE_FLAG_BUILD_STATIC_OBJECT = 0x00000002,
};

struct HFPOINT {
    std::uint32_t dwPosX, dwPosZ;
};

struct EVENT_TRIGGER_DESC {
    VECTOR3 v3Pos;
    VECTOR3 v3Scale;
    VECTOR3 v3Rot;
    std::uint32_t dwColor;
};

struct GXLIGHT_PROPERTY {
    LIGHT_DESC lightDesc;
    std::uint32_t dwID;
    BOOL bLightSwitch;
    BOOL bShadowSwitch;
    BOOL bEnableDynamicLight;
};

struct GXOBJECT_PROPERTY {
    VECTOR3 v3Pos;
    VECTOR3 v3Axis;
    VECTOR3 v3Scale;
    float   fRad;
    std::uint32_t dwID;
    BOOL bApplyHField;
    BOOL bAsEffect;
    BOOL bLock;
};

struct GXTRIGGER_PROPERTY {
    EVENT_TRIGGER_DESC evDesc;
    std::uint32_t dwID;
};

constexpr const char* PID_MOD_FILENAME    = "*MOD_FILE_NAME";
constexpr const char* PID_MOTION_NUM      = "*MOTION_NUM";
constexpr const char* PID_MATERIAL_NUM    = "*MATERIAL_NUM";
constexpr const char* PID_STATIC_MODEL    = "STATIC_MODEL";
constexpr const char* PID_HEIGHT_FIELD    = "HEIGHT_FIELD";
constexpr const char* PID_GX_MAP          = "GX_MAP";
constexpr const char* PID_GX_OBJECT       = "GX_OBJECT";
constexpr const char* PID_GX_LIGHT        = "GX_LIGHT";
constexpr const char* PID_GX_TRIGGER      = "GX_TRIGGER";
constexpr const char* PID_GX_METADATA     = "GX_METADATA";
constexpr const char* PID_MAX_HEIGHT      = "MAX_HEIGHT";
constexpr const char* PID_MIN_HEIGHT      = "MIN_HEIGHT";
constexpr const char* PID_BOUNDINGBOX_MAX = "BOX_MAX";
constexpr const char* PID_BOUNDINGBOX_MIN = "BOX_MIN";

struct TRI_FACE {
    VECTOR3 v3Point;
    PLANE   plane;
    std::uint16_t wIndex[4];
};
constexpr int TRI_FACE_SIZE         = 12 + 16 + 8;
constexpr int TRI_FACE_NORMAL_OFFSET = 12;
constexpr int TRI_FACE_D_OFFSET      = 24;
constexpr int TRI_FACE_INDEX_OFFSET  = 28;

struct SHORT_RECT {
    short left, top, right, bottom;
};

struct HFIELD_POS {
    std::uint32_t dwX, dwZ;
};

struct FOG_DESC {
    float fStart, fEnd, fDensity;
    std::uint32_t dwColor;
    BOOL bEnable;
};

struct VIEWPORT {
    CAMERA_DESC cameraDesc;
    VIEW_VOLUME ViewVolume;
    PLANE   planeCameraEye;
    MATRIX4 matViewInverse;
    MATRIX4 matView;
    MATRIX4 matProj;
    MATRIX4 matForBillBoard;
    std::uint32_t dwAmbientColor;
    std::uint32_t dwEmissiveColor;
    SHORT_RECT rcClip;
    std::uint16_t wClipWidth;
    std::uint16_t wClipHeight;
    BOOL bFullScreen;
    FOG_DESC fogDesc;
    DIRECTIONAL_LIGHT_DESC dirLightDesc;
};

class I3DModel;
struct MODEL_HANDLE {
    I3DModel* pModel;
    std::uint32_t dwRefIndex;
};

struct CLOCK {
    std::uint32_t dwLO, dwHI;
};

struct PERFORMANCE_CONTEXT {
    CLOCK dwClock;
    CLOCK dwUsagedClock;
    std::uint32_t dwPrvTick;
    std::uint32_t dwFrameCount;
    std::uint32_t dwAvgFrame;
};

struct DECAL_DESC {
    VECTOR3 v3Position;
    VECTOR3 v3FaceDirection;
    VECTOR3 v3UpDirection;
    VECTOR3 v3XYZScale;
    BOOL    bLookAtPivot;
    char    szMaterialName[MAX_NAME_LEN];
    std::uint32_t dwTextureCoordGenMethod;
    std::uint32_t dwTTL;
};

struct MULTI_DWORD_KEY {
    std::uint32_t dwNum;
    std::uint32_t dwKey[16];
};

struct SET_FRAME_ARGS {
    MATRIX4* pMatrixEntry;
    COLLISION_MODEL_DESC* pColModelDesc;
    MATRIX4* pParentMat;
    std::uint32_t dwFrame;
    std::uint32_t dwMotionIndex;
    MATRIX4 matBillboard;
    MATRIX4 matTransform[2];
    MATRIX4 matWorldForPhysique;
    std::uint32_t dwFlag;
};

struct FONT_PROPERTY_DESC {
    HFONT hFont;
    int   iWidth;
    int   iHeight;
    std::uint32_t dwColor;
    char* pszString;
    std::uint32_t dwStrLen;
    CHAR_CODE_TYPE type;
};

struct PICK_GXOBJECT_DESC {
    GXOBJECT_HANDLE gxo;
    VECTOR3 v3IntersectPoint;
    float   fDist;
    std::uint32_t dwModelIndex;
    std::uint32_t dwObjIndex;
};

struct LOCKED_RECT {
    INT   Pitch;
    void* pBits;
};

struct CUSTOM_EFFECT_DESC {
    char szEffectShaderName[32];
    TEXGEN_METHOD method;
    BOOL bDisableSrcTex;
    char szTexName[MAX_NAME_LEN];
    std::uint32_t dwFlag, dwReserved1, dwReserved2, dwReserved3, dwReserved4;
};

enum TRANSFORM_MATRIX_TYPE : std::uint32_t {
    TRANSFORM_MATRIX_TYPE_VIEW  = 0,
    TRANSFORM_MATRIX_TYPE_WORLD = 1,
    TRANSFORM_MATRIX_TYPE_PRJ   = 2,
};

struct AFTER_INTERPOLATION_CALL_BACK_ARG {
    std::uint32_t dwIncreasedTick;
    std::uint32_t dwTickPerFrame;
};

struct AAELLIPSOID {
    VECTOR3 P;
    float w, h;
};

struct COLLISION_RESULT {
    float fMeetTime;
    VECTOR3 vWhereMeet;
    VECTOR3 vMeetPivot;
    PLANE MeetPlane;
    std::uint32_t dwComponentType;
};

struct COLLISION_TEST_RESULT {
    VECTOR3 Candidate;
    VECTOR3 LastVelocity;
    BOOL bLand;
};

// Forward-declare callback typedefs
using ErrorHandleProc = std::uint32_t(__stdcall*)(std::uint32_t type, std::uint32_t dwErrorPriority, void* pCodeAddress, char* szStr);
using LOAD_CALLBACK_FUNC = std::uint32_t(__stdcall*)(std::uint32_t dwCurCount, std::uint32_t dwTotalCount, void* pArg);
using GX_FUNC = float();
using SHADE_FUNC = float(float, float, float, float, float);
using AfterInterpolationCallBack = std::uint32_t(__stdcall*)(AFTER_INTERPOLATION_CALL_BACK_ARG* pArg);
using CollisionTestCallBackProcedure = std::uint32_t(__stdcall*)(COLLISION_RESULT* pResult);

} // namespace mxh::gx