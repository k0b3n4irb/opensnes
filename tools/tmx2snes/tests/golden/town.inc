/* Generated from the Entities layer by tmx2snes -e.
 * Do not edit: edit the map. */

#define CHEST_COUNT 1
#define CHEST_FIELDS \
    u8 tx; u8 ty;
#define CHEST_TABLE { \
    { 31, 28 } \
}
#define CHEST_TX 31
#define CHEST_TY 28

#define NPC_COUNT 2
#define NPC_FIELDS \
    u8 tx; u8 ty; const char *text;
#define NPC_TABLE { \
    { 28, 33, "HEY! HOW ARE YOU?" }, \
    { 34, 31, "WELCOME, TRAVELER!" } \
}

#define SPAWN_COUNT 1
#define SPAWN_FIELDS \
    u8 tx; u8 ty;
#define SPAWN_TABLE { \
    { 31, 34 } \
}
#define SPAWN_TX 31
#define SPAWN_TY 34
