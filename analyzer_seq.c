#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h> // Apenas para usar a funcao de tempo omp_get_wtime, se disponivel, ou clock()
#include "hash_table.h"

#define MAX_LINE_LEN 1024
#define HASH_SIZE 20000 // Tamanho sugerido para evitar muitas colisoes iniciais

// Função auxiliar para extrair a URL da linha de log
// Formato esperado: IP ... "GET /url ..."
void extract_url(char* line, char* url_buffer) {
    char* start = strstr(line, "GET ");
    if (start) {
        start += 4; // Pula "GET "
        char* end = strchr(start, ' ');
        if (end) {
            int len = end - start;
            strncpy(url_buffer, start, len);
            url_buffer[len] = '\0';
            return;
        }
    }
    url_buffer[0] = '\0'; // Falha
}

int main(int argc, char *argv[]) {
    // 1. Inicialização e Carga do Manifest
    printf(">> Carregando manifest.txt...\n");
    HashTable* ht = ht_create(HASH_SIZE);
    
    FILE* f_manifest = fopen("manifest.txt", "r");
    if (!f_manifest) { perror("Erro ao abrir manifest.txt"); return 1; }
    
    char buffer[MAX_LINE_LEN];
    while (fgets(buffer, MAX_LINE_LEN, f_manifest)) {
        // Remove quebra de linha
        buffer[strcspn(buffer, "\n")] = 0;
        if(strlen(buffer) > 0) ht_put(ht, buffer);
    }
    fclose(f_manifest);

    // 2. Carregar Log para Memória (Pré-processamento)
    printf(">> Lendo access_log.txt para a RAM...\n");
    FILE* f_log = fopen("access_log.txt", "r");
    if (!f_log) { perror("Erro ao abrir access_log.txt"); return 1; }

    // Estimativa dinamica ou fixa. Vamos usar uma lista grande de ponteiros.
    // O PDF fala em milhões. Vamos alocar espaço para ponteiros.
    // Nota: Em um cenário real, usariamos realloc, aqui fixo um limite seguro ou conto linhas antes.
    // Vou usar uma abordagem de ler, extrair a URL e guardar APENAS a URL na memoria para economizar RAM.
    
    long capacity = 10000000; // Suporta 10 milhoes de linhas
    char** log_urls = malloc(capacity * sizeof(char*));
    long total_lines = 0;
    char line_buffer[MAX_LINE_LEN];
    char url_buffer[MAX_LINE_LEN];

    while (fgets(line_buffer, MAX_LINE_LEN, f_log)) {
        if (total_lines >= capacity) break;
        
        extract_url(line_buffer, url_buffer);
        if (strlen(url_buffer) > 0) {
            log_urls[total_lines] = strdup(url_buffer); // Aloca string exata
            total_lines++;
        }
    }
    fclose(f_log);
    printf(">> Log carregado: %ld linhas válidas.\n", total_lines);

    // 3. Processamento (A Parte Crítica do Teste)
    printf(">> Iniciando processamento SEQUENCIAL...\n");
    
    double start_time = omp_get_wtime(); // Requer flag -fopenmp mesmo no seq para ter precisão

    for (long i = 0; i < total_lines; i++) {
        CacheNode* node = ht_get(ht, log_urls[i]);
        if (node) {
            node->hit_count++; // Sem proteção, pois é sequencial
        }
    }

    double end_time = omp_get_wtime();
    printf(">> Tempo Sequencial: %.4f segundos\n", end_time - start_time);

    // 4. Salvar Resultados e Limpeza
    ht_save_results(ht, "results.csv");
    
    // Limpar memória do log
    for (long i = 0; i < total_lines; i++) free(log_urls[i]);
    free(log_urls);
    ht_destroy(ht);

    return 0;
}