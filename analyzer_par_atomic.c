#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "hash_table.h"

#define MAX_LINE_LEN 1024
#define HASH_SIZE 20000

void extract_url(char* line, char* url_buffer) {
    char* start = strstr(line, "GET ");
    if (start) {
        start += 4;
        char* end = strchr(start, ' ');
        if (end) {
            int len = end - start;
            strncpy(url_buffer, start, len);
            url_buffer[len] = '\0';
            return;
        }
    }
    url_buffer[0] = '\0';
}

int main(int argc, char *argv[]) {
    // 1. Setup (Idêntico)
    HashTable* ht = ht_create(HASH_SIZE);
    
    FILE* f_manifest = fopen("manifest.txt", "r");
    char buffer[MAX_LINE_LEN];
    while (fgets(buffer, MAX_LINE_LEN, f_manifest)) {
        buffer[strcspn(buffer, "\n")] = 0;
        if(strlen(buffer) > 0) ht_put(ht, buffer);
    }
    fclose(f_manifest);

    FILE* f_log = fopen("access_log.txt", "r");
    long capacity = 10000000;
    char** log_urls = malloc(capacity * sizeof(char*));
    long total_lines = 0;
    char line_buffer[MAX_LINE_LEN];
    char url_buffer[MAX_LINE_LEN];

    while (fgets(line_buffer, MAX_LINE_LEN, f_log)) {
        if (total_lines >= capacity) break;
        extract_url(line_buffer, url_buffer);
        if (strlen(url_buffer) > 0) {
            log_urls[total_lines] = strdup(url_buffer);
            total_lines++;
        }
    }
    fclose(f_log);

    // 2. Processamento Paralelo (ATOMIC)
    printf(">> Iniciando processamento PARALELO (ATOMIC) com %d threads...\n", omp_get_max_threads());
    
    double start_time = omp_get_wtime();

    #pragma omp parallel for
    for (long i = 0; i < total_lines; i++) {
        // A busca na tabela é thread-safe pois a tabela é somente leitura nesta fase.
        // Várias threads podem chamar ht_get ao mesmo tempo.
        CacheNode* node = ht_get(ht, log_urls[i]);
        
        if (node) {
            // A atualização do contador precisa ser atômica para evitar condições de corrida
            // Mas bloqueia apenas esta linha de instrução, não o bloco todo.
            #pragma omp atomic update
            node->hit_count++;
        }
    }

    double end_time = omp_get_wtime();
    printf(">> Tempo Atomic: %.4f segundos\n", end_time - start_time);

    ht_save_results(ht, "results.csv");
    
    for (long i = 0; i < total_lines; i++) free(log_urls[i]);
    free(log_urls);
    ht_destroy(ht);

    return 0;
}