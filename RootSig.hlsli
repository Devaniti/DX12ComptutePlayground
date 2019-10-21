#define RootSig "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                "DENY_VERTEX_SHADER_ROOT_ACCESS | " \
                "DENY_HULL_SHADER_ROOT_ACCESS | " \
                "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                "DENY_GEOMETRY_SHADER_ROOT_ACCESS | " \
                "DENY_PIXEL_SHADER_ROOT_ACCESS), " \
                "DescriptorTable(UAV(u0, numDescriptors = 2, flags = DATA_STATIC)) "