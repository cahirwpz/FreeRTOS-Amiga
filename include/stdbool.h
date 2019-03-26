#ifndef _STDBOOL_H_
#define _STDBOOL_H_

#ifndef __bool_true_false_are_defined
typedef unsigned char bool; /* VBCC does not support _Bool :( */
#define true 1
#define false 0
#define __bool_true_false_are_defined 1
#endif /* !__bool_true_false_are_defined */

#endif /* !_STDBOOL_H_ */
