#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "sim_types.h"
#include "cache_dyn.h"
#include "lru_dyn.h"
#include "qlearning_fixed_dyn.h"

double hit_rate(int hits, int misses){
	int total;

	total = hits + misses;

	if(total > 0){
		return ((double)hits / total) * 100.0;
	}

	return 0.0;
}

/* =========================================================
   Insercao LRU
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
   Insercao Q-Learning Fixed Dinamico
   ========================================================= */

void insert_cache_qlfd(CacheDyn *cache, QLearningFixedDyn *ql, int address){
	int set_idx, tag;
	int hit_way, empty_way, victim_way;

	split_address_dyn(cache, address, &set_idx, &tag);

	hit_way = find_hit_dyn(cache, set_idx, tag);

	if(hit_way != -1){
		qlfd_on_hit(ql, set_idx, tag, hit_way);
		return;
	}

	empty_way = find_empty_line_dyn(cache, set_idx);

	if(empty_way != -1){
		fill_line_dyn(cache, set_idx, empty_way, tag);
		qlfd_on_miss_fill(ql, set_idx, tag, empty_way);
	}else{
		victim_way = qlfd_choose_action(ql, set_idx, tag);
		fill_line_dyn(cache, set_idx, victim_way, tag);
		qlfd_on_miss_fill(ql, set_idx, tag, victim_way);
	}
}

/* =========================================================
   Cenario 1: L1 LRU + L2 LRU
   ========================================================= */

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

/* =========================================================
   Cenario 2: L1 QL Fixed + L2 LRU
   ========================================================= */

ResultadoHierarquia simular_ql_lru(int acessos[], int n){
	CacheDyn l1;
	CacheDyn l2;

	QLearningFixedDyn ql_l1;
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

	init_qlearning_fixed_dyn(&ql_l1, L1_WAYS, QL_DEFAULT_ALPHA, QL_DEFAULT_GAMMA, QL_DEFAULT_EPSILON);
	init_lru_dyn(&lru_l2, L2_WAYS);

	for(i = 0; i < n; i++){
		address = acessos[i];

		split_address_dyn(&l1, address, &set_l1, &tag_l1);
		hit_l1 = find_hit_dyn(&l1, set_l1, tag_l1);

		if(hit_l1 != -1){
			r.l1_hits++;
			qlfd_on_hit(&ql_l1, set_l1, tag_l1, hit_l1);
			continue;
		}

		r.l1_misses++;

		split_address_dyn(&l2, address, &set_l2, &tag_l2);
		hit_l2 = find_hit_dyn(&l2, set_l2, tag_l2);

		if(hit_l2 != -1){
			r.l2_hits++;
			lru_dyn_on_hit(&lru_l2, set_l2, hit_l2);
			insert_cache_qlfd(&l1, &ql_l1, address);
			continue;
		}

		r.l2_misses++;

		insert_cache_lru(&l2, &lru_l2, address);
		insert_cache_qlfd(&l1, &ql_l1, address);
	}

	return r;
}

/* =========================================================
   Cenario 3: L1 QL Fixed + L2 QL Fixed
   ========================================================= */

ResultadoHierarquia simular_ql_ql(int acessos[], int n){
	CacheDyn l1;
	CacheDyn l2;

	QLearningFixedDyn ql_l1;
	QLearningFixedDyn ql_l2;

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

	init_qlearning_fixed_dyn(&ql_l1, L1_WAYS, QL_DEFAULT_ALPHA, QL_DEFAULT_GAMMA, QL_DEFAULT_EPSILON);
	init_qlearning_fixed_dyn(&ql_l2, L2_WAYS, QL_DEFAULT_ALPHA, QL_DEFAULT_GAMMA, QL_DEFAULT_EPSILON);

	for(i = 0; i < n; i++){
		address = acessos[i];

		split_address_dyn(&l1, address, &set_l1, &tag_l1);
		hit_l1 = find_hit_dyn(&l1, set_l1, tag_l1);

		if(hit_l1 != -1){
			r.l1_hits++;
			qlfd_on_hit(&ql_l1, set_l1, tag_l1, hit_l1);
			continue;
		}

		r.l1_misses++;

		split_address_dyn(&l2, address, &set_l2, &tag_l2);
		hit_l2 = find_hit_dyn(&l2, set_l2, tag_l2);

		if(hit_l2 != -1){
			r.l2_hits++;
			qlfd_on_hit(&ql_l2, set_l2, tag_l2, hit_l2);
			insert_cache_qlfd(&l1, &ql_l1, address);
			continue;
		}

		r.l2_misses++;

		insert_cache_qlfd(&l2, &ql_l2, address);
		insert_cache_qlfd(&l1, &ql_l1, address);
	}

	return r;
}

/* =========================================================
   Benchmarks
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
   Execucao
   ========================================================= */

void executar_benchmark(const char *nome, int trace[], int n, FILE *csv){
	ResultadoHierarquia r_lru_lru;
	ResultadoHierarquia r_ql_lru;
	ResultadoHierarquia r_ql_ql;

	double hr_lru_l1;
	double hr_ql_lru_l1;
	double hr_ql_ql_l1;

	double ganho_ql_lru;
	double ganho_ql_ql;

	r_lru_lru = simular_lru_lru(trace, n);

	srand(42);
	r_ql_lru = simular_ql_lru(trace, n);

	srand(42);
	r_ql_ql = simular_ql_ql(trace, n);

	hr_lru_l1 = hit_rate(r_lru_lru.l1_hits, r_lru_lru.l1_misses);
	hr_ql_lru_l1 = hit_rate(r_ql_lru.l1_hits, r_ql_lru.l1_misses);
	hr_ql_ql_l1 = hit_rate(r_ql_ql.l1_hits, r_ql_ql.l1_misses);

	ganho_ql_lru = hr_ql_lru_l1 - hr_lru_l1;
	ganho_ql_ql = hr_ql_ql_l1 - hr_lru_l1;

	printf("\n========================================\n");
	printf("Benchmark: %s | Acessos: %d\n", nome, n);

	printf("\n[1] L1 LRU + L2 LRU\n");
	printf("L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_lru_lru.l1_hits, r_lru_lru.l1_misses, hr_lru_l1);
	printf("L2 -> Hits: %d | Misses: %d | HitRate: %.2f%% | Memoria: %d\n",
		r_lru_lru.l2_hits,
		r_lru_lru.l2_misses,
		hit_rate(r_lru_lru.l2_hits, r_lru_lru.l2_misses),
		r_lru_lru.l2_misses);

	printf("\n[2] L1 QL Fixed + L2 LRU\n");
	printf("L1 -> Hits: %d | Misses: %d | HitRate: %.2f%% | Ganho L1: %.2f p.p.\n",
		r_ql_lru.l1_hits, r_ql_lru.l1_misses, hr_ql_lru_l1, ganho_ql_lru);
	printf("L2 -> Hits: %d | Misses: %d | HitRate: %.2f%% | Memoria: %d\n",
		r_ql_lru.l2_hits,
		r_ql_lru.l2_misses,
		hit_rate(r_ql_lru.l2_hits, r_ql_lru.l2_misses),
		r_ql_lru.l2_misses);

	printf("\n[3] L1 QL Fixed + L2 QL Fixed\n");
	printf("L1 -> Hits: %d | Misses: %d | HitRate: %.2f%% | Ganho L1: %.2f p.p.\n",
		r_ql_ql.l1_hits, r_ql_ql.l1_misses, hr_ql_ql_l1, ganho_ql_ql);
	printf("L2 -> Hits: %d | Misses: %d | HitRate: %.2f%% | Memoria: %d\n",
		r_ql_ql.l2_hits,
		r_ql_ql.l2_misses,
		hit_rate(r_ql_ql.l2_hits, r_ql_ql.l2_misses),
		r_ql_ql.l2_misses);

	if(csv != NULL){
		fprintf(csv,
			"%s,%d,"
			"%d,%d,%.2f,%d,%d,%.2f,%d,"
			"%d,%d,%.2f,%d,%d,%.2f,%d,%.2f,"
			"%d,%d,%.2f,%d,%d,%.2f,%d,%.2f\n",
			nome,
			n,

			r_lru_lru.l1_hits,
			r_lru_lru.l1_misses,
			hr_lru_l1,
			r_lru_lru.l2_hits,
			r_lru_lru.l2_misses,
			hit_rate(r_lru_lru.l2_hits, r_lru_lru.l2_misses),
			r_lru_lru.l2_misses,

			r_ql_lru.l1_hits,
			r_ql_lru.l1_misses,
			hr_ql_lru_l1,
			r_ql_lru.l2_hits,
			r_ql_lru.l2_misses,
			hit_rate(r_ql_lru.l2_hits, r_ql_lru.l2_misses),
			r_ql_lru.l2_misses,
			ganho_ql_lru,

			r_ql_ql.l1_hits,
			r_ql_ql.l1_misses,
			hr_ql_ql_l1,
			r_ql_ql.l2_hits,
			r_ql_ql.l2_misses,
			hit_rate(r_ql_ql.l2_hits, r_ql_ql.l2_misses),
			r_ql_ql.l2_misses,
			ganho_ql_ql
		);
	}
}

int main(){
	FILE *csv;
	int trace[MAX_TRACE];
	int n;

	csv = fopen("results/benchmarks_ql_l2.csv", "w");

	if(csv == NULL){
		printf("Aviso: nao foi possivel criar results/benchmarks_ql_l2.csv\n");
		printf("Os resultados serao exibidos apenas no terminal.\n");
	}else{
		fprintf(csv,
			"benchmark,acessos,"
			"lru_l1_hits,lru_l1_misses,lru_l1_hitrate,lru_l2_hits,lru_l2_misses,lru_l2_hitrate,lru_memoria,"
			"ql_lru_l1_hits,ql_lru_l1_misses,ql_lru_l1_hitrate,ql_lru_l2_hits,ql_lru_l2_misses,ql_lru_l2_hitrate,ql_lru_memoria,ql_lru_ganho_l1,"
			"ql_ql_l1_hits,ql_ql_l1_misses,ql_ql_l1_hitrate,ql_ql_l2_hits,ql_ql_l2_misses,ql_ql_l2_hitrate,ql_ql_memoria,ql_ql_ganho_l1\n"
		);
	}

	printf("=== BENCHMARKS COM Q-LEARNING FIXED NA L2 ===\n");

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
		printf("\nResultados salvos em: results/benchmarks_ql_l2.csv\n");
	}

	return 0;
}