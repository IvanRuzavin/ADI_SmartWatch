/* SLA (Secure Loader Area) header, embedded at the start of the FLASH image
 * so that sign_app.exe patches this reserved 512 B block in place instead of
 * prepending its own fallback header (which shifted the code by 256 B).
 * Mirrors: C:/MaximSDK/Libraries/CMSIS/Device/Maxim/MAX32690/Source/sla_header_MAX32690.c
 */

#define SWAP(x) \
    (((x) >> 24) | (((x)&0x00FF0000) >> 8) | (((x)&0x0000FF00) << 8) | (((x)&0x000000FF) << 24))

typedef enum {
    ROM_A1_VERSION = 0x01000000,
} enum_rom_version_t;

extern unsigned int _start_SWAP;
extern unsigned int _SLA_Size_SWAP;

typedef enum {
    MagicH = 0x48495357,
    MagicL = 0x45444744,
} enum_magic_t;

typedef struct {
    enum_magic_t MagicHigh;
    enum_magic_t MagicLow;
} magic_t;

typedef struct {
    magic_t Magic;
    enum_rom_version_t RomVersion;
    unsigned int LoadAddr;
    unsigned int SLA_CodeSize;
    unsigned int *JumpAddr;
    unsigned int ArgSize;
    unsigned int AppVersionNumber;
} flash_app_header_t;

__attribute__((section(".sb_sla_header"))) __attribute__((__used__))
const flash_app_header_t sb_header = {
    .Magic =
        {
            .MagicHigh = SWAP(MagicH),
            .MagicLow  = SWAP(MagicL),
        },
    .RomVersion       = SWAP(ROM_A1_VERSION),
    .LoadAddr         = SWAP(0x10000000),
    .SLA_CodeSize     = (unsigned int)&_SLA_Size_SWAP,
    .JumpAddr         = &_start_SWAP,
    .ArgSize          = 0,
    .AppVersionNumber = SWAP(0x01000000),
};
