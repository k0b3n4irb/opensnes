/**
 * @file main.c
 * @brief Calculator - Assembly handles everything
 *
 * C just calls the assembly main loop which does all init and runtime.
 */

/* Assembly main loop - does all init and never returns */
extern void calc_main_loop(void);

int main(void) {
    calc_main_loop();
    return 0;
}
