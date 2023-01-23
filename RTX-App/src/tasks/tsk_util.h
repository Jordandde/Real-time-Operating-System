#include "k_inc.h"


char *stringCopy(char* dest, const char* source);
int strlen(char *source);
void prep_msg(U8 *msgBuf, char *str, int len, int sender, int tid);
void copyHeader(RTX_MSG_HDR *headerInfo, U8 *buf);