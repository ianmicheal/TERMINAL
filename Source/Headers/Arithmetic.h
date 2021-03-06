#ifndef __TERMINAL_ARITHMETIC_H__
#define __TERMINAL_ARITHMETIC_H__

#include <shinobi.h>

static const float ARI_EPSILON = 1.0e-10;
static const float ARI_HALFEPSILON = 1.0e-5;

bool ARI_IsZero( const float p_Value );
float ARI_Tangent( const float p_Angle );

#endif /* __TERMIANL_ARITHMETIC_H__ */

