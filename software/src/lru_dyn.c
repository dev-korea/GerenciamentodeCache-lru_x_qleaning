#include "lru_dyn.h"

void init_lru_dyn(LRUDyn *lru, int ways){
	int i, j;

	lru->ways = ways;

	for(i = 0; i < MAX_SETS; i++){
		for(j = 0; j < MAX_WAYS; j++){
			lru->ordem[i][j] = j;
		}
	}
}

void lru_dyn_on_hit(LRUDyn *lru, int set_idx, int way){
	int i;
	int pos = -1;
	int valor;

	for(i = 0; i < lru->ways; i++){
		if(lru->ordem[set_idx][i] == way){
			pos = i;
			break;
		}
	}

	if(pos == -1){
		return;
	}

	valor = lru->ordem[set_idx][pos];

	for(i = pos; i < lru->ways - 1; i++){
		lru->ordem[set_idx][i] = lru->ordem[set_idx][i + 1];
	}

	lru->ordem[set_idx][lru->ways - 1] = valor;
}

int lru_dyn_choose_victim(LRUDyn *lru, int set_idx){
	return lru->ordem[set_idx][0];
}

void lru_dyn_on_fill(LRUDyn *lru, int set_idx, int way){
	lru_dyn_on_hit(lru, set_idx, way);
}