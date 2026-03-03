/* Pin Tracker Header */

#ifndef KINETRIX_PIN_TRACKER_H
#define KINETRIX_PIN_TRACKER_H

#include "ast.h"

// Track all GPIO pins used in a program
void ast_track_pins(ASTNode *program);

#endif
