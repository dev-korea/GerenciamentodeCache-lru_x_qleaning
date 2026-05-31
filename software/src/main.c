#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "sim_types.h"
#include "sim_debug.h"
#include "simulator.h"
#include "qlearning_fixed_dyn.h"

/* =========================================================
   Utilitario de exibicao local (apenas debug de cache unica)
   ========================================================= */

static void print_cache_set(Cache *cache, int set_idx){
	int i;

	printf("Set %d -> ", set_idx);

	for(i = 0; i < WAYS; i++){
		if(cache->conjuntos[set_idx].linhas[i].valid){
			printf("[via %d: tag=%d] ", i, cache->conjuntos[set_idx].linhas[i].tag);
		}else{
			printf("[via %d: vazio] ", i);
		}
	}

	printf("\n");
}

/* =========================================================
   Impressao comparativa - cache unica
   ========================================================= */

void print_resultados(const char *nome_trace, Resultado lru, Resultado qlf, Resultado qlx){
	printf("\n===== TRACE: %s =====\n", nome_trace);
	printf("LRU                -> Hits: %d | Misses: %d | HitRate: %.2f%%\n", lru.hits, lru.misses, lru.hit_rate);
	printf("Q-Learning Float   -> Hits: %d | Misses: %d | HitRate: %.2f%%\n", qlf.hits, qlf.misses, qlf.hit_rate);
	printf("Q-Learning Fixed   -> Hits: %d | Misses: %d | HitRate: %.2f%%\n", qlx.hits, qlx.misses, qlx.hit_rate);
}

/* =========================================================
   Opcao 1: comparacao cache unica
   ========================================================= */

void run_compare_all(){
	int trace_localidade[] = {
		0x0000, 0x0800, 0x0000, 0x0800,
		0x0000, 0x0800, 0x0000, 0x0800
	};

	int trace_conflito[] = {
		0x0000, 0x0800, 0x1000,
		0x0000, 0x0800, 0x1000,
		0x0000, 0x0800, 0x1000
	};

	int trace_misto[] = {
		0x0000, 0x0800, 0x0000, 0x0800,
		0x1000, 0x0000, 0x0800, 0x1000,
		0x0000, 0x0800, 0x0000, 0x1000
	};

	int n1 = sizeof(trace_localidade) / sizeof(trace_localidade[0]);
	int n2 = sizeof(trace_conflito) / sizeof(trace_conflito[0]);
	int n3 = sizeof(trace_misto) / sizeof(trace_misto[0]);

	Resultado r_lru, r_qlf, r_qlx;

	printf("\n=== COMPARACAO LRU x QL-FLOAT x QL-FIXED ===\n");

	r_lru = run_lru(trace_localidade, n1);

	srand(42);
	r_qlf = run_qlearning_float(trace_localidade, n1);

	srand(42);
	r_qlx = run_qlearning_fixed(trace_localidade, n1);

	print_resultados("Boa Localidade", r_lru, r_qlf, r_qlx);

	r_lru = run_lru(trace_conflito, n2);

	srand(42);
	r_qlf = run_qlearning_float(trace_conflito, n2);

	srand(42);
	r_qlx = run_qlearning_fixed(trace_conflito, n2);

	print_resultados("Conflito Forte", r_lru, r_qlf, r_qlx);

	r_lru = run_lru(trace_misto, n3);

	srand(42);
	r_qlf = run_qlearning_float(trace_misto, n3);

	srand(42);
	r_qlx = run_qlearning_fixed(trace_misto, n3);

	print_resultados("Misto", r_lru, r_qlf, r_qlx);
}

/* =========================================================
   Opcao 2: debug comparativo float x fixed
   ========================================================= */

void run_debug_misto_float_fixed(){
	int trace_misto[] = {
		0x0000, 0x0800, 0x0000, 0x0800,
		0x1000, 0x0000, 0x0800, 0x1000,
		0x0000, 0x0800, 0x0000, 0x1000
	};

	int n = sizeof(trace_misto) / sizeof(trace_misto[0]);

	Cache cache_float, cache_fixed;
	QLearning qlf;
	QLearningFixed qlx;

	int i;
	int addr, set_idx, tag, rtag;
	int hit_way_f, empty_way_f, victim_way_f;
	int hit_way_x, empty_way_x, victim_way_x;

	srand(42);

	init_cache(&cache_float);
	init_cache(&cache_fixed);
	init_qlearning(&qlf, 0.1f, 0.9f, 0.1f);
	init_qlearning_fixed(&qlx, 100, 900, 10);

	printf("\n=== DEBUG TRACE MISTO: FLOAT x FIXED ===\n");

	for(i = 0; i < n; i++){
		addr = trace_misto[i];
		split_address(addr, &set_idx, &tag);
		rtag = tag % REDUCED_TAGS;

		printf("\n----------------------------------------\n");
		printf("Passo %d | Endereco: 0x%04X | set=%d | tag=%d | rtag=%d\n",
			i, addr, set_idx, tag, rtag);

		printf("\n[FLOAT]\n");
		print_cache_set(&cache_float, set_idx);

		hit_way_f = find_hit(&cache_float, set_idx, tag);

		if(hit_way_f != -1){
			cache_float.hits++;
			printf("Resultado: HIT na via %d\n", hit_way_f);
			ql_on_hit(&qlf, set_idx, tag, hit_way_f);
		}else{
			cache_float.misses++;
			printf("Resultado: MISS\n");

			empty_way_f = find_empty_line(&cache_float, set_idx);

			if(empty_way_f != -1){
				fill_line(&cache_float, set_idx, empty_way_f, tag);
				printf("Acao: preencheu via vazia %d\n", empty_way_f);
				ql_on_miss_fill(&qlf, set_idx, tag, empty_way_f);
			}else{
				printf("Q-values float antes da escolha: [%.3f, %.3f]\n",
					qlf.q_table[set_idx][rtag][0],
					qlf.q_table[set_idx][rtag][1]);

				victim_way_f = ql_choose_action(&qlf, set_idx, tag);
				fill_line(&cache_float, set_idx, victim_way_f, tag);
				printf("Acao: substituiu via %d\n", victim_way_f);
				ql_on_miss_fill(&qlf, set_idx, tag, victim_way_f);
			}
		}

		printf("Q-values float depois: [%.3f, %.3f]\n",
			qlf.q_table[set_idx][rtag][0],
			qlf.q_table[set_idx][rtag][1]);

		print_cache_set(&cache_float, set_idx);

		printf("\n[FIXED]\n");
		print_cache_set(&cache_fixed, set_idx);

		hit_way_x = find_hit(&cache_fixed, set_idx, tag);

		if(hit_way_x != -1){
			cache_fixed.hits++;
			printf("Resultado: HIT na via %d\n", hit_way_x);
			ql_fixed_on_hit(&qlx, set_idx, tag, hit_way_x);
		}else{
			cache_fixed.misses++;
			printf("Resultado: MISS\n");

			empty_way_x = find_empty_line(&cache_fixed, set_idx);

			if(empty_way_x != -1){
				fill_line(&cache_fixed, set_idx, empty_way_x, tag);
				printf("Acao: preencheu via vazia %d\n", empty_way_x);
				ql_fixed_on_miss_fill(&qlx, set_idx, tag, empty_way_x);
			}else{
				printf("Q-values fixed antes da escolha: [%d, %d]\n",
					qlx.q_table[set_idx][rtag][0],
					qlx.q_table[set_idx][rtag][1]);

				victim_way_x = ql_fixed_choose_action(&qlx, set_idx, tag);
				fill_line(&cache_fixed, set_idx, victim_way_x, tag);
				printf("Acao: substituiu via %d\n", victim_way_x);
				ql_fixed_on_miss_fill(&qlx, set_idx, tag, victim_way_x);
			}
		}

		printf("Q-values fixed depois: [%d, %d]\n",
			qlx.q_table[set_idx][rtag][0],
			qlx.q_table[set_idx][rtag][1]);

		print_cache_set(&cache_fixed, set_idx);
	}

	printf("\n========================================\n");
	printf("RESUMO FINAL DEBUG\n");

	printf("Float -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		cache_float.hits,
		cache_float.misses,
		(cache_float.hits + cache_float.misses) ? ((double)cache_float.hits / (cache_float.hits + cache_float.misses)) * 100.0 : 0.0);

	printf("Fixed -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		cache_fixed.hits,
		cache_fixed.misses,
		(cache_fixed.hits + cache_fixed.misses) ? ((double)cache_fixed.hits / (cache_fixed.hits + cache_fixed.misses)) * 100.0 : 0.0);
}

/* =========================================================
   Opcao 3: relatorio de custo da Q-table
   ========================================================= */

void relatorio_custo_qtable(){
	FILE *fp;
	int bits_opcoes[] = {8, 12, 16, 24, 32};
	int n_opcoes = sizeof(bits_opcoes) / sizeof(bits_opcoes[0]);

	int i;
	int bits_q;

	long long total_entradas;
	long long total_bits;

	double total_bytes;
	double total_kb;

	total_entradas = (long long)N_SETS * REDUCED_TAGS * WAYS;

	fp = fopen("results/relatorio_qtable.txt", "w");

	if(fp == NULL){
		printf("\nErro ao criar results/relatorio_qtable.txt\n");
		printf("Verifique se a pasta results existe.\n");
		return;
	}

	printf("\n===== RELATORIO DE CUSTO DA Q-TABLE =====\n");
	fprintf(fp, "===== RELATORIO DE CUSTO DA Q-TABLE =====\n");

	printf("\nConfiguracao atual:\n");
	printf("N_SETS       : %d\n", N_SETS);
	printf("REDUCED_TAGS : %d\n", REDUCED_TAGS);
	printf("WAYS         : %d\n", WAYS);
	printf("Entradas Q   : %lld\n", total_entradas);

	fprintf(fp, "\nConfiguracao atual:\n");
	fprintf(fp, "N_SETS       : %d\n", N_SETS);
	fprintf(fp, "REDUCED_TAGS : %d\n", REDUCED_TAGS);
	fprintf(fp, "WAYS         : %d\n", WAYS);
	fprintf(fp, "Entradas Q   : %lld\n", total_entradas);

	printf("\nBits por Q-value | Total bits | Total bytes | Total KB\n");
	printf("------------------------------------------------------\n");

	fprintf(fp, "\nBits por Q-value | Total bits | Total bytes | Total KB\n");
	fprintf(fp, "------------------------------------------------------\n");

	for(i = 0; i < n_opcoes; i++){
		bits_q = bits_opcoes[i];

		total_bits = total_entradas * bits_q;
		total_bytes = total_bits / 8.0;
		total_kb = total_bytes / 1024.0;

		printf("%16d | %10lld | %11.2f | %8.2f KB\n",
			bits_q, total_bits, total_bytes, total_kb);

		fprintf(fp, "%16d | %10lld | %11.2f | %8.2f KB\n",
			bits_q, total_bits, total_bytes, total_kb);
	}

	printf("\nAnalise:\n");
	printf("- Aumentar os bits aumenta a precisao dos valores Q.\n");
	printf("- Mais bits reduzem perdas por quantizacao no fixed-point.\n");
	printf("- Porem, mais bits aumentam area ocupada no FPGA.\n");
	printf("- Comparadores, somadores e multiplicadores tambem ficam maiores.\n");
	printf("- Isso pode aumentar a latencia ou reduzir a frequencia maxima.\n");
	printf("- Para a etapa atual, 16 bits signed e uma escolha equilibrada.\n");

	fprintf(fp, "\nAnalise:\n");
	fprintf(fp, "- Aumentar os bits aumenta a precisao dos valores Q.\n");
	fprintf(fp, "- Mais bits reduzem perdas por quantizacao no fixed-point.\n");
	fprintf(fp, "- Porem, mais bits aumentam area ocupada no FPGA.\n");
	fprintf(fp, "- Comparadores, somadores e multiplicadores tambem ficam maiores.\n");
	fprintf(fp, "- Isso pode aumentar a latencia ou reduzir a frequencia maxima.\n");
	fprintf(fp, "- Para a etapa atual, 16 bits signed e uma escolha equilibrada.\n");

	printf("\nParametros fixed-point atuais:\n");
	printf("SCALE       : %d\n", QL_SCALE);
	printf("alpha       : 100  equivale a 0.100\n");
	printf("gamma       : 900  equivale a 0.900\n");
	printf("reward hit  : +1000\n");
	printf("reward miss : -1000\n");

	fprintf(fp, "\nParametros fixed-point atuais:\n");
	fprintf(fp, "SCALE       : %d\n", QL_SCALE);
	fprintf(fp, "alpha       : 100  equivale a 0.100\n");
	fprintf(fp, "gamma       : 900  equivale a 0.900\n");
	fprintf(fp, "reward hit  : +1000\n");
	fprintf(fp, "reward miss : -1000\n");

	fclose(fp);

	printf("\nRelatorio salvo em: results/relatorio_qtable.txt\n");
}

/* =========================================================
   Opcao 4: validacao automatica
   ========================================================= */

void print_validacao(const char *nome_teste, int passou){
	if(passou){
		printf("[OK]   %s\n", nome_teste);
	}else{
		printf("[ERRO] %s\n", nome_teste);
	}
}

void validacao_automatica(){
	int passou;
	int total_testes = 0;
	int testes_ok = 0;

	Resultado r_lru;
	Resultado r_qlf;
	Resultado r_qlx;

	QLearningFixed ql_teste;

	int trace_cache_simples[] = {
		0x0000, 0x0800, 0x0000, 0x0800
	};

	int trace_conflito[] = {
		0x0000, 0x0800, 0x1000,
		0x0000, 0x0800, 0x1000
	};

	int trace_misto[] = {
		0x0000, 0x0800, 0x0000, 0x0800,
		0x1000, 0x0000, 0x0800, 0x1000,
		0x0000, 0x0800, 0x0000, 0x1000
	};

	int n_cache_simples = sizeof(trace_cache_simples) / sizeof(trace_cache_simples[0]);
	int n_conflito = sizeof(trace_conflito) / sizeof(trace_conflito[0]);
	int n_misto = sizeof(trace_misto) / sizeof(trace_misto[0]);

	printf("\n===== VALIDACAO AUTOMATICA DO ALGORITMO =====\n\n");

	r_lru = run_lru(trace_cache_simples, n_cache_simples);

	passou = (r_lru.hits == 2 && r_lru.misses == 2);
	print_validacao("Cache base + LRU em caso simples (esperado: 2 hits e 2 misses)", passou);

	total_testes++;
	if(passou){
		testes_ok++;
	}

	r_lru = run_lru(trace_conflito, n_conflito);

	passou = (r_lru.hits == 0 && r_lru.misses == 6);
	print_validacao("LRU em conflito forte / thrashing (esperado: 0 hits e 6 misses)", passou);

	total_testes++;
	if(passou){
		testes_ok++;
	}

	srand(42);
	r_qlf = run_qlearning_float(trace_misto, n_misto);

	srand(42);
	r_qlx = run_qlearning_fixed(trace_misto, n_misto);

	passou = (r_qlf.hits == r_qlx.hits && r_qlf.misses == r_qlx.misses);
	print_validacao("Equivalencia Q-Learning Float x Fixed no trace misto", passou);

	total_testes++;
	if(passou){
		testes_ok++;
	}

	init_qlearning_fixed(&ql_teste, QL_DEFAULT_ALPHA, QL_DEFAULT_GAMMA, QL_DEFAULT_EPSILON);

	ql_fixed_on_miss_fill(&ql_teste, 0, 0, 0);

	passou = (ql_teste.q_table[0][0][0] == -100);
	print_validacao("Atualizacao fixed apos MISS reduz valor Q", passou);

	total_testes++;
	if(passou){
		testes_ok++;
	}

	ql_fixed_on_hit(&ql_teste, 0, 0, 0);

	passou = (ql_teste.q_table[0][0][0] == 10);
	print_validacao("Atualizacao fixed apos HIT aumenta valor Q", passou);

	total_testes++;
	if(passou){
		testes_ok++;
	}

	printf("\nResumo da validacao: %d/%d testes passaram.\n", testes_ok, total_testes);

	if(testes_ok == total_testes){
		printf("Resultado geral: VALIDACAO CONCLUIDA COM SUCESSO.\n");
	}else{
		printf("Resultado geral: EXISTEM TESTES COM FALHA.\n");
	}
}

/* =========================================================
   Funcoes da hierarquia L1 + L2
   ========================================================= */

double hit_rate_hierarquia(int hits, int misses){
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
		hit_rate_hierarquia(r.l1_hits, r.l1_misses));

	printf("L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r.l2_hits,
		r.l2_misses,
		hit_rate_hierarquia(r.l2_hits, r.l2_misses));

	printf("Acessos a memoria principal: %d\n", r.l2_misses);
}

void salvar_resultado_hierarquia(FILE *fp, const char *nome_trace, ResultadoHierarquia r_lru, ResultadoHierarquia r_ql){
	fprintf(fp, "\n===== TRACE: %s =====\n", nome_trace);

	fprintf(fp, "L1 LRU + L2 LRU\n");
	fprintf(fp, "L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_lru.l1_hits,
		r_lru.l1_misses,
		hit_rate_hierarquia(r_lru.l1_hits, r_lru.l1_misses));

	fprintf(fp, "L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_lru.l2_hits,
		r_lru.l2_misses,
		hit_rate_hierarquia(r_lru.l2_hits, r_lru.l2_misses));

	fprintf(fp, "Acessos a memoria principal: %d\n", r_lru.l2_misses);

	fprintf(fp, "\nL1 Q-Learning Fixed + L2 LRU\n");
	fprintf(fp, "L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_ql.l1_hits,
		r_ql.l1_misses,
		hit_rate_hierarquia(r_ql.l1_hits, r_ql.l1_misses));

	fprintf(fp, "L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_ql.l2_hits,
		r_ql.l2_misses,
		hit_rate_hierarquia(r_ql.l2_hits, r_ql.l2_misses));

	fprintf(fp, "Acessos a memoria principal: %d\n", r_ql.l2_misses);
}

void executar_trace_hierarquia(const char *nome_trace, int acessos[], int n, FILE *fp){
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
		salvar_resultado_hierarquia(fp, nome_trace, r_lru_lru, r_ql_lru);
	}
}

/* =========================================================
   Geradores de traces da hierarquia - opcao 5
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

int gerar_trace_misto_hierarquia(int trace[]){
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
   Opcao 5: avaliacao da hierarquia L1 + L2
   ========================================================= */

void avaliar_hierarquia_l1_l2(){
	FILE *fp;

	int trace[MAX_TRACE];
	int n;

	fp = fopen("results/resultado_hierarquia.txt", "w");

	if(fp == NULL){
		printf("Aviso: nao foi possivel criar results/resultado_hierarquia.txt\n");
		printf("Os resultados serao exibidos apenas no terminal.\n");
	}

	printf("\n=== AVALIACAO HIERARQUIA L1 + L2 ===\n");

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
	executar_trace_hierarquia("Streaming sequencial", trace, n, fp);

	n = gerar_trace_hotset(trace);
	executar_trace_hierarquia("Hotset / boa localidade", trace, n, fp);

	n = gerar_trace_conflito_l1(trace);
	executar_trace_hierarquia("Conflito forte na L1", trace, n, fp);

	n = gerar_trace_misto_hierarquia(trace);
	executar_trace_hierarquia("Misto", trace, n, fp);

	n = gerar_trace_reuso_com_interferencia(trace);
	executar_trace_hierarquia("Reuso com interferencia", trace, n, fp);

	if(fp != NULL){
		fclose(fp);
		printf("\nResultados salvos em: results/resultado_hierarquia.txt\n");
	}
}

/* =========================================================
   Geradores de benchmarks - opcao 6
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
   Opcao 6: benchmarks da hierarquia L1 + L2
   ========================================================= */

void executar_benchmark_menu(const char *nome, int trace[], int n, FILE *csv){
	ResultadoHierarquia r_lru;
	ResultadoHierarquia r_ql;

	double lru_l1_hr;
	double ql_l1_hr;
	double ganho_l1;

	r_lru = simular_lru_lru(trace, n, 0);

	srand(42);
	r_ql = simular_ql_lru(trace, n, 0);

	lru_l1_hr = hit_rate_hierarquia(r_lru.l1_hits, r_lru.l1_misses);
	ql_l1_hr = hit_rate_hierarquia(r_ql.l1_hits, r_ql.l1_misses);
	ganho_l1 = ql_l1_hr - lru_l1_hr;

	printf("\n========================================\n");
	printf("Benchmark: %s | Acessos: %d\n", nome, n);

	printf("\nL1 LRU + L2 LRU\n");
	printf("L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_lru.l1_hits,
		r_lru.l1_misses,
		lru_l1_hr);

	printf("L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_lru.l2_hits,
		r_lru.l2_misses,
		hit_rate_hierarquia(r_lru.l2_hits, r_lru.l2_misses));

	printf("Memoria principal: %d\n", r_lru.l2_misses);

	printf("\nL1 Q-Learning Fixed + L2 LRU\n");
	printf("L1 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_ql.l1_hits,
		r_ql.l1_misses,
		ql_l1_hr);

	printf("L2 -> Hits: %d | Misses: %d | HitRate: %.2f%%\n",
		r_ql.l2_hits,
		r_ql.l2_misses,
		hit_rate_hierarquia(r_ql.l2_hits, r_ql.l2_misses));

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
			hit_rate_hierarquia(r_lru.l2_hits, r_lru.l2_misses),
			r_lru.l2_misses,
			r_ql.l1_hits,
			r_ql.l1_misses,
			ql_l1_hr,
			r_ql.l2_hits,
			r_ql.l2_misses,
			hit_rate_hierarquia(r_ql.l2_hits, r_ql.l2_misses),
			r_ql.l2_misses,
			ganho_l1
		);
	}
}

void executar_benchmarks_hierarquia(){
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

	printf("\n=== BENCHMARKS HIERARQUIA L1 + L2 ===\n");

	printf("\nConfiguracao L1: %d bytes | bloco %d | %d vias\n",
		L1_SIZE, L1_BLOCK, L1_WAYS);

	printf("Configuracao L2: %d bytes | bloco %d | %d vias\n",
		L2_SIZE, L2_BLOCK, L2_WAYS);

	n = benchmark_streaming(trace);
	executar_benchmark_menu("Streaming sequencial", trace, n, csv);

	n = benchmark_hotset_pequeno(trace);
	executar_benchmark_menu("Hotset pequeno", trace, n, csv);

	n = benchmark_conflito_l1(trace);
	executar_benchmark_menu("Conflito forte L1", trace, n, csv);

	n = benchmark_reuso_interferencia(trace);
	executar_benchmark_menu("Reuso com interferencia", trace, n, csv);

	n = benchmark_matriz_linha(trace);
	executar_benchmark_menu("Matriz varredura por linha", trace, n, csv);

	n = benchmark_matriz_coluna(trace);
	executar_benchmark_menu("Matriz varredura por coluna", trace, n, csv);

	n = benchmark_misto_maior(trace);
	executar_benchmark_menu("Misto maior", trace, n, csv);

	n = benchmark_linked_list_simulada(trace);
	executar_benchmark_menu("Linked List simulada", trace, n, csv);

	n = benchmark_pattern_search(trace);
	executar_benchmark_menu("Pattern Search", trace, n, csv);

	if(csv != NULL){
		fclose(csv);
		printf("\nResultados salvos em: results/benchmarks_hierarquia.csv\n");
	}
}

/* =========================================================
   Opcao 7: chama benchmark externo com Q-Learning Fixed na L2
   ========================================================= */

void executar_benchmarks_ql_l2(){
	int retorno;

	printf("\n=== EXECUTANDO BENCHMARKS COM Q-LEARNING FIXED NA L2 ===\n");

	retorno = system(".\\benchmarks_ql_l2.exe");

	if(retorno != 0){
		printf("\nErro ao executar benchmarks_ql_l2.exe\n");
		printf("Compile antes com:\n");
		printf("gcc src/main_benchmarks_ql_l2.c src/cache_dyn.c src/lru_dyn.c src/qlearning_fixed_dyn.c -Iinclude -o benchmarks_ql_l2\n");
	}
}
void executar_validacao_50_conflito(){
	int retorno;

	printf("\n=== EXECUTANDO VALIDACAO CONTROLADA COM 50 ENTRADAS - CONFLITO L1 ===\n");

	retorno = system(".\\validacao_50_conflito.exe trace\\trace_validacao_50_conflito.txt");

	if(retorno != 0){
		printf("\nErro ao executar validacao_50_conflito.exe\n");
		printf("Compile antes com:\n");
		printf("gcc src/main_validacao_50_conflito.c src/cache.c src/lru.c src/qlearning.c src/qlearning_fixed.c src/cache_dyn.c src/lru_dyn.c -Iinclude -o validacao_50_conflito\n");
	}
}

/* =========================================================
   Main com menu
   ========================================================= */

int main(){
	int op;

	do{
		printf("\n");
		printf("+----------------------------------------------------------+\n");
		printf("|                  MENU PRINCIPAL                          |\n");
		printf("+----------------------------------------------------------+\n");
		printf("| 1. Comparar LRU x QL-Float x QL-Fixed                    |\n");
		printf("| 2. Debug comparativo Float x Fixed (trace misto)         |\n");
		printf("| 3. Relatorio de custo da Q-table                         |\n");
		printf("| 4. Validacao automatica do algoritmo                     |\n");
		printf("| 5. Avaliacao hierarquia L1 + L2                          |\n");
		printf("| 6. Benchmarks da hierarquia L1 + L2                      |\n");
		printf("| 7. Benchmarks com Q-Learning Fixed na L2                 |\n");
		printf("| 8. Validacao controlada com 50 entradas - conflito L1    |\n");
		printf("| 0. Sair                                                  |\n");	
		printf("+----------------------------------------------------------+\n");

		printf("Escolha: ");

		if(scanf("%d", &op) != 1){
			printf("Entrada invalida.\n");
			return 1;
		}

		switch(op){
			case 1:
				run_compare_all();
				break;

			case 2:
				run_debug_misto_float_fixed();
				break;

			case 3:
				relatorio_custo_qtable();
				break;

			case 4:
				validacao_automatica();
				break;

			case 5:
				avaliar_hierarquia_l1_l2();
				break;

			case 6:
				executar_benchmarks_hierarquia();
				break;

			case 7:
				executar_benchmarks_ql_l2();
				break;
			case 8:
				executar_validacao_50_conflito();
				break;
			case 0:
				printf("Encerrando...\n");
				break;

			default:
				printf("Opcao invalida.\n");
		}

	}while(op != 0);

	return 0;
}
