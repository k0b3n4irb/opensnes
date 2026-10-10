const t_metasprite mirror_metasprite0[] = {
	METASPR_ITEM(0, 0, 0, OBJ_PAL(0) | OBJ_PRIO(0)),
	METASPR_TERM
};

const t_metasprite mirror_metasprite1[] = {
	METASPR_ITEM(0, 0, 0, OBJ_PAL(0) | OBJ_PRIO(0) | OBJ_FLIPX),
	METASPR_TERM
};

const t_metasprite mirror_metasprite2[] = {
	METASPR_ITEM(0, 0, 4, OBJ_PAL(0) | OBJ_PRIO(0)),
	METASPR_TERM
};

const t_metasprite mirror_metasprite3[] = {
	METASPR_ITEM(0, 0, 4, OBJ_PAL(0) | OBJ_PRIO(0) | OBJ_FLIPX),
	METASPR_TERM
};

const t_metasprite* mirror_metasprites[4] = {
	mirror_metasprite0, mirror_metasprite1, mirror_metasprite2, mirror_metasprite3
};
