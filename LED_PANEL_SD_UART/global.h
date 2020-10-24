/*
 * global.h
 *
 * Created: 10/23/2020 12:52:02 PM
 *  Author: ChrisFritz
 */ 


#ifndef GLOBAL_H_
#define GLOBAL_H_

// global AVRLIB defines
#include "avrlibdefs.h"
// global AVRLIB types definitions
#include "avrlibtypes.h"

/* project/system dependent defines */

// CPU clock speed
#define F_CPU        16000000               		// 16MHz processor
#define CYCLES_PER_US ((F_CPU+500000)/1000000) 	// cpu cycles per microsecond

// useful in any library to clear interrupts and restore entry state
#ifndef CRITICAL_SECTION_START
#define CRITICAL_SECTION_START	unsigned char _sreg = SREG; cli()
#define CRITICAL_SECTION_END	SREG = _sreg
#endif



#endif /* GLOBAL_H_ */