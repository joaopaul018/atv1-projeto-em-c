#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_SENSORS 100
#define MAX_LINE 256
#define MAX_ID_LEN 32
#define MAX_VAL_LEN 17

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
   
    if (strcmp(val, "true") == 0 || strcmp(val, "false") == 0) return BOOL;

    
    if (strchr(val, '.') != NULL) 
    {
        char* endptr;
        strtod(val, &endptr);
        if (*endptr == '\0') return FLOAT;
    }

    
    char* endptr;
    strtol(val, &endptr, 10);
    if (*endptr == '\0') return INT;

    
    int is_alphanum = 1;
    for (int i = 0; i < strlen(val); i++) 
    {
        char c = val[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9'))) 
             {
            is_alphanum = 0;
            break;
        }
    }
    if (strlen(val) <= 16 && is_alphanum) 
    {
        return STR;
    }

    return UNKNOWN;
}


int compare_entries(const void* a, const void* b) 
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

    FILE* fp = fopen(argv[1], "r");
    if (!fp) 
    {
        fprintf(stderr, "Erro ao abrir o arquivo.\n");
        return 1;
    }

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) 
    {
        long timestamp;
        char id[MAX_ID_LEN];
        char value[MAX_VAL_LEN];

        
        char* newline = strchr(line, '\n');
        if (newline) *newline = '\0';

        
        if (sscanf(line, "%ld %s", &timestamp, id) != 2) 
        {
            fprintf(stderr, "Linha inválida (falta ID ou timestamp): %s\n", line);
            continue;
        }

        
        char* start = line;
        char* first_space = strchr(start, ' ');
        if (!first_space) continue;
        start = first_space + 1;
        first_space = strchr(start, ' ');
        if (!first_space) continue;
        start = first_space + 1;

        strcpy(value, start); 

        
        int found = -1;
        for (int i = 0; i < sensor_count; i++) 
        {
            if (strcmp(sensors[i].id, id) == 0) 
            {
                found = i;
                break;
            }
        }

        if (found == -1) 
        {
            if (sensor_count >= MAX_SENSORS) 
            {
                fprintf(stderr, "Limite de sensores atingido.\n");
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
            s->entries = realloc(s->entries, s->capacity * sizeof(DataEntry));
        }

        s->entries[s->count].timestamp = timestamp;
        strncpy(s->entries[s->count].value, value, MAX_VAL_LEN - 1);
        s->entries[s->count].value[MAX_VAL_LEN - 1] = '\0';
        s->count++;
    }

    fclose(fp);

    
    for (int i = 0; i < sensor_count; i++) 
    {
        Sensor* s = &sensors[i];
        qsort(s->entries, s->count, sizeof(DataEntry), compare_entries);
        save_sensor_data(s);
        free(s->entries);
    }

    printf("Processamento concluido!\n");

    return 0;
}
