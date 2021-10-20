
OUTPUT_ARCH(mips)

#define START_SEGMENT(segmentName, ramAddr) \
    _##segmentName##SegmentStart = ADDR(.segmentName); \
    _##segmentName##SegmentRomStart = romPos; \
    .segmentName ramAddr : AT(romPos) SUBALIGN(16)

#define START_NOLOAD(segmentName, ramAddr) \
    _##segmentName##SegmentBssStart = ADDR(.segmentName.noload); \
    .segmentName.noload ramAddr (NOLOAD) : SUBALIGN(16)

#define START_SEGMENT_AFTER(segmentName, previousSegment) \
    _##segmentName##SegmentStart = ADDR(.segmentName); \
    _##segmentName##SegmentRomStart = romPos; \
    .segmentName ADDR(.previousSegment) + SIZEOF(.previousSegment) : AT(romPos) SUBALIGN(16)

#define START_NOLOAD_AFTER(segmentName, previousSegment) \
    _##segmentName##SegmentBssStart = ADDR(.segmentName.noload); \
    .segmentName.noload ADDR(.previousSegment) + SIZEOF(.previousSegment) (NOLOAD) : SUBALIGN(16)

#define END_SEGMENT(segmentName) \
    _##segmentName##SegmentEnd = ADDR(.segmentName) + SIZEOF(.segmentName); \
    romPos += SIZEOF(.segmentName); \
    _##segmentName##SegmentSize = SIZEOF(.segmentName); \
    _##segmentName##SegmentRomEnd = romPos

#define END_NOLOAD(segmentName) \
    _##segmentName##SegmentBssEnd = ADDR(.segmentName.noload) + SIZEOF(.segmentName.noload); \
    _##segmentName##SegmentBssSize = SIZEOF(.segmentName.noload)

SECTIONS
{
    romPos = 0;

    START_SEGMENT(header, 0xA4000000)
    {
        KEEP(BUILD_ROOT/platforms/n64/src/header/header.o(.*data));
        KEEP(BUILD_ROOT/platforms/n64/boot/boot.6102.o);
    }
    END_SEGMENT(header);

    romPos = 0x1000;

    START_SEGMENT(main, 0x80000400)
    {
        KEEP(BUILD_ROOT/platforms/n64/boot/entry.o(.text*));
        KEEP(BUILD_ROOT/codesegment.o(.text.init));
        BUILD_ROOT/codesegment.o(.text*);
        
        PR/rspboot.o(.text*);
        PR/gspF3DEX2.fifo.o(.text*);
        PR/gspL3DEX2.fifo.o(.text*);
        PR/gspS2DEX2.fifo.o(.text*);

        . = ALIGN(64);
        BUILD_ROOT/codesegment.o(.data*);
        BUILD_ROOT/codesegment.o(COMMON*);
        BUILD_ROOT/codesegment.o(.scommon*);

        PR/rspboot.o(.*data*);
        PR/gspF3DEX2.fifo.o(.*data*);
        PR/gspL3DEX2.fifo.o(.*data*);
        PR/gspS2DEX2.fifo.o(.*data*);
        BUILD_ROOT/codesegment.o(.rodata*);
        . = ALIGN(16);
    }
    END_SEGMENT(main);

    __popcountsi2 = numberOfSetBits;

    START_NOLOAD_AFTER(main, main)
    {
        . = ALIGN(64);
        BUILD_ROOT/codesegment.o(.bss*);
        . = ALIGN(16);
    }
    END_NOLOAD(main);

    memPoolStart = .;

    /DISCARD/ :
    {
        *(*);
    }
}