/*
** Machine registers mr0 - mr36 for the SPARC architecture.
**
** The first NUM_REAL_REGS of these are real machine registers.
** The others are just slots in a global array.
**
** This is a bit tricky on sparcs, because of the sliding register
** windows. There are only seven global registers (g1-g7), and of
** these three (g5-g7) are reserved for use by the operating system,
** and the other four (g1-g4) are all temp registers that get clobbered
** by calls to the C standard library functions.
**
** So it looks like we'll have to use the sliding registers.
** This won't work at all unless we are using gcc's non-local gotos.
*/

#ifndef USE_GCC_NONLOCAL_GOTOS
  #include "no_regs.h"
#else

#define NUM_REAL_REGS 10

reg 	Word	mr0 __asm__("i0");
reg	Word	mr1 __asm__("i1");	/* potentially non-clobbered */
reg	Word	mr2 __asm__("i2");	/* potentially non-clobbered */
reg	Word	mr3 __asm__("i3");	/* potentially non-clobbered */
reg	Word	mr4 __asm__("i4");	/* potentially non-clobbered */
reg	Word	mr5 __asm__("i5");	/* potentially non-clobbered */
reg	Word	mr6 __asm__("l1");
reg	Word	mr7 __asm__("l2");
reg	Word	mr8 __asm__("l3");
reg	Word	mr9 __asm__("l4");

/* we could use l5, l6, and l7 as well, */
/* but for the moment at least I'll leave them for gcc */

#define save_registers()			\
	(					\
		saved_regs[0] = mr0,		\
		saved_regs[1] = mr1,		\
		saved_regs[2] = mr2,		\
		saved_regs[3] = mr3,		\
		saved_regs[4] = mr4,		\
		saved_regs[5] = mr5,		\
		saved_regs[6] = mr6,		\
		saved_regs[7] = mr7,		\
		saved_regs[8] = mr8,		\
		saved_regs[9] = mr9,		\
		(void)0				\
	)

#define restore_registers()			\
	(					\
		mr0 = saved_regs[0],		\
		mr1 = saved_regs[1],		\
		mr2 = saved_regs[2],		\
		mr3 = saved_regs[3],		\
		mr4 = saved_regs[4],		\
		mr5 = saved_regs[5],		\
		mr6 = saved_regs[6],		\
		mr7 = saved_regs[7],		\
		mr8 = saved_regs[8],		\
		mr9 = saved_regs[9],		\
		(void)0				\
	)

#define	mr10	unreal_reg_10
#define	mr11	unreal_reg_11
#define	mr12	unreal_reg_12
#define	mr13	unreal_reg_13
#define	mr14	unreal_reg_14
#define	mr15	unreal_reg_15
#define	mr16	unreal_reg_16
#define	mr17	unreal_reg_17
#define	mr18	unreal_reg_18
#define	mr19	unreal_reg_19
#define	mr20	unreal_reg_20
#define	mr21	unreal_reg_21
#define	mr22	unreal_reg_22
#define	mr23	unreal_reg_23
#define	mr24	unreal_reg_24
#define	mr25	unreal_reg_25
#define	mr26	unreal_reg_26
#define	mr27	unreal_reg_27
#define	mr28	unreal_reg_28
#define	mr29	unreal_reg_29
#define	mr30	unreal_reg_30
#define	mr31	unreal_reg_31
#define	mr32	unreal_reg_32
#define	mr33	unreal_reg_33
#define	mr34	unreal_reg_34
#define	mr35	unreal_reg_35
#define	mr36	unreal_reg_36

#endif
