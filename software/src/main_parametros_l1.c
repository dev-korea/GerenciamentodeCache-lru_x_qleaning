#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "sim_types.h"
#include "cache.h"
#include "lru.h"
#include "qlearning_fixed.h"

typedef int (*GeradorTrace)(int trace[]);

typedef struct{
	const char *nome;
	GeradorTrace gerar;
} Benchmark;

/* =========================================================
   Funcoes auxiliares
   ========================================================= */

double calcula_hit_rate(int hits, int misses){
	int total;

	total = hits + misses;

	if(total > 0){
		return ((double)hits / total) * 100.0;
	}

	return 0.0;
}

Resultado run_lru_l1(int acessos[], int n){
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
	r.hit_rate = calcula_hit_rate(cache.hits, cache.misses);

	return r;
}

Resultado run_ql_fixed_l1(int acessos[], int n, int alpha, int gamma, int epsilon){
	Cache cache;
	QLearningFixed ql;
	Resultado r;

	int i;
	int set_idx, tag;
	int hit_way, empty_way, victim_way;

	init_cache(&cache);
	init_qlearning_fixed(&ql, alpha, gamma, epsilon);

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
	r.hit_rate = calcula_hit_rate(cache.hits, cache.misses);

	return r;
}

/* =========================================================
   Geradores de traces / aplicacoes
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

/* =========================================================
   Execucao das varreduras de parametros
   ========================================================= */

void salvar_linha_csv(FILE *csv, const char *benchmark, const char *tipo, int alpha, int gamma, int epsilon, Resultado lru, Resultado ql){
	double ganho;

	ganho = ql.hit_rate - lru.hit_rate;

	if(csv != NULL){
		fprintf(csv,
			"%s,%s,%d,%.3f,%d,%.3f,%d,%d,%d,%.2f,%d,%d,%.2f,%.2f\n",
			benchmark,
			tipo,
			alpha,
			alpha / 1000.0,
			gamma,
			gamma / 1000.0,
			epsilon,
			lru.hits,
			lru.misses,
			lru.hit_rate,
			ql.hits,
			ql.misses,
			ql.hit_rate,
			ganho
		);
	}
}

void imprimir_resultado_parametro(const char *tipo, int alpha, int gamma, int epsilon, Resultado lru, Resultado ql){
	double ganho;

	ganho = ql.hit_rate - lru.hit_rate;

	printf("%-8s | alpha=%4d (%.3f) | gamma=%4d (%.3f) | epsilon=%2d | LRU=%.2f%% | QL=%.2f%% | ganho=%.2f p.p.\n",
		tipo,
		alpha,
		alpha / 1000.0,
		gamma,
		gamma / 1000.0,
		epsilon,
		lru.hit_rate,
		ql.hit_rate,
		ganho
	);
}

void executar_analise_benchmark(const char *nome, int trace[], int n, FILE *csv){
	Resultado r_lru;
	Resultado r_ql;

	int i;

	int alpha_base = 100;
	int gamma_base = 900;
	int epsilon_base = 10;

	int gammas[] = {0, 300, 500, 700, 900};
	int alphas[] = {50, 100, 200, 500};
	int epsilons[] = {0, 5, 10, 20};

	int n_gammas = sizeof(gammas) / sizeof(gammas[0]);
	int n_alphas = sizeof(alphas) / sizeof(alphas[0]);
	int n_epsilons = sizeof(epsilons) / sizeof(epsilons[0]);

	r_lru = run_lru_l1(trace, n);

	printf("\n========================================\n");
	printf("Benchmark: %s | Acessos: %d\n", nome, n);
	printf("Baseline LRU: Hits=%d | Misses=%d | HitRate=%.2f%%\n\n",
		r_lru.hits,
		r_lru.misses,
		r_lru.hit_rate
	);

	printf("---- Variando GAMMA: quanto o algoritmo olha para o futuro ----\n");
	for(i = 0; i < n_gammas; i++){
		srand(42);
		r_ql = run_ql_fixed_l1(trace, n, alpha_base, gammas[i], epsilon_base);

		imprimir_resultado_parametro("GAMMA", alpha_base, gammas[i], epsilon_base, r_lru, r_ql);
		salvar_linha_csv(csv, nome, "gamma", alpha_base, gammas[i], epsilon_base, r_lru, r_ql);
	}

	printf("\n---- Variando ALPHA: velocidade de aprendizado ----\n");
	for(i = 0; i < n_alphas; i++){
		srand(42);
		r_ql = run_ql_fixed_l1(trace, n, alphas[i], gamma_base, epsilon_base);

		imprimir_resultado_parametro("ALPHA", alphas[i], gamma_base, epsilon_base, r_lru, r_ql);
		salvar_linha_csv(csv, nome, "alpha", alphas[i], gamma_base, epsilon_base, r_lru, r_ql);
	}

	printf("\n---- Variando EPSILON: exploracao aleatoria ----\n");
	for(i = 0; i < n_epsilons; i++){
		srand(42);
		r_ql = run_ql_fixed_l1(trace, n, alpha_base, gamma_base, epsilons[i]);

		imprimir_resultado_parametro("EPSILON", alpha_base, gamma_base, epsilons[i], r_lru, r_ql);
		salvar_linha_csv(csv, nome, "epsilon", alpha_base, gamma_base, epsilons[i], r_lru, r_ql);
	}
}

/* =========================================================
   Main
   ========================================================= */

int main(){
	FILE *csv;
	int trace[MAX_TRACE];
	int n;
	int i;

	Benchmark benchmarks[] = {
		{"Streaming sequencial", benchmark_streaming},
		{"Hotset pequeno", benchmark_hotset_pequeno},
		{"Conflito forte L1", benchmark_conflito_l1},
		{"Reuso com interferencia", benchmark_reuso_interferencia},
		{"Misto maior", benchmark_misto_maior},
		{"Linked List simulada", benchmark_linked_list_simulada}
	};

	int total_benchmarks = sizeof(benchmarks) / sizeof(benchmarks[0]);

	csv = fopen("results/analise_parametros_l1.csv", "w");

	if(csv == NULL){
		printf("Aviso: nao foi possivel criar results/analise_parametros_l1.csv\n");
		printf("Os resultados serao exibidos apenas no terminal.\n");
	}else{
		fprintf(csv,
			"benchmark,tipo_varredura,alpha_int,alpha,gamma_int,gamma,epsilon,lru_hits,lru_misses,lru_hitrate,ql_hits,ql_misses,ql_hitrate,ganho_pp\n"
		);
	}

	printf("=== ANALISE DE PARAMETROS DO Q-LEARNING FIXED NA L1 ===\n");
	printf("\nConfiguracao L1:\n");
	printf("Tamanho: 4 KB | Bloco: 32 bytes | Vias: 2 | Conjuntos: 64\n");

	printf("\nParametros base:\n");
	printf("alpha = 100  -> 0.100\n");
	printf("gamma = 900  -> 0.900\n");
	printf("epsilon = 10 -> 10%%\n");

	for(i = 0; i < total_benchmarks; i++){
		n = benchmarks[i].gerar(trace);
		executar_analise_benchmark(benchmarks[i].nome, trace, n, csv);
	}

	if(csv != NULL){
		fclose(csv);
		printf("\nResultados salvos em: results/analise_parametros_l1.csv\n");
	}

	return 0;
}