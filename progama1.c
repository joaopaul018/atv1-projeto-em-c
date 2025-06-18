#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SENSORS 100
#define MAX_LINE 256
#define MAX_ID_LEN 32
#define MAX_VAL_LEN 17


#define is_digit(c) ((c) >= '0' && (c) <= '9')
#define is_alpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define is_alphanum(c) (is_digit(c) || is_alpha(c))
#define is_space(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == '\v' || (c) == '\f')

typedef enum 
{
    INT,
    BOOL,
    FLOAT,
    STR,
    UNKNOWN
} DataType;

typedef struct 
{
    long timestamp;
    char value[MAX_VAL_LEN];
} DataEntry;

typedef struct 
{
    char id[MAX_ID_LEN];
    DataType type;
    DataEntry* entries;
    int count;
    int capacity;
} Sensor;

Sensor sensors[MAX_SENSORS];
int sensor_count = 0;

DataType detect_type(const char* val) 
{
    if (strcmp(val, "true") == 0 || strcmp(val, "false") == 0)
        return BOOL;

    if (strchr(val, '.') != NULL) 
    {
        char* endptr;
        strtod(val, &endptr);
        if (*endptr == '\0') return FLOAT;
    }

    char* endptr;
    strtol(val, &endptr, 10);
    if (*endptr == '\0') return INT;

    int is_alphanum_str = 1;
    for (int i = 0; val[i]; i++) 
    {
        if (!is_alphanum(val[i])) 
        {
            is_alphanum_str = 0;
            break;
        }
    }
    if (strlen(val) <= 16 && is_alphanum_str) 
    {
        return STR;
    }

    return UNKNOWN;
}

int compare_entries_desc(const void* a, const void* b) 
{
    DataEntry* entry_a = (DataEntry*)a;
    DataEntry* entry_b = (DataEntry*)b;
    if (entry_a->timestamp > entry_b->timestamp) return -1;
    if (entry_a->timestamp < entry_b->timestamp) return 1;
    return 0;
}

void save_sensor_data(Sensor* sensor) 
{
    char filename[64];
    snprintf(filename, sizeof(filename), "%s.dat", sensor->id);

    FILE* fp = fopen(filename, "w");
    if (!fp) 
    {
        fprintf(stderr, "Erro ao abrir arquivo: %s\n", filename);
        return;
    }

    for (int i = 0; i < sensor->count; i++) 
    {
        fprintf(fp, "%ld %s\n", sensor->entries[i].timestamp, sensor->entries[i].value);
    }

    fclose(fp);
}

int main(int argc, char* argv[]) 
{
    if (argc != 2) 
    {
        fprintf(stderr, "Uso: %s <nome_do_arquivo>\n", argv[0]);
        return 1;
    }

    if (argv[1] == NULL || strlen(argv[1]) == 0) 
    {
        fprintf(stderr, "Erro: Nome do arquivo não pode ser vazio.\n");
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (!fp) 
    {
        fprintf(stderr, "Erro ao abrir o arquivo '%s'.\n", argv[1]);
        return 1;
    }

    char line[MAX_LINE];
    int line_number = 0;

    while (fgets(line, sizeof(line), fp)) 
    {
        line_number++;

        line[strcspn(line, "\n")] = '\0';

        int empty = 1;
        for (int i = 0; line[i]; i++) 
        {
            if (!is_space(line[i])) 
            {
                empty = 0;
                break;
            }
        }
        if (empty) continue;

        long timestamp;
        char id[MAX_ID_LEN];
        char value[MAX_VAL_LEN];

        char* token = strtok(line, " ");
        if (!token || !is_digit(token[0]) && !(token[0] == '-' && token[1] != '\0')) 
        {
            printf("Linha %d: Timestamp invalido\n", line_number);
            continue;
        }

        char* endptr;
        timestamp = strtol(token, &endptr, 10);
        if (*endptr != '\0') 
        {
            printf("Linha %d: Formato de timestamp invalido\n", line_number);
            continue;
        }

        token = strtok(NULL, " ");
        if (!token) 
        {
            printf("Linha %d: Falta o ID do sensor\n", line_number);
            continue;
        }
        strncpy(id, token, MAX_ID_LEN - 1);
        id[MAX_ID_LEN - 1] = '\0';

        token = strtok(NULL, "");
        if (!token) 
        {
            printf("Linha %d: Falta o valor do sensor\n", line_number);
            continue;
        }
        strncpy(value, token, MAX_VAL_LEN - 1);
        value[MAX_VAL_LEN - 1] = '\0';

        int found = -1;
        for (int i = 0; i < sensor_count; i++) 
        {
            if (strcmp(sensors[i].id, id) == 0) 
            {
                found = i;
                break;
            }
        }

        if (found == -1) {
            if (sensor_count >= MAX_SENSORS) 
            {
                printf("Limite de sensores atingido. Ignorando linha %d.\n", line_number);
                continue;
            }
            strcpy(sensors[sensor_count].id, id);
            sensors[sensor_count].type = detect_type(value);
            sensors[sensor_count].count = 0;
            sensors[sensor_count].capacity = 10;
            sensors[sensor_count].entries = malloc(sensors[sensor_count].capacity * sizeof(DataEntry));
            found = sensor_count++;
        }

        Sensor* s = &sensors[found];

        if (s->count >= s->capacity) 
        {
            s->capacity *= 2;
            DataEntry* temp = realloc(s->entries, s->capacity * sizeof(DataEntry));
            if (!temp) 
            {
                printf("Erro de alocação na linha %d\n", line_number);
                fclose(fp);
                for (int i = 0; i < sensor_count; i++) free(sensors[i].entries);
                return 1;
            }
            s->entries = temp;
        }

        s->entries[s->count].timestamp = timestamp;
        strncpy(s->entries[s->count].value, value, MAX_VAL_LEN - 1);
        s->entries[s->count].value[MAX_VAL_LEN - 1] = '\0';
        s->count++;
    }

    fclose(fp);

    for (int i = 0; i < sensor_count; i++) 
    {
        qsort(sensors[i].entries, sensors[i].count, sizeof(DataEntry), compare_entries_desc);
        save_sensor_data(&sensors[i]);
        free(sensors[i].entries);
    }

    printf("Processamento concluido!\n");

    return 0;
}
