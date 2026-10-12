/* A hex escape ends at the first character that is not a hex digit. Until
 * 2026-10-11 the frontend took any letter for a digit: "z\x7Fw" came out as
 * 'z', 16 — one wrong byte, and the w gone. */
const char hex_w[] = "z\x7Fw";
const char hex_q[] = "\x5aQ";
const char hex_g[] = "\x4g";
const char hex_end[] = "q\x7F";
