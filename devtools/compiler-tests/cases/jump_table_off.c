/* L2: QBE_NO_JUMP_TABLE=1 keeps the compare tree of a dense switch. */
typedef unsigned short u16;

u16 dense(u16 v) {
    switch (v) {
    case 0: return 11;
    case 1: return 22;
    case 2: return 33;
    case 3: return 44;
    case 4: return 55;
    case 5: return 66;
    case 6: return 77;
    case 7: return 88;
    default: return 99;
    }
}
