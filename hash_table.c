#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

// --- Constantes Internas ---
#define FNV_OFFSET_BASIS 2166136261UL
#define FNV_PRIME 16777619UL

// --- Funções Privadas ---

/*
 * Função de Hash (djb2)
 */
static size_t hash_djb2(const char* str, size_t size) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash % size;
}

/*
 * Cria um novo nó de cache (função auxiliar).
 */
static CacheNode* create_node(const char* url) {
    CacheNode* node = (CacheNode*)malloc(sizeof(CacheNode));
    if (!node) {
        perror("Erro ao alocar CacheNode");
        exit(EXIT_FAILURE);
    }
    
    // Aloca e copia a URL
    node->url = (char*)malloc(strlen(url) + 1);
    if (!node->url) {
        perror("Erro ao alocar string da URL");
        free(node);
        exit(EXIT_FAILURE);
    }
    strcpy(node->url, url);
    
    node->hit_count = 0; // Inicializa o contador
    node->next = NULL;
    return node;
}

// --- Implementação da API Pública ---

HashTable* ht_create(size_t size) {
    if (size < 1) {
        fprintf(stderr, "Tamanho da tabela deve ser ao menos 1\n");
        return NULL;
    }
    
    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) {
        perror("Erro ao alocar HashTable");
        return NULL;
    }
    
    ht->table = (CacheNode**)calloc(size, sizeof(CacheNode*));
    if (!ht->table) {
        perror("Erro ao alocar buckets da tabela");
        free(ht);
        return NULL;
    }
    
    ht->size = size;
    return ht;
}

void ht_destroy(HashTable* ht) {
    if (!ht) return;
    
    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* current = ht->table[i];
        while (current) {
            CacheNode* next = current->next;
            free(current->url);
            free(current);
            current = next;
        }
    }
    
    free(ht->table);
    free(ht);
}

void ht_put(HashTable* ht, const char* url) {
    if (!ht || !url) return;
    
    size_t index = hash_djb2(url, ht->size);
    
    CacheNode* current = ht->table[index];
    while (current) {
        if (strcmp(current->url, url) == 0) {
            return; 
        }
        current = current->next;
    }
    
    CacheNode* new_node = create_node(url);
    new_node->next = ht->table[index];
    ht->table[index] = new_node;
}

CacheNode* ht_get(HashTable* ht, const char* url) {
    if (!ht || !url) return NULL;
    
    size_t index = hash_djb2(url, ht->size);
    
    CacheNode* current = ht->table[index];
    while (current) {
        if (strcmp(current->url, url) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// --- MODIFICAÇÃO PARA O GABARITO ---

/* * Função auxiliar de comparação para o qsort.
 * Ordena alfabeticamente pela URL.
 */
int compare_nodes(const void* a, const void* b) {
    CacheNode* nodeA = *(CacheNode**)a;
    CacheNode* nodeB = *(CacheNode**)b;
    return strcmp(nodeA->url, nodeB->url);
}

/*
 * Salva os resultados em ordem ALFABÉTICA para bater com o gabarito.
 */
void ht_save_results(HashTable* ht, const char* filename) {
    if (!ht || !filename) {
        fprintf(stderr, "Erro: Tabela ou nome de arquivo nulo ao salvar resultados.\n");
        return;
    }

    // 1. Conta o total de nós para alocar vetor temporário
    size_t total_nodes = 0;
    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* current = ht->table[i];
        while (current) {
            total_nodes++;
            current = current->next;
        }
    }

    // 2. Coleta todos os ponteiros
    CacheNode** all_nodes = malloc(total_nodes * sizeof(CacheNode*));
    if (!all_nodes) {
        perror("Erro ao alocar vetor de ordenação");
        return;
    }

    size_t k = 0;
    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* current = ht->table[i];
        while (current) {
            all_nodes[k++] = current;
            current = current->next;
        }
    }

    // 3. Ordena (Essencial para o diff funcionar!)
    qsort(all_nodes, total_nodes, sizeof(CacheNode*), compare_nodes);

    // 4. Salva no arquivo
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Erro ao abrir arquivo de resultados");
        free(all_nodes);
        return;
    }

    for (size_t i = 0; i < total_nodes; i++) {
        fprintf(fp, "%s,%ld\n", all_nodes[i]->url, all_nodes[i]->hit_count);
    }

    fclose(fp);
    free(all_nodes);
}

void ht_print(HashTable* ht) {
    // (Mantido igual, omitido para brevidade se não for usado)
}