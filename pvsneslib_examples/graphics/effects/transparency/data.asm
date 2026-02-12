.section ".rodata1" superfree
land_tiles:
.incbin "res/backgrounds.pic"
land_tiles_end:
.ends

.section ".rodata2" superfree
land_map:
.incbin "res/backgrounds.map"
land_map_end:

land_pal:
.incbin "res/backgrounds.pal"
land_pal_end:
.ends

.section ".rodata3" superfree
cloud_tiles:
.incbin "res/clouds.pic"
cloud_tiles_end:
.ends

.section ".rodata4" superfree
cloud_map:
.incbin "res/clouds.map"
cloud_map_end:

cloud_pal:
.incbin "res/clouds.pal"
cloud_pal_end:
.ends
