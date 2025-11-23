#!/bin/bash

ZIP_FILE="cdn_data_logs.zip"
LOG_DIST="log_distribuido.txt"
LOG_CONC="log_concorrente.txt"
GAB_DIST="gabarito_distribuido.csv"
GAB_CONC="gabarito_concorrente.csv"
LOG_TARGET="access_log.txt"
MANIFEST="manifest.txt"

GREEN='\033[0;32m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m'

# --- NOVA SEÇÃO: DETALHES DA MÁQUINA ---
echo -e "${CYAN}=== 0. AMBIENTE DE EXECUÇÃO (Para o Relatório) ===${NC}"
echo "--------------------------------------------------------"
echo -n "1. SO:            " && cat /etc/os-release | grep "PRETTY_NAME" | cut -d'"' -f2
echo -n "2. Modelo CPU:    " && lscpu | grep "Model name" | sed 's/Model name:[ \t]*//'
echo -n "3. Núcleos (vCPU):" && nproc
echo -n "4. Frequência:    " && lscpu | grep "MHz" | head -n 1 | awk '{print $3 " MHz"}'
echo -n "5. RAM Total:     " && free -h | grep "Mem:" | awk '{print $2}'
echo "--------------------------------------------------------"
echo ""

echo -e "${CYAN}=== 1. PREPARAÇÃO DOS DADOS ===${NC}"

if [ ! -f "generate_cdn_data.py" ]; then
    echo -e "${RED}Erro: generate_cdn_data.py não encontrado.${NC}"
    exit 1
fi

if [ ! -f "$ZIP_FILE" ]; then
    python3 generate_cdn_data.py
fi

unzip -o $ZIP_FILE > /dev/null

echo -e "${CYAN}=== 2. COMPILAÇÃO ===${NC}"
make clean
make

if [ ! -f "./analyzer_seq" ]; then
    echo -e "${RED}Erro na compilação.${NC}"
    exit 1
fi

echo -e "${CYAN}=== 3. TESTE DE ESCALABILIDADE (LOG DISTRIBUÍDO) ===${NC}"
cp $LOG_DIST $LOG_TARGET

echo -e "\n${GREEN}>> Rodando Sequencial (Baseline)...${NC}"
./analyzer_seq | grep "Tempo"
if diff -q results.csv $GAB_DIST >/dev/null; then
    echo -e "${GREEN}[OK] Resultados corretos.${NC}"
else
    echo -e "${RED}[ERRO] Diff falhou no Sequencial${NC}"
fi

THREADS_LIST=(1 2 4 8)

for t in "${THREADS_LIST[@]}"; do
    echo -e "\n----------------------------------------"
    echo -e "TESTANDO COM OMP_NUM_THREADS = $t"
    echo "----------------------------------------"
    export OMP_NUM_THREADS=$t

    echo -n "Critical: "
    ./analyzer_par_critical | grep "Tempo"
    if diff -q results.csv $GAB_DIST >/dev/null; then
        echo -e "${GREEN}          [OK] Resultados corretos.${NC}"
    else
        echo -e "${RED}          [ERRO] Diff falhou!${NC}"
    fi
    
    echo -n "Atomic:   "
    ./analyzer_par_atomic | grep "Tempo"
    if diff -q results.csv $GAB_DIST >/dev/null; then
        echo -e "${GREEN}          [OK] Resultados corretos.${NC}"
    else
        echo -e "${RED}          [ERRO] Diff falhou!${NC}"
    fi
done

echo -e "\n${CYAN}=== 4. TESTE DE CONTENÇÃO (LOG CONCORRENTE) ===${NC}"
cp $LOG_CONC $LOG_TARGET

THREADS_CONC=8
export OMP_NUM_THREADS=$THREADS_CONC

echo -e "${GREEN}>> Rodando Comparativo de Contenção (8 Threads)${NC}"

echo -n "Sequencial: "
./analyzer_seq | grep "Tempo"
if diff -q results.csv $GAB_CONC >/dev/null; then
    echo -e "${GREEN}            [OK] Resultados corretos.${NC}"
else
    echo -e "${RED}            [ERRO] Diff falhou!${NC}"
fi

echo -n "Critical:   "
./analyzer_par_critical | grep "Tempo"
if diff -q results.csv $GAB_CONC >/dev/null; then
    echo -e "${GREEN}            [OK] Resultados corretos.${NC}"
else
    echo -e "${RED}            [ERRO] Diff falhou!${NC}"
fi

echo -n "Atomic:     "
./analyzer_par_atomic | grep "Tempo"
if diff -q results.csv $GAB_CONC >/dev/null; then
    echo -e "${GREEN}            [OK] Resultados corretos.${NC}"
else
    echo -e "${RED}            [ERRO] Diff falhou!${NC}"
fi

echo -e "\n${CYAN}=== FIM DOS TESTES ===${NC}"
rm $LOG_TARGET