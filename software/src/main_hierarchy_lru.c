#include <stdio.h>
#include "config.h"
#include "cache_dyn.h"
#include "lru_dyn.h"
#include "sim_debug.h"

void insert_cache_lru(CacheDyn *cache, LRUDyn *lru, int address){
	int set_idx, tag;
	int hit_way, empty_way, victim_way;

	split_address_dyn(cache, address, &set_idx, &tag);

	hit_way = find_hit_dyn(cache, set_idx, tag);

	if(hit_way != -1){
		lru_dyn_on_hit(lru, set_idx, hit_way);
		return;
	}

	empty_way = find_empty_line_dyn(cache, set_idx);

	if(empty_way != -1){
		fill_line_dyn(cache, set_idx, empty_way, tag);
		lru_dyn_on_fill(lru, set_idx, empty_way);
	}else{
		victim_way = lru_dyn_choose_victim(lru, set_idx);
		fill_line_dyn(cache, set_idx, victim_way, tag);
		lru_dyn_on_fill(lru, set_idx, victim_way);
	}
}

void access_hierarchy_lru(CacheDyn *l1, LRUDyn *lru_l1, CacheDyn *l2, LRUDyn *lru_l2, int address){
	int set_l1, tag_l1;
	int set_l2, tag_l2;
	int hit_l1, hit_l2;

	split_address_dyn(l1, address, &set_l1, &tag_l1);
	hit_l1 = find_hit_dyn(l1, set_l1, tag_l1);

	printf("\nAcesso 0x%04X", address);

	if(hit_l1 != -1){
		l1->hits++;
		lru_dyn_on_hit(lru_l1, set_l1, hit_l1);
		printf(" -> HIT L1");
		return;
	}

	l1->misses++;
	printf(" -> MISS L1");

	split_address_dyn(l2, address, &set_l2, &tag_l2);
	hit_l2 = find_hit_dyn(l2, set_l2, tag_l2);

	if(hit_l2 != -1){
		l2->hits++;
		lru_dyn_on_hit(lru_l2, set_l2, hit_l2);
		printf(" | HIT L2");

		insert_cache_lru(l1, lru_l1, address);
		return;
	}

	l2->misses++;
	printf(" | MISS L2 | Busca memoria principal");

	insert_cache_lru(l2, lru_l2, address);
	insert_cache_lru(l1, lru_l1, address);
}

int main(){
	CacheDyn l1;
	CacheDyn l2;

	LRUDyn lru_l1;
	LRUDyn lru_l2;

	int acessos[] = {
		0x0000, 0x0020, 0x0040, 0x0060,
		0x0000, 0x0020, 0x1000, 0x2000,
		0x0000, 0x0020, 0x0040, 0x0060,
		0x3000, 0x4000, 0x0000, 0x0020
	};

	int n = sizeof(acessos) / sizeof(acessos[0]);
	int i;

	init_cache_dyn(&l1, L1_SIZE, L1_BLOCK, L1_WAYS);
	init_cache_dyn(&l2, L2_SIZE, L2_BLOCK, L2_WAYS);

	init_lru_dyn(&lru_l1, L1_WAYS);
	init_lru_dyn(&lru_l2, L2_WAYS);

	printf("=== SIMULADOR HIERARQUIA L1 + L2 COM LRU ===\n");

	printf("\nConfiguracao L1: %d bytes | bloco %d | %d vias | %d conjuntos\n",
		l1.size_bytes, l1.block_size, l1.ways, l1.n_sets);

	printf("Configuracao L2: %d bytes | bloco %d | %d vias | %d conjuntos\n",
		l2.size_bytes, l2.block_size, l2.ways, l2.n_sets);

	for(i = 0; i < n; i++){
		access_hierarchy_lru(&l1, &lru_l1, &l2, &lru_l2, acessos[i]);
	}

	print_cache_dyn_stats(&l1, "L1");
	print_cache_dyn_stats(&l2, "L2");

	return 0;
}