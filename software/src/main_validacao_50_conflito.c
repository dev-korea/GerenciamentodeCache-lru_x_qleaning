#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "sim_types.h"
#include "cache.h"
#include "lru.h"
#include "qlearning.h"
#include "qlearning_fixed.h"
#include "cache_dyn.h"
#include "lru_dyn.h"

#define MAX_ACESSOS 1024

double calcular_hit_rate(int hits, int misses){
	int total;

	total = hits + misses;

	if(total > 0){
		return ((double)hits / total) * 100.0;
	}

	return 0.0;
}

int carregar_trace(const char *nome_arquivo, int acessos[]){
	FILE *fp;
	int n = 0;
	unsigned int endereco;

	fp = fopen(nome_arquivo, "r");

	if(fp == NULL){
		printf("Erro ao abrir arquivo: %s\n", nome_arquivo);
		return -1;
	}

	while(fscanf(fp, "%x", &endereco) == 1 && n < MAX_ACESSOS){
		acessos[n] = (int)endereco;
		n++;
	}

	fclose(fp);

	return n;
}

/* =========================================================
   Cache unica com LRU
   ========================================================= */

Resultado rodar_lru(int acessos[], int n){
	Cache cache;
	LRU lru;
	Resultado r;

	int i;
	int set_idx, tag;
	int hit_way, empty_way, victim_way;

	init_cache(&cache);
	init_lru(&lru);

	for(i = 0; i < n; i++){
		split_address(acessos[i], &set_idx, &tag);

		hit_way = find_hit(&cache, set_idx, tag);

		if(hit_way != -1){
			cache.hits++;
			lru_on_hit(&lru, set_idx, hit_way);
		}else{
			cache.misses++;

			empty_way = find_empty_line(&cache, set_idx);

			if(empty_way != -1){
				fill_line(&cache, set_idx, empty_way, tag);
				lru_on_fill(&lru, set_idx, empty_way);
			}else{
				victim_way = lru_choose_victim(&lru, set_idx);
				fill_line(&cache, set_idx, victim_way, tag);
				lru_on_fill(&lru, set_idx, victim_way);
			}
		}
	}

	r.hits = cache.hits;
	r.misses = cache.misses;
	r.hit_rate = calcular_hit_rate(cache.hits, cache.misses);

	return r;
}

/* =========================================================
   Cache unica com Q-Learning Float
   epsilon = 0 para validacao deterministica
   ========================================================= */

Resultado rodar_ql_float_epsilon_zero(int acessos[], int n){
	Cache cache;
	QLearning ql;
	Resultado r;

	int i;
	int set_idx, tag;
	int hit_way, empty_way, victim_way;

	init_cache(&cache);

	/* alpha=0.1, gamma=0.9, epsilon=0.0 */
	init_qlearning(&ql, 0.1f, 0.9f, 0.0f);

	for(i = 0; i < n; i++){
		split_address(acessos[i], &set_idx, &tag);

		hit_way = find_hit(&cache, set_idx, tag);

		if(hit_way != -1){
			cache.hits++;
			ql_on_hit(&ql, set_idx, tag, hit_way);
		}else{
			cache.misses++;

			empty_way = find_empty_line(&cache, set_idx);

			if(empty_way != -1){
				fill_line(&cache, set_idx, empty_way, tag);
				ql_on_miss_fill(&ql, set_idx, tag, empty_way);
			}else{
				victim_way = ql_choose_action(&ql, set_idx, tag);
				fill_line(&cache, set_idx, victim_way, tag);
				ql_on_miss_fill(&ql, set_idx, tag, victim_way);
			}
		}
	}

	r.hits = cache.hits;
	r.misses = cache.misses;
	r.hit_rate = calcular_hit_rate(cache.hits, cache.misses);

	return r;
}

/* =========================================================
   Cache unica com Q-Learning Fixed
   epsilon = 0 para validacao deterministica
   ========================================================= */

Resultado rodar_ql_fixed_epsilon_zero(int acessos[], int n){
	Cache cache;
	QLearningFixed ql;
	Resultado r;

	int i;
	int set_idx, tag;
	int hit_way, empty_way, victim_way;

	init_cache(&cache);

	/* alpha=100 -> 0.1, gamma=900 -> 0.9, epsilon=0 */
	init_qlearning_fixed(&ql, 100, 900, 0);

	for(i = 0; i < n; i++){
		split_address(acessos[i], &set_idx, &tag);

		hit_way = find_hit(&cache, set_idx, tag);

		if(hit_way != -1){
			cache.hits++;
			ql_fixed_on_hit(&ql, set_idx, tag, hit_way);
		}else{
			cache.misses++;

			empty_way = find_empty_line(&cache, set_idx);

			if(empty_way != -1){
				fill_line(&cache, set_idx, empty_way, tag);
				ql_fixed_on_miss_fill(&ql, set_idx, tag, empty_way);
			}else{
				victim_way = ql_fixed_choose_action(&ql, set_idx, tag);
				fill_line(&cache, set_idx, victim_way, tag);
				ql_fixed_on_miss_fill(&ql, set_idx, tag, victim_way);
			}
		}
	}

	r.hits = cache.hits;
	r.misses = cache.misses;
	r.hit_rate = calcular_hit_rate(cache.hits, cache.misses);

	return r;
}

/* =========================================================
   Funcoes auxiliares para hierarquia L1 + L2
   ========================================================= */

void inserir_cache_lru_dyn(CacheDyn *cache, LRUDyn *lru, int address){
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

void inserir_l1_ql_fixed_epsilon_zero(CacheDyn *l1, QLearningFixed *ql, int address){
	int set_idx, tag;
	int hit_way, empty_way, victim_way;

	split_address_dyn(l1, address, &set_idx, &tag);

	hit_way = find_hit_dyn(l1, set_idx, tag);

	if(hit_way != -1){
		ql_fixed_on_hit(ql, set_idx, tag, hit_way);
		return;
	}

	empty_way = find_empty_line_dyn(l1, set_idx);

	if(empty_way != -1){
		fill_line_dyn(l1, set_idx, empty_way, tag);
		ql_fixed_on_miss_fill(ql, set_idx, tag, empty_way);
	}else{
		victim_way = ql_fixed_choose_action(ql, set_idx, tag);
		fill_line_dyn(l1, set_idx, victim_way, tag);
		ql_fixed_on_miss_fill(ql, set_idx, tag, victim_way);
	}
}

/* =========================================================
   Hierarquia L1 LRU + L2 LRU
   ========================================================= */

ResultadoHierarquia rodar_hierarquia_lru_lru(int acessos[], int n){
	CacheDyn l1;
	CacheDyn l2;

	LRUDyn lru_l1;
	LRUDyn lru_l2;

	ResultadoHierarquia r;

	int i;
	int address;
	int set_l1, tag_l1;
	int set_l2, tag_l2;
	int hit_l1, hit_l2;

	r.l1_hits = 0;
	r.l1_misses = 0;
	r.l2_hits = 0;
	r.l2_misses = 0;

	init_cache_dyn(&l1, L1_SIZE, L1_BLOCK, L1_WAYS);
	init_cache_dyn(&l2, L2_SIZE, L2_BLOCK, L2_WAYS);

	init_lru_dyn(&lru_l1, L1_WAYS);
	init_lru_dyn(&lru_l2, L2_WAYS);

	for(i = 0; i < n; i++){
		address = acessos[i];

		split_address_dyn(&l1, address, &set_l1, &tag_l1);
		hit_l1 = find_hit_dyn(&l1, set_l1, tag_l1);

		if(hit_l1 != -1){
			r.l1_hits++;
			lru_dyn_on_hit(&lru_l1, set_l1, hit_l1);
			continue;
		}

		r.l1_misses++;

		split_address_dyn(&l2, address, &set_l2, &tag_l2);
		hit_l2 = find_hit_dyn(&l2, set_l2, tag_l2);

		if(hit_l2 != -1){
			r.l2_hits++;
			lru_dyn_on_hit(&lru_l2, set_l2, hit_l2);

			inserir_cache_lru_dyn(&l1, &lru_l1, address);
			continue;
		}

		r.l2_misses++;

		inserir_cache_lru_dyn(&l2, &lru_l2, address);
		inserir_cache_lru_dyn(&l1, &lru_l1, address);
	}

	return r;
}

/* =========================================================
   Hierarquia L1 QL Fixed epsilon=0 + L2 LRU
   ========================================================= */

ResultadoHierarquia rodar_hierarquia_ql_lru_epsilon_zero(int acessos[], int n){
	CacheDyn l1;
	CacheDyn l2;

	QLearningFixed ql_l1;
	LRUDyn lru_l2;

	ResultadoHierarquia r;

	int i;
	int address;
	int set_l1, tag_l1;
	int set_l2, tag_l2;
	int hit_l1, hit_l2;

	r.l1_hits = 0;
	r.l1_misses = 0;
	r.l2_hits = 0;
	r.l2_misses = 0;

	init_cache_dyn(&l1, L1_SIZE, L1_BLOCK, L1_WAYS);
	init_cache_dyn(&l2, L2_SIZE, L2_BLOCK, L2_WAYS);

	init_qlearning_fixed(&ql_l1, 100, 900, 0);
	init_lru_dyn(&lru_l2, L2_WAYS);

	for(i = 0; i < n; i++){
		address = acessos[i];

		split_address_dyn(&l1, address, &set_l1, &tag_l1);
		hit_l1 = find_hit_dyn(&l1, set_l1, tag_l1);

		if(hit_l1 != -1){
			r.l1_hits++;
			ql_fixed_on_hit(&ql_l1, set_l1, tag_l1, hit_l1);
			continue;
		}

		r.l1_misses++;

		split_address_dyn(&l2, address, &set_l2, &tag_l2);
		hit_l2 = find_hit_dyn(&l2, set_l2, tag_l2);

		if(hit_l2 != -1){
			r.l2_hits++;
			lru_dyn_on_hit(&lru_l2, set_l2, hit_l2);

			inserir_l1_ql_fixed_epsilon_zero(&l1, &ql_l1, address);
			continue;
		}

		r.l2_misses++;

		inserir_cache_lru_dyn(&l2, &lru_l2, address);
		inserir_l1_ql_fixed_epsilon_zero(&l1, &ql_l1, address);
	}

	return r;
}

/* =========================================================
   Impressao e validacao
   ========================================================= */

void validar_resultado(const char *nome, int hits, int misses, int hits_esperado, int misses_esperado){
	double hr;

	hr = calcular_hit_rate(hits, misses);

	printf("%s -> Hits: %d | Misses: %d | HitRate: %.2f%% ",
		nome,
		hits,
		misses,
		hr
	);

	if(hits == hits_esperado && misses == misses_esperado){
		printf("[OK]\n");
	}else{
		printf("[ERRO] esperado Hits=%d Misses=%d\n", hits_esperado, misses_esperado);
	}
}

int main(int argc, char *argv[]){
	int acessos[MAX_ACESSOS];
	int n;

	const char *arquivo;

	Resultado r_lru;
	Resultado r_ql_float;
	Resultado r_ql_fixed;

	ResultadoHierarquia h_lru_lru;
	ResultadoHierarquia h_ql_lru;

	if(argc >= 2){
		arquivo = argv[1];
	}else{
		arquivo = "traces/trace_validacao_50_conflito.txt";
	}

	n = carregar_trace(arquivo, acessos);

	if(n <= 0){
		printf("Nao foi possivel carregar o trace.\n");
		return 1;
	}

	printf("=== VALIDACAO COM TRACE DE 50 ENTRADAS - CONFLITO FORTE ===\n");
	printf("Arquivo carregado: %s\n", arquivo);
	printf("Total de acessos: %d\n", n);

	if(n != 50){
		printf("Aviso: o arquivo deveria ter 50 acessos.\n");
	}

	printf("\nConfiguracao do teste:\n");
	printf("L1 = 4 KB | bloco = 32 bytes | 2 vias | 64 conjuntos\n");
	printf("Trace usa os enderecos 0x0000, 0x0800 e 0x1000 no mesmo set.\n");
	printf("Q-Learning usa epsilon = 0 para resultado deterministico.\n");

	printf("\n===== CACHE UNICA =====\n");

	r_lru = rodar_lru(acessos, n);

	srand(42);
	r_ql_float = rodar_ql_float_epsilon_zero(acessos, n);

	srand(42);
	r_ql_fixed = rodar_ql_fixed_epsilon_zero(acessos, n);

	validar_resultado("LRU", r_lru.hits, r_lru.misses, 0, 50);
	validar_resultado("Q-Learning Float", r_ql_float.hits, r_ql_float.misses, 14, 36);
	validar_resultado("Q-Learning Fixed", r_ql_fixed.hits, r_ql_fixed.misses, 14, 36);

	printf("\n===== HIERARQUIA L1 + L2 =====\n");

	h_lru_lru = rodar_hierarquia_lru_lru(acessos, n);

	srand(42);
	h_ql_lru = rodar_hierarquia_ql_lru_epsilon_zero(acessos, n);

	printf("\nL1 LRU + L2 LRU\n");
	validar_resultado("L1", h_lru_lru.l1_hits, h_lru_lru.l1_misses, 0, 50);
	validar_resultado("L2", h_lru_lru.l2_hits, h_lru_lru.l2_misses, 47, 3);

	printf("\nL1 Q-Learning Fixed + L2 LRU\n");
	validar_resultado("L1", h_ql_lru.l1_hits, h_ql_lru.l1_misses, 14, 36);
	validar_resultado("L2", h_ql_lru.l2_hits, h_ql_lru.l2_misses, 33, 3);

	printf("\nValidacao finalizada.\n");

	return 0;
}