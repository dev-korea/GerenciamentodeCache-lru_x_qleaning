#include "lru.h"

void init_lru(LRU *lru){
	int i, j;

	for(i = 0; i < N_SETS; i++){
		for(j = 0; j < WAYS; j++){
			lru->ordem[i][j] = j;
		}
	}
}

void lru_on_hit(LRU *lru, int set_idx, int way){
	int i;
	int pos = -1;
	int valor;

	for(i = 0; i < WAYS; i++){
		if(lru->ordem[set_idx][i] == way){
			pos = i;
			break;
		}
	}

	if(pos == -1){
		return;
	}

	valor = lru->ordem[set_idx][pos];

	for(i = pos; i < WAYS - 1; i++){
		lru->ordem[set_idx][i] = lru->ordem[set_idx][i + 1];
	}

	lru->ordem[set_idx][WAYS - 1] = valor;
}

int lru_choose_victim(LRU *lru, int set_idx){
	return lru->ordem[set_idx][0];
}

void lru_on_fill(LRU *lru, int set_idx, int way){
	lru_on_hit(lru, set_idx, way);
}