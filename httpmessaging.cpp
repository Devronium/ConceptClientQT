#include "httpmessaging.h"
#include "AnsiString.h"
#include <stdlib.h>

#define USER_AGENT    "concept://1.2"

struct MemoryStruct {
    char         *memory;
    size_t       size;
    size_t       offset;
    PROGRESS_API notify_parent;
    bool         idle_call;
    bool         shown;
};

static size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t realsize          = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return -1;
    }

    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    if ((mem->notify_parent) && (mem->size > BIG_MESSAGE) && (!mem->shown)) {
        mem->notify_parent(-1, 5, mem->idle_call);
        mem->shown = true;
    }

    return realsize;
}

int GetResponse(struct MemoryStruct *resp) {
    int response = -1;

    if (resp->memory) {
        char *resp_start = strchr(resp->memory, ' ');
        if (resp_start) {
            resp_start++;
            char *resp_end = strchr(resp_start, ' ');
            if (resp_end) {
                resp_end[0] = 0;
                response    = atoi(resp_start);
            }
        }
        free(resp->memory);
        resp->memory = 0;
        resp->size   = 0;
    }
    return response;
}

void HTTPInit() {
    //curl_global_init(CURL_GLOBAL_ALL);
}

int HTTPPostSend(char *host, char *data, int len, int max_packet, PROGRESS_API notify_parent, bool idle_call) {
    int res = len;

    if ((!len) || (!data))
        return 0;

    /*
       int pack_size = len;
       if (max_packet > 0)
        pack_size = len > max_packet ? max_packet : len;
       do {
        struct MemoryStruct chunk;
        struct MemoryStruct chunk_resp;
        chunk.memory        = 0;
        chunk.size          = 0;
        chunk.offset        = 0;
        chunk.notify_parent = notify_parent;
        chunk.idle_call     = idle_call;
        chunk.shown         = false;

        chunk_resp.memory        = 0;
        chunk_resp.size          = 0;
        chunk_resp.offset        = 0;
        chunk_resp.notify_parent = 0;
        chunk_resp.idle_call     = 0;
        chunk_resp.shown         = false;

        CURL *handle = curl_easy_init();
        if (!handle)
            return(-1);
        struct curl_httppost *formpost = NULL;
        struct curl_httppost *lastptr  = NULL;

        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(handle, CURLOPT_WRITEHEADER, (void *)&chunk_resp);
        curl_formadd(&formpost, &lastptr,
                     CURLFORM_PTRNAME, "b",
                     CURLFORM_BUFFER, "x",
                     CURLFORM_BUFFERPTR, data,
                     CURLFORM_BUFFERLENGTH, (long)pack_size,
                     CURLFORM_END);

        curl_easy_setopt(handle, CURLOPT_URL, host);
        curl_easy_setopt(handle, CURLOPT_HTTPPOST, formpost);
        curl_easy_setopt(handle, CURLOPT_USERAGENT, USER_AGENT);
        if (curl_easy_perform(handle))
            res = -2;

        curl_easy_cleanup(handle);
        curl_formfree(formpost);

        int response = GetResponse(&chunk_resp);
        if (((response < 200) && (response != 100)) || ((response >= 300) && (response != 304)))
            res = -3;
        else {
            len -= max_packet;
            if (len > 0)
                data += max_packet;
        }
        len -= max_packet;
        if (len > 0)
            data += max_packet;

        if (len < pack_size)
            pack_size = len;

        free(chunk.memory);
       } while ((len > 0) && (res > 0));*/
    return res;
}

int HTTPGetRecv(char *host, char **data, int *len, PROGRESS_API notify_parent, bool idle_call) {
    int res = 0;

/*    *data = 0;
 * len  = 0;


    int response = -1;
    int out;
    do {
        out = 1;
        CURL *handle = curl_easy_init();
        if (!handle)
            return(-1);
        struct MemoryStruct chunk;
        struct MemoryStruct chunk_resp;
        chunk.memory        = 0;
        chunk.size          = 0;
        chunk.offset        = 0;
        chunk.notify_parent = notify_parent;
        chunk.idle_call     = idle_call;
        chunk.shown         = false;

        chunk_resp.memory        = 0;
        chunk_resp.size          = 0;
        chunk_resp.offset        = 0;
        chunk_resp.notify_parent = notify_parent;
        chunk_resp.idle_call     = idle_call;

        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(handle, CURLOPT_WRITEHEADER, (void *)&chunk_resp);
        curl_easy_setopt(handle, CURLOPT_URL, host);
        curl_easy_setopt(handle, CURLOPT_USERAGENT, USER_AGENT);
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
        headers = curl_slist_append(headers, "Connection: close");
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

        if (curl_easy_perform(handle))
            res = -2;

        if ((chunk.shown) && (notify_parent))
            notify_parent(101, 0, idle_call);

        curl_slist_free_all(headers);
        response = GetResponse(&chunk_resp);
        if ((response < 200) || ((response >= 300) && (response != 304)))
            if (response != -1)
                res = -3;


        curl_easy_cleanup(handle);

        if ((response == 200) && (chunk.size == 0)) {
            if (chunk.memory) {
                free(chunk.memory);
                chunk.memory = 0;
            }
            out = 0;
            continue;
        }

        if (res >= 0) {
 * data = chunk.memory;
 * len  = chunk.size;
            res   = chunk.size;
        } else {
            if (chunk.memory)
                free(chunk.memory);
            return(res);
        }
    } while (!out);*/
    return res;
}

char *piece       = 0;
int  piece_size   = 0;
int  piece_offset = 0;
int HTTPRecv(char *host, char *data, int len, int flags, PROGRESS_API notify_parent, bool idle_call) {
    int res = -1;

    /*if ((piece_size) && (piece_offset >= piece_size)) {
        if (piece)
            free(piece);
        piece        = 0;
        piece_size   = 0;
        piece_offset = 0;
       }
       if (!piece)
        res = HTTPGetRecv(host, &piece, &piece_size, notify_parent);

       if (piece) {
        int  size        = len;
        char *start      = piece + piece_offset;
        int  piece_vsize = piece_size - piece_offset;

        if (size > piece_vsize) {
            size = piece_size;
            memcpy(data, start, size);
            res = size;
            free(piece);
            piece        = 0;
            piece_size   = 0;
            piece_offset = 0;
        } else {
            memcpy(data, start, size);
            res           = size;
            piece_offset += size;
        }
       }*/
    return res;
}

int HTTPHaveMessage(char *host) {
    /*if (piece) {
        if ((piece_size) && (piece_offset < piece_size))
            return(1);
        free(piece);
        piece        = 0;
        piece_size   = 0;
        piece_offset = 0;
        return(0);
       }
       char *buf = 0;
       int  len  = 0;
       HTTPGetRecv(host, &buf, &len);
       if (buf)
        if ((len == 1) && (buf[0] == '1')) {
            HTTPFreeData(buf);
            return(1);
        }
       HTTPFreeData(buf);
     */
    return 0;
}

void HTTPFreeData(char *buffer) {
    if (buffer)
        free(buffer);
}
