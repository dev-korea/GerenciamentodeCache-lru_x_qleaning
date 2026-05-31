#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "sim_types.h"
#include "cache_dyn.h"
#include "lru_dyn.h"
#include "qlearning_fixed.h"

#define MAX_TRACE 128

/* =========================================================
   Funcoes auxiliares
   ========================================================= */

double hit_rate(int hits, int misses){
	int total;

	total = hits + misses;

	if(total > 0){
		return ((double)hits / total) * 100.0;
	}

	return 0.0;
}

void print_resultado_hierarquia(const char *nome, ResultadoHierarquia r){
	printf("\n===== RESULTADO: %s =====\n", nome);

	printf("L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r.l1_hits,
		r.l1_misses,
		hit_rate(r.l1_hits, r.l1_misses));

	printf("L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r.l2_hits,
		r.l2_misses,
		hit_rate(r.l2_hits, r.l2_misses));

	printf("Acessos a memoria principal: %d\n", r.l2_misses);
}

void salvar_resultado(FILE *fp, const char *nome_trace, ResultadoHierarquia r_lru, ResultadoHierarquia r_ql){
	fprintf(fp, "\n===== TRACE: %s =====\n", nome_trace);

	fprintf(fp, "L1 LRU + L2 LRU\n");
	fprintf(fp, "L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_lru.l1_hits,
		r_lru.l1_misses,
		hit_rate(r_lru.l1_hits, r_lru.l1_misses));

	fprintf(fp, "L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_lru.l2_hits,
		r_lru.l2_misses,
		hit_rate(r_lru.l2_hits, r_lru.l2_misses));

	fprintf(fp, "Acessos a memoria principal: %d\n", r_lru.l2_misses);

	fprintf(fp, "\nL1 Q-Learning Fixed + L2 LRU\n");
	fprintf(fp, "L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_ql.l1_hits,
		r_ql.l1_misses,
		hit_rate(r_ql.l1_hits, r_ql.l1_misses));

	fprintf(fp, "L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_ql.l2_hits,
		r_ql.l2_misses,
		hit_rate(r_ql.l2_hits, r_ql.l2_misses));

	fprintf(fp, "Acessos a memoria principal: %d\n", r_ql.l2_misses);
}

/* =========================================================
   Insercao usando LRU
   Nao conta hit/miss, apenas insere ou atualiza politica
   ========================================================= */

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

/* =========================================================
   Insercao na L1 usando Q-Learning Fixed
   Nao conta hit/miss, apenas decide via e atualiza Q-table
   ========================================================= */

void insert_l1_qlearning_fixed(CacheDyn *l1, QLearningFixed *ql, int address){
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
   Simulacao 1: L1 LRU + L2 LRU
   ========================================================= */

ResultadoHierarquia simular_lru_lru(int acessos[], int n, int verbose){
	CacheDyn l1;
	CacheDyn l2;

	LRUDyn lru_l1;
	LRUDyn lru_l2;

	ResultadoHierarquia r;

	int i;
	int address;

	int set_l1, tag_l1;
	int set_l2, tag_l2;

	int hit_l1;
	int hit_l2;

	r.l1_hits = 0;
	r.l1_misses = 0;
	r.l2_hits = 0;
	r.l2_misses = 0;

	init_cache_dyn(&l1, L1_SIZE, L1_BLOCK, L1_WAYS);
	init_cache_dyn(&l2, L2_SIZE, L2_BLOCK, L2_WAYS);

	init_lru_dyn(&lru_l1, L1_WAYS);
	init_lru_dyn(&lru_l2, L2_WAYS);

	if(verbose){
		printf("\n=== HIERARQUIA: L1 LRU + L2 LRU ===\n");
	}

	for(i = 0; i < n; i++){
		address = acessos[i];

		split_address_dyn(&l1, address, &set_l1, &tag_l1);
		hit_l1 = find_hit_dyn(&l1, set_l1, tag_l1);

		if(verbose){
			printf("\nAcesso 0x%04X", address);
		}

		if(hit_l1 != -1){
			r.l1_hits++;
			lru_dyn_on_hit(&lru_l1, set_l1, hit_l1);

			if(verbose){
				printf(" -> HIT L1");
			}

			continue;
		}

		r.l1_misses++;

		if(verbose){
			printf(" -> MISS L1");
		}

		split_address_dyn(&l2, address, &set_l2, &tag_l2);
		hit_l2 = find_hit_dyn(&l2, set_l2, tag_l2);

		if(hit_l2 != -1){
			r.l2_hits++;
			lru_dyn_on_hit(&lru_l2, set_l2, hit_l2);

			if(verbose){
				printf(" | HIT L2");
			}

			insert_cache_lru(&l1, &lru_l1, address);
			continue;
		}

		r.l2_misses++;

		if(verbose){
			printf(" | MISS L2 | Busca memoria principal");
		}

		insert_cache_lru(&l2, &lru_l2, address);
		insert_cache_lru(&l1, &lru_l1, address);
	}

	return r;
}

/* =========================================================
   Simulacao 2: L1 Q-Learning Fixed + L2 LRU
   ========================================================= */

ResultadoHierarquia simular_ql_lru(int acessos[], int n, int verbose){
	CacheDyn l1;
	CacheDyn l2;

	QLearningFixed ql_l1;
	LRUDyn lru_l2;

	ResultadoHierarquia r;

	int i;
	int address;

	int set_l1, tag_l1;
	int set_l2, tag_l2;

	int hit_l1;
	int hit_l2;

	r.l1_hits = 0;
	r.l1_misses = 0;
	r.l2_hits = 0;
	r.l2_misses = 0;

	init_cache_dyn(&l1, L1_SIZE, L1_BLOCK, L1_WAYS);
	init_cache_dyn(&l2, L2_SIZE, L2_BLOCK, L2_WAYS);

	init_qlearning_fixed(&ql_l1, QL_DEFAULT_ALPHA, QL_DEFAULT_GAMMA, QL_DEFAULT_EPSILON);
	init_lru_dyn(&lru_l2, L2_WAYS);

	if(verbose){
		printf("\n=== HIERARQUIA: L1 Q-LEARNING FIXED + L2 LRU ===\n");
	}

	for(i = 0; i < n; i++){
		address = acessos[i];

		split_address_dyn(&l1, address, &set_l1, &tag_l1);
		hit_l1 = find_hit_dyn(&l1, set_l1, tag_l1);

		if(verbose){
			printf("\nAcesso 0x%04X", address);
		}

		if(hit_l1 != -1){
			r.l1_hits++;
			ql_fixed_on_hit(&ql_l1, set_l1, tag_l1, hit_l1);

			if(verbose){
				printf(" -> HIT L1");
			}

			continue;
		}

		r.l1_misses++;

		if(verbose){
			printf(" -> MISS L1");
		}

		split_address_dyn(&l2, address, &set_l2, &tag_l2);
		hit_l2 = find_hit_dyn(&l2, set_l2, tag_l2);

		if(hit_l2 != -1){
			r.l2_hits++;
			lru_dyn_on_hit(&lru_l2, set_l2, hit_l2);

			if(verbose){
				printf(" | HIT L2");
			}

			insert_l1_qlearning_fixed(&l1, &ql_l1, address);
			continue;
		}

		r.l2_misses++;

		if(verbose){
			printf(" | MISS L2 | Busca memoria principal");
		}

		insert_cache_lru(&l2, &lru_l2, address);
		insert_l1_qlearning_fixed(&l1, &ql_l1, address);
	}

	return r;
}

/* =========================================================
   Executa um trace nas duas configuracoes
   ========================================================= */

void executar_trace(const char *nome_trace, int acessos[], int n, FILE *fp){
	ResultadoHierarquia r_lru_lru;
	ResultadoHierarquia r_ql_lru;

	r_lru_lru = simular_lru_lru(acessos, n, 0);

	srand(42);
	r_ql_lru = simular_ql_lru(acessos, n, 0);

	printf("\n========================================\n");
	printf("TRACE: %s | total de acessos: %d\n", nome_trace, n);

	print_resultado_hierarquia("L1 LRU + L2 LRU", r_lru_lru);
	print_resultado_hierarquia("L1 Q-Learning Fixed + L2 LRU", r_ql_lru);

	if(fp != NULL){
		salvar_resultado(fp, nome_trace, r_lru_lru, r_ql_lru);
	}
}

/* =========================================================
   Geradores de traces
   ========================================================= */

int gerar_trace_streaming(int trace[]){
	int i;
	int n = 32;

	for(i = 0; i < n; i++){
		trace[i] = i * 0x20;
	}

	return n;
}

int gerar_trace_hotset(int trace[]){
	int i;
	int base[] = {
		0x0000, 0x0020, 0x0040, 0x0060
	};

	int n = 32;

	for(i = 0; i < n; i++){
		trace[i] = base[i % 4];
	}

	return n;
}

int gerar_trace_conflito_l1(int trace[]){
	int i;
	int base[] = {
		0x0000, 0x0800, 0x1000
	};

	int n = 24;

	for(i = 0; i < n; i++){
		trace[i] = base[i % 3];
	}

	return n;
}

int gerar_trace_misto(int trace[]){
	int base[] = {
		0x0000, 0x0020, 0x0040, 0x0060,
		0x0000, 0x0020, 0x1000, 0x2000,
		0x0000, 0x0020, 0x0040, 0x0060,
		0x3000, 0x4000, 0x0000, 0x0020
	};

	int i;
	int n = sizeof(base) / sizeof(base[0]);

	for(i = 0; i < n; i++){
		trace[i] = base[i];
	}

	return n;
}

int gerar_trace_reuso_com_interferencia(int trace[]){
	int base[] = {
		0x0000, 0x0020, 0x0000, 0x0020,
		0x0800, 0x1000,
		0x0000, 0x0020,
		0x1800, 0x2000,
		0x0000, 0x0020,
		0x0040, 0x0060,
		0x0000, 0x0020
	};

	int i;
	int n = sizeof(base) / sizeof(base[0]);

	for(i = 0; i < n; i++){
		trace[i] = base[i];
	}

	return n;
}

/* =========================================================
   Main
   ========================================================= */

int main(){
	FILE *fp;

	int trace[MAX_TRACE];
	int n;

	fp = fopen("results/resultado_hierarquia.txt", "w");

	if(fp == NULL){
		printf("Aviso: nao foi possivel criar results/resultado_hierarquia.txt\n");
		printf("Os resultados serao exibidos apenas no terminal.\n");
	}

	printf("=== AVALIACAO HIERARQUIA L1 + L2 ===\n");

	printf("\nConfiguracao L1: %d bytes | bloco %d | %d vias\n",
		L1_SIZE, L1_BLOCK, L1_WAYS);

	printf("Configuracao L2: %d bytes | bloco %d | %d vias\n",
		L2_SIZE, L2_BLOCK, L2_WAYS);

	if(fp != NULL){
		fprintf(fp, "=== AVALIACAO HIERARQUIA L1 + L2 ===\n");

		fprintf(fp, "\nConfiguracao L1: %d bytes | bloco %d | %d vias\n",
			L1_SIZE, L1_BLOCK, L1_WAYS);

		fprintf(fp, "Configuracao L2: %d bytes | bloco %d | %d vias\n",
			L2_SIZE, L2_BLOCK, L2_WAYS);
	}

	n = gerar_trace_streaming(trace);
	executar_trace("Streaming sequencial", trace, n, fp);

	n = gerar_trace_hotset(trace);
	executar_trace("Hotset / boa localidade", trace, n, fp);

	n = gerar_trace_conflito_l1(trace);
	executar_trace("Conflito forte na L1", trace, n, fp);

	n = gerar_trace_misto(trace);
	executar_trace("Misto", trace, n, fp);

	n = gerar_trace_reuso_com_interferencia(trace);
	executar_trace("Reuso com interferencia", trace, n, fp);

	if(fp != NULL){
		fclose(fp);
		printf("\nResultados salvos em: results/resultado_hierarquia.txt\n");
	}

	return 0;
}