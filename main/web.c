#ifndef WEB_C
#define WEB_C

#include <stdio.h>
#include <string.h>
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "scan.h"
#include "webstr.h"

#define MIN(x, y) x > y ? y : x
#define TAG "WEB"

#define HTMLMAXLEN 50000
#define BOXMAXLEN 100
// https://www.sitepoint.com/draw-rectangle-html

; // code wont conpile with out this, really don't understand why this is a thing

/* functions */
char *getHTMLRespond();
char *createOnlineBox(uint32_t);
char *createOfflineBox(uint32_t);
char *createPrevOnlineBox(uint32_t);
char *MAC2Char(uint8_t*);


/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req)
{
    /* Send a simple response */
    char *respond = getHTMLRespond(); // request html respon create using current database data
    httpd_resp_send(req, respond, HTMLMAXLEN); // send html respond to client
    free(respond); // free html respond after sending
    return ESP_OK;
}

/* Our URI handler function to be called during POST /uri request */
esp_err_t post_handler(httpd_req_t *req)
{
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Respond POST");
    /* Send a simple response */
    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

/* URI handler structure for POST /uri */
httpd_uri_t uri_post = {
    .uri      = "/",
    .method   = HTTP_POST,
    .handler  = post_handler,
    .user_ctx = NULL
};

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}


// reconstruct html
char * getHTMLRespond(){
    char *respond = calloc(HTMLMAXLEN, sizeof(char));
    if(respond == NULL){
        // respond if not enought space
        respond = calloc(20, sizeof(char));
        respond = "not enough space";
        return respond;
    }
    // headers
    strcat(respond, HEAD);
    // contents
    for(uint32_t i=0; i<getMaxDevice(); i++){
        char *newBox; // new device box
        if(getDeviceInfos()[i].online == 1) // online
            newBox = createOnlineBox(i);
        else if(getDeviceInfos()[i].online == 0) // offline
            newBox = createOfflineBox(i);
        else if(getDeviceInfos()[i].online == 2) // previous online
            newBox = createPrevOnlineBox(i);
        else
            newBox = NULL;
        if(newBox == NULL) break; // break if no space

        strcat(respond, newBox); // add into respond
        free(newBox); // free the space
    }
    // tails
    strcat(respond, TAIL);

    // log success message
    ESP_LOGI(TAG, "Successfully created HTML respond");
    return respond;
}

char * createOnlineBox(uint32_t num){
    char *onlineBox = calloc(BOXMAXLEN, sizeof(char)); // create space for storing new box
    if(onlineBox == NULL) return NULL; // check if correct

    // create char ip
    esp_ip4_addr_t ip;
    char char_ip[IP4ADDR_STRLEN_MAX];
    ip.addr = getDeviceInfos()[num].ip; // getip from database
    esp_ip4addr_ntoa(&ip, char_ip, IP4ADDR_STRLEN_MAX);

    // create char MAC
    char *char_mac = MAC2Char(getDeviceInfos()[num].mac);


    sprintf(onlineBox, CHAR_ONLINE_BOX, char_ip, char_mac);
    return onlineBox;
}

char * createOfflineBox(uint32_t num){
    char *offlineBox = calloc(BOXMAXLEN, sizeof(char)); // create space for storing new box
    if(offlineBox == NULL) return NULL; // check if correct

    esp_ip4_addr_t ip;
    char char_ip[IP4ADDR_STRLEN_MAX];
    ip.addr = getDeviceInfos()[num].ip; // getip from database
    esp_ip4addr_ntoa(&ip, char_ip, IP4ADDR_STRLEN_MAX);


    sprintf(offlineBox, CHAR_OFFLINE_BOX, char_ip);
    return offlineBox;
}

char * createPrevOnlineBox(uint32_t num){
    char *prevOnlineBox = calloc(BOXMAXLEN, sizeof(char)); // create space for storing new box
    if(prevOnlineBox == NULL) return NULL; // check if correct

    // create char ip
    esp_ip4_addr_t ip;
    char char_ip[IP4ADDR_STRLEN_MAX];
    ip.addr = getDeviceInfos()[num].ip; // getip from database
    esp_ip4addr_ntoa(&ip, char_ip, IP4ADDR_STRLEN_MAX);

    // create char MAC
    char *char_mac = MAC2Char(getDeviceInfos()[num].mac);

    sprintf(prevOnlineBox, CHAR_PREVONLINE_BOX, char_ip, char_mac);
    return prevOnlineBox;
}

// turn uint8_t[6] to string
char *MAC2Char(uint8_t *mac){
    char *char_mac = malloc(18*sizeof(char));
    sprintf(char_mac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return char_mac;
}

// MAC Lookup
/* https://maclookup.app/ */



#endif
