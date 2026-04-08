//
// Basado en el ejemplo oficial de MsQuic:
//   https://github.com/microsoft/msquic/blob/main/src/tools/sample/sample.c
// Documentacion de la API:
//   https://github.com/microsoft/msquic/blob/main/docs/API.md
//
// Compilar:
//   gcc publisher.c -o publisher -I/usr/local/include/msquic -lmsquic
//
// Ejecutar:
//   ./publisher

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <msquic.h>

// -- Constantes ------------------------------------------------------
#define ALPN "broker/1"
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 4433
#define CA_CERT_FILE "cert.crt"
#define TOPIC_MAX 64
#define MSG_MAX 1024
#define CMD_MAX 1200

// -- Estado global ---------------------------------------------------
static const QUIC_API_TABLE *MsQuic;
static HQUIC Registration;
static HQUIC Configuration;
static HQUIC Connection;

static bool g_connected = false;
static bool g_exit = false;

static char g_host[128] = {0};
static uint16_t g_port = 4433;
static char g_topic[TOPIC_MAX] = {0};

static void print_message_prompt(void) {
    printf("Mensaje (/salir para terminar): ");
    fflush(stdout);
}

typedef struct {
    QUIC_BUFFER buffer;
    uint8_t *payload;
    uint32_t length;
} PendingSend;

static QUIC_STATUS QUIC_API
StreamCallback(HQUIC stream, void *ctx, QUIC_STREAM_EVENT *e);

// -- Envio del comando PUB ------------------------------------------

// send_publish: abre un stream bidireccional cliente<->servidor
// y envia el comando PUB <topic> <mensaje> con FIN.
static void send_publish(const char *message) {
    HQUIC stream = NULL;
    PendingSend *pending = NULL;

    pending = (PendingSend *)calloc(1, sizeof(*pending));
    if (!pending) {
        fprintf(stderr, "[pub] malloc fallo\n");
        return;
    }

    char cmd[CMD_MAX];
    int n = snprintf(cmd, sizeof(cmd), "PUB %s %s\n", g_topic, message);
    if (n <= 0 || n >= (int)sizeof(cmd)) {
        fprintf(stderr, "[pub] comando invalido\n");
        free(pending);
        return;
    }

    pending->payload = (uint8_t *)malloc((size_t)n);
    if (!pending->payload) {
        fprintf(stderr, "[pub] malloc fallo\n");
        free(pending);
        return;
    }
    memcpy(pending->payload, cmd, (size_t)n);
    pending->length = (uint32_t)n;

    if (QUIC_FAILED(MsQuic->StreamOpen(Connection, QUIC_STREAM_OPEN_FLAG_NONE,
                                       StreamCallback, pending, &stream))) {
        fprintf(stderr, "[pub] StreamOpen fallo\n");
        free(pending->payload);
        free(pending);
        return;
    }

    if (QUIC_FAILED(MsQuic->StreamStart(stream, QUIC_STREAM_START_FLAG_NONE))) {
        fprintf(stderr, "[pub] StreamStart fallo\n");
        MsQuic->StreamClose(stream);
        return;
    }
}

// -- Callback de stream ---------------------------------------------

// StreamCallback: se usa para liberar buffers de envio y cerrar streams.
static QUIC_STATUS QUIC_API
StreamCallback(HQUIC stream, void *ctx, QUIC_STREAM_EVENT *e) {
    PendingSend *pending = (PendingSend *)ctx;

    if (e->Type == QUIC_STREAM_EVENT_START_COMPLETE) {
        if (!pending || QUIC_FAILED(e->START_COMPLETE.Status)) {
            fprintf(stderr, "[pub] StreamStart completo fallo\n");
            MsQuic->StreamClose(stream);
            return QUIC_STATUS_SUCCESS;
        }

        pending->buffer.Length = pending->length;
        pending->buffer.Buffer = pending->payload;

        printf("[pub] comando enviado: %.*s\n", (int)pending->length, pending->payload);

        if (QUIC_FAILED(MsQuic->StreamSend(stream, &pending->buffer, 1, QUIC_SEND_FLAG_FIN, pending))) {
            fprintf(stderr, "[pub] StreamSend fallo\n");
            free(pending->payload);
            free(pending);
            MsQuic->StreamClose(stream);
            return QUIC_STATUS_SUCCESS;
        }
    } else if (e->Type == QUIC_STREAM_EVENT_SEND_COMPLETE) {
        PendingSend *send = (PendingSend *)e->SEND_COMPLETE.ClientContext;
        free(send->payload);
        free(send);
    } else if (e->Type == QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE) {
        MsQuic->StreamClose(stream);
    }

    return QUIC_STATUS_SUCCESS;
}

// -- Callback de conexion -------------------------------------------

// ConnectionCallback: replica el patron del sample oficial para cliente.
static QUIC_STATUS QUIC_API
ConnectionCallback(HQUIC conn, void *ctx, QUIC_CONNECTION_EVENT *e) {
    (void)conn;
    (void)ctx;

    if (e->Type == QUIC_CONNECTION_EVENT_CONNECTED) {
        printf("[conn] conectado al broker\n");
        g_connected = true;
    } else if (e->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT) {
        fprintf(stderr, "[conn] cierre por transporte, status=0x%x\n",
                e->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
    } else if (e->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE) {
        printf("[conn] conexion cerrada\n");
        g_exit = true;
        MsQuic->ConnectionClose(Connection);
        Connection = NULL;
    }

    return QUIC_STATUS_SUCCESS;
}

// -- main ------------------------------------------------------------

int main(void) {
    strncpy(g_host, DEFAULT_HOST, sizeof(g_host) - 1);
    g_host[sizeof(g_host) - 1] = '\0';
    g_port = DEFAULT_PORT;

    printf("Topic: ");
    fflush(stdout);
    if (fgets(g_topic, sizeof(g_topic), stdin) == NULL) {
        fprintf(stderr, "No se pudo leer el topic\n");
        return 1;
    }

    g_topic[strcspn(g_topic, "\r\n")] = '\0';
    if (g_topic[0] == '\0') {
        fprintf(stderr, "Topic invalido\n");
        return 1;
    }

    // 1. Abrir API
    if (QUIC_FAILED(MsQuicOpen2(&MsQuic))) {
        fprintf(stderr, "MsQuicOpen2 fallo\n");
        return 1;
    }

    // 2. Registration
    const QUIC_REGISTRATION_CONFIG reg = {
        "publisher",
        QUIC_EXECUTION_PROFILE_LOW_LATENCY,
    };

    if (QUIC_FAILED(MsQuic->RegistrationOpen(&reg, &Registration))) {
        fprintf(stderr, "RegistrationOpen fallo\n");
        MsQuicClose(MsQuic);
        return 1;
    }

    // 3. Configuration de cliente
    QUIC_BUFFER alpn = {
        .Length = sizeof(ALPN) - 1,
        .Buffer = (uint8_t *)ALPN,
    };

    QUIC_SETTINGS settings = {0};
    settings.IdleTimeoutMs = 30000;
    settings.IsSet.IdleTimeoutMs = TRUE;
    settings.PeerUnidiStreamCount = 16;
    settings.IsSet.PeerUnidiStreamCount = TRUE;

    if (QUIC_FAILED(MsQuic->ConfigurationOpen(Registration, &alpn, 1, &settings,
                                              sizeof(settings), NULL,
                                              &Configuration))) {
        fprintf(stderr, "ConfigurationOpen fallo\n");
        MsQuic->RegistrationClose(Registration);
        MsQuicClose(MsQuic);
        return 1;
    }

    QUIC_CREDENTIAL_CONFIG cred = {0};
    cred.Type = QUIC_CREDENTIAL_TYPE_NONE;
    cred.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_SET_CA_CERTIFICATE_FILE;
    cred.CaCertificateFile = CA_CERT_FILE;

    if (QUIC_FAILED(MsQuic->ConfigurationLoadCredential(Configuration, &cred))) {
        fprintf(stderr, "ConfigurationLoadCredential fallo\n");
        MsQuic->ConfigurationClose(Configuration);
        MsQuic->RegistrationClose(Registration);
        MsQuicClose(MsQuic);
        return 1;
    }

    // 4. Connection
    if (QUIC_FAILED(MsQuic->ConnectionOpen(Registration, ConnectionCallback, NULL,
                                           &Connection))) {
        fprintf(stderr, "ConnectionOpen fallo\n");
        MsQuic->ConfigurationClose(Configuration);
        MsQuic->RegistrationClose(Registration);
        MsQuicClose(MsQuic);
        return 1;
    }

    if (QUIC_FAILED(MsQuic->ConnectionStart(Connection, Configuration,
                                            QUIC_ADDRESS_FAMILY_INET,
                                            g_host, g_port))) {
        fprintf(stderr, "ConnectionStart fallo\n");
        MsQuic->ConnectionClose(Connection);
        MsQuic->ConfigurationClose(Configuration);
        MsQuic->RegistrationClose(Registration);
        MsQuicClose(MsQuic);
        return 1;
    }

    printf("Conectando publisher a %s:%u topic=%s...\n", g_host, g_port, g_topic);

    while (!g_connected && !g_exit) {
        usleep(10000);
    }

    if (g_exit) {
        fprintf(stderr, "No se pudo establecer la conexion con el broker\n");
    } else {
        printf("Publisher conectado al broker\n");
        printf("Listo para publicar. Escribe mensajes y ENTER para enviarlos.\n");
    }

    while (!g_exit) {
        char line[MSG_MAX];

        print_message_prompt();
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0') {
            continue;
        }

        if (strcmp(line, "/salir") == 0) {
            break;
        }

        send_publish(line);
    }

    if (Connection != NULL) {
        MsQuic->ConnectionShutdown(Connection,
                                   QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
    }

    // Espera simple a shutdown callback
    while (!g_exit) {
        usleep(10000);
    }

    MsQuic->ConfigurationClose(Configuration);
    MsQuic->RegistrationClose(Registration);
    MsQuicClose(MsQuic);

    return 0;
}
