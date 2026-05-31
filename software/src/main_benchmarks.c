#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "sim_types.h"
#include "cache_dyn.h"
#include "lru_dyn.h"
#include "qlearning_fixed.h"

double hit_rate(int hits, int misses){
	int total;

	total = hits + misses;

	if(total > 0){
		return ((double)hits / total) * 100.0;
	}

	return 0.0;
}

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

ResultadoHierarquia simular_lru_lru(int acessos[], int n){
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
			insert_cache_lru(&l1, &lru_l1, address);
			continue;
		}

		r.l2_misses++;

		insert_cache_lru(&l2, &lru_l2, address);
		insert_cache_lru(&l1, &lru_l1, address);
	}

	return r;
}

ResultadoHierarquia simular_ql_lru(int acessos[], int n){
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

	init_qlearning_fixed(&ql_l1, QL_DEFAULT_ALPHA, QL_DEFAULT_GAMMA, QL_DEFAULT_EPSILON);
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
			insert_l1_qlearning_fixed(&l1, &ql_l1, address);
			continue;
		}

		r.l2_misses++;

		insert_cache_lru(&l2, &lru_l2, address);
		insert_l1_qlearning_fixed(&l1, &ql_l1, address);
	}

	return r;
}

/* =========================================================
   Geradores de benchmark
   ========================================================= */

int benchmark_streaming(int trace[]){
	int i;
	int n = 512;

	for(i = 0; i < n; i++){
		trace[i] = i * 0x20;
	}

	return n;
}

int benchmark_hotset_pequeno(int trace[]){
	int i;
	int base[] = {
		0x0000, 0x0020, 0x0040, 0x0060
	};

	int n = 512;

	for(i = 0; i < n; i++){
		trace[i] = base[i % 4];
	}

	return n;
}

int benchmark_conflito_l1(int trace[]){
	int i;
	int base[] = {
		0x0000, 0x0800, 0x1000
	};

	int n = 512;

	for(i = 0; i < n; i++){
		trace[i] = base[i % 3];
	}

	return n;
}

int benchmark_reuso_interferencia(int trace[]){
	int i;
	int base[] = {
		0x0000, 0x0020, 0x0000, 0x0020,
		0x0800, 0x1000,
		0x0000, 0x0020,
		0x1800, 0x2000,
		0x0000, 0x0020,
		0x0040, 0x0060,
		0x0000, 0x0020
	};

	int tam_base = sizeof(base) / sizeof(base[0]);
	int n = 512;

	for(i = 0; i < n; i++){
		trace[i] = base[i % tam_base];
	}

	return n;
}

int benchmark_matriz_linha(int trace[]){
	int i, j;
	int n = 0;
	int dim = 32;
	int base = 0x0000;

	for(i = 0; i < dim; i++){
		for(j = 0; j < dim; j++){
			trace[n] = base + ((i * dim + j) * 4);
			n++;
		}
	}

	return n;
}

int benchmark_matriz_coluna(int trace[]){
	int i, j;
	int n = 0;
	int dim = 32;
	int base = 0x0000;

	for(j = 0; j < dim; j++){
		for(i = 0; i < dim; i++){
			trace[n] = base + ((i * dim + j) * 4);
			n++;
		}
	}

	return n;
}

int benchmark_misto_maior(int trace[]){
	int i;
	int base[] = {
		0x0000, 0x0020, 0x0040, 0x0060,
		0x0000, 0x0020,
		0x0800, 0x1000,
		0x0000, 0x0020,
		0x1800, 0x2000,
		0x0040, 0x0060,
		0x3000, 0x4000,
		0x0000, 0x0020
	};

	int tam_base = sizeof(base) / sizeof(base[0]);
	int n = 512;

	for(i = 0; i < n; i++){
		trace[i] = base[i % tam_base];
	}

	return n;
}

/* =========================================================
   Novo benchmark: Linked List simulada
   ========================================================= */

int benchmark_linked_list_simulada(int trace[]){
	int i;
	int n = 512;

	int lista[] = {
		0x0000, 0x3000, 0x0800, 0x5000,
		0x1000, 0x7000, 0x1800, 0x9000,
		0x2000, 0xB000, 0x2800, 0xD000,
		0x4000, 0xE000, 0x4800, 0xF000
	};

	int tam_lista = sizeof(lista) / sizeof(lista[0]);

	for(i = 0; i < n; i++){
		trace[i] = lista[i % tam_lista];
	}

	return n;
}

/* =========================================================
   Novo benchmark: Pattern Search
   ========================================================= */

int benchmark_pattern_search(int trace[]){
	int i, j;
	int n = 0;

	int texto_base = 0x0000;
	int padrao_base = 0x8000;

	int tamanho_texto = 256;
	int tamanho_padrao = 4;

	for(i = 0; i < tamanho_texto; i++){
		for(j = 0; j < tamanho_padrao; j++){
			trace[n] = texto_base + ((i + j) * 4);
			n++;

			trace[n] = padrao_base + (j * 4);
			n++;
		}
	}

	return n;
}

/* =========================================================
   Execucao e impressao
   ========================================================= */

void executar_benchmark(const char *nome, int trace[], int n, FILE *csv){
	ResultadoHierarquia r_lru;
	ResultadoHierarquia r_ql;

	double lru_l1_hr;
	double ql_l1_hr;
	double ganho_l1;

	r_lru = simular_lru_lru(trace, n);

	srand(42);
	r_ql = simular_ql_lru(trace, n);

	lru_l1_hr = hit_rate(r_lru.l1_hits, r_lru.l1_misses);
	ql_l1_hr = hit_rate(r_ql.l1_hits, r_ql.l1_misses);
	ganho_l1 = ql_l1_hr - lru_l1_hr;

	printf("\n========================================\n");
	printf("Benchmark: %s | Acessos: %d\n", nome, n);

	printf("\nL1 LRU + L2 LRU\n");
	printf("L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_lru.l1_hits, r_lru.l1_misses, lru_l1_hr);
	printf("L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_lru.l2_hits, r_lru.l2_misses, hit_rate(r_lru.l2_hits, r_lru.l2_misses));
	printf("Memoria principal: %d\n", r_lru.l2_misses);

	printf("\nL1 Q-Learning Fixed + L2 LRU\n");
	printf("L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_ql.l1_hits, r_ql.l1_misses, ql_l1_hr);
	printf("L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_ql.l2_hits, r_ql.l2_misses, hit_rate(r_ql.l2_hits, r_ql.l2_misses));
	printf("Memoria principal: %d\n", r_ql.l2_misses);

	printf("\nGanho L1 QL - LRU: %.2f pontos percentuais\n", ganho_l1);

	if(csv != NULL){
		fprintf(csv,
			"%s,%d,%d,%d,%.2f,%d,%d,%.2f,%d,%d,%d,%.2f,%d,%d,%.2f,%d,%.2f\n",
			nome,
			n,
			r_lru.l1_hits,
			r_lru.l1_misses,
			lru_l1_hr,
			r_lru.l2_hits,
			r_lru.l2_misses,
			hit_rate(r_lru.l2_hits, r_lru.l2_misses),
			r_lru.l2_misses,
			r_ql.l1_hits,
			r_ql.l1_misses,
			ql_l1_hr,
			r_ql.l2_hits,
			r_ql.l2_misses,
			hit_rate(r_ql.l2_hits, r_ql.l2_misses),
			r_ql.l2_misses,
			ganho_l1
		);
	}
}

int main(){
	FILE *csv;
	int trace[MAX_TRACE];
	int n;

	csv = fopen("results/benchmarks_hierarquia.csv", "w");

	if(csv == NULL){
		printf("Aviso: nao foi possivel criar results/benchmarks_hierarquia.csv\n");
		printf("Os resultados serao exibidos apenas no terminal.\n");
	}else{
		fprintf(csv,
			"benchmark,acessos,lru_l1_hits,lru_l1_misses,lru_l1_hitrate,lru_l2_hits,lru_l2_misses,lru_l2_hitrate,lru_memoria,ql_l1_hits,ql_l1_misses,ql_l1_hitrate,ql_l2_hits,ql_l2_misses,ql_l2_hitrate,ql_memoria,ganho_l1_pp\n"
		);
	}

	printf("=== BENCHMARKS HIERARQUIA L1 + L2 ===\n");

	printf("\nConfiguracao L1: %d bytes | bloco %d | %d vias\n",
		L1_SIZE, L1_BLOCK, L1_WAYS);

	printf("Configuracao L2: %d bytes | bloco %d | %d vias\n",
		L2_SIZE, L2_BLOCK, L2_WAYS);

	n = benchmark_streaming(trace);
	executar_benchmark("Streaming sequencial", trace, n, csv);

	n = benchmark_hotset_pequeno(trace);
	executar_benchmark("Hotset pequeno", trace, n, csv);

	n = benchmark_conflito_l1(trace);
	executar_benchmark("Conflito forte L1", trace, n, csv);

	n = benchmark_reuso_interferencia(trace);
	executar_benchmark("Reuso com interferencia", trace, n, csv);

	n = benchmark_matriz_linha(trace);
	executar_benchmark("Matriz varredura por linha", trace, n, csv);

	n = benchmark_matriz_coluna(trace);
	executar_benchmark("Matriz varredura por coluna", trace, n, csv);

	n = benchmark_misto_maior(trace);
	executar_benchmark("Misto maior", trace, n, csv);

	n = benchmark_linked_list_simulada(trace);
	executar_benchmark("Linked List simulada", trace, n, csv);

	n = benchmark_pattern_search(trace);
	executar_benchmark("Pattern Search", trace, n, csv);

	if(csv != NULL){
		fclose(csv);
		printf("\nResultados salvos em: results/benchmarks_hierarquia.csv\n");
	}

	return 0;
}