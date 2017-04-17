#ifndef _SDMBNLocal_H_
#define _SDMBNLocal_H_

#include <SDMBN.h>

struct get_perflow_args
{
    PerflowKey *key;
    int id;
};

///// FUNCTION PROTOTYPES ////////////////////////////////////////////////////
int local_get_perflow(PerflowKey *key, int id, int raiseEvents);
int local_put_perflow(int hashkey, PerflowKey *key, char *state);
int local_get_config(int id);
int local_put_config(int hashkey, char *state);
int local_del_perflow(PerflowKey *key, int id);

#endif
