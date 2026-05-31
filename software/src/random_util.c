#include <stdlib.h>
#include "random_util.h"

int rand_pct(void){
    return rand() % 100;
}

int rand_way(int n){
    return rand() % n;
}
