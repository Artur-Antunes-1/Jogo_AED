#include "ai_service.h"

#include "config.h"

#include <curl/curl.h>
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* memory;
    size_t size;
} MemoryStruct;

static size_t writeMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t actualSize = size * nmemb;
    MemoryStruct* mem = (MemoryStruct*)userp;

    char* ptr = (char*)realloc(mem->memory, mem->size + actualSize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memória (realloc)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, actualSize);
    mem->size += actualSize;
    mem->memory[mem->size] = 0;

    return actualSize;
}

static char* duplicateString(const char* source) {
    if (!source) {
        return NULL;
    }
    size_t length = strlen(source) + 1;
    char* copy = (char*)malloc(length);
    if (copy) {
        memcpy(copy, source, length);
    }
    return copy;
}

char* call_gemini_api(const char* prompt) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Erro ao iniciar o cURL\n");
        return NULL;
    }

    MemoryStruct chunk = { .memory = (char*)malloc(1), .size = 0 };
    if (!chunk.memory) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    char api_url[200];
    sprintf(api_url, "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=%s", API_KEY);

    cJSON* json_payload = cJSON_CreateObject();
    cJSON* contents = cJSON_CreateArray();
    cJSON* part_obj = cJSON_CreateObject();
    cJSON* parts_array = cJSON_CreateArray();
    cJSON* text_obj = cJSON_CreateObject();

    cJSON_AddStringToObject(text_obj, "text", prompt);
    cJSON_AddItemToArray(parts_array, text_obj);
    cJSON_AddItemToObject(part_obj, "parts", parts_array);
    cJSON_AddItemToArray(contents, part_obj);
    cJSON_AddItemToObject(json_payload, "contents", contents);

    char* json_string = cJSON_Print(json_payload);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    char* response_text = NULL;

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() falhou: %s\n", curl_easy_strerror(res));
    } else {
        cJSON* json_response = cJSON_Parse(chunk.memory);
        if (json_response == NULL) {
            fprintf(stderr, "Erro ao analisar JSON: %s\n", cJSON_GetErrorPtr());
        } else {
            cJSON* error = cJSON_GetObjectItem(json_response, "error");
            if (error) {
                cJSON* errorMessage = cJSON_GetObjectItem(error, "message");
                if (cJSON_IsString(errorMessage)) {
                    fprintf(stderr, "ERRO DA API: %s\n", errorMessage->valuestring);
                }
            } else {
                cJSON* candidates = cJSON_GetObjectItem(json_response, "candidates");
                if (cJSON_IsArray(candidates)) {
                    cJSON* candidate = cJSON_GetArrayItem(candidates, 0);
                    if (candidate) {
                        cJSON* content = cJSON_GetObjectItem(candidate, "content");
                        cJSON* parts = cJSON_GetObjectItem(content, "parts");
                        if (cJSON_IsArray(parts)) {
                            cJSON* part = cJSON_GetArrayItem(parts, 0);
                            cJSON* text = cJSON_GetObjectItem(part, "text");
                            if (cJSON_IsString(text) && (text->valuestring != NULL)) {
                                response_text = duplicateString(text->valuestring);
                            }
                        }
                    }
                }
            }
            cJSON_Delete(json_response);
        }
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(json_string);
    cJSON_Delete(json_payload);
    free(chunk.memory);

    return response_text;
}

void list_available_models(void) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return;
    }

    MemoryStruct chunk = { .memory = (char*)malloc(1), .size = 0 };
    if (!chunk.memory) {
        curl_easy_cleanup(curl);
        return;
    }

    printf("Verificando modelos de IA disponíveis...\n");

    char api_url[200];
    sprintf(api_url, "https://generativelanguage.googleapis.com/v1beta/models?key=%s", API_KEY);

    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "list_models() falhou: %s\n", curl_easy_strerror(res));
    } else {
        printf("\n--- LISTA DE MODELOS DISPONÍVEIS (JSON) ---\n%s\n----------------------------------------------\n\n", chunk.memory);
    }

    curl_easy_cleanup(curl);
    free(chunk.memory);
}

