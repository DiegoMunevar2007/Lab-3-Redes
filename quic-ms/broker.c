
// Basado en el ejemplo oficial de MsQuic:
//   https://github.com/microsoft/msquic/blob/main/src/tools/sample/sample.c
// Documentacion de la API:
//   https://github.com/microsoft/msquic/blob/main/docs/API.md
//
// Compilar:
//   gcc broker.c -o broker -I/usr/local/include/msquic -lmsquic
//
// Generar certificado de prueba (OpenSSL):
//   openssl req -nodes -new -x509 -keyout cert.key -out cert.crt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <msquic.h>

// ── Constantes ───────────────────────────────────────────────────────
#define MAX_CLIENTS 32
#define ALPN        "broker/1"   // identificador de protocolo para el handshake TLS
#define CERT_FILE   "cert.crt"
#define KEY_FILE    "cert.key"

// ── Estado de cada cliente conectado ─────────────────────────────────
typedef struct {
    HQUIC connection;
    char  topic[64];
    bool  subscribed;
} Client;

// ── Globales ──────────────────────────────────────────────────────────
static const QUIC_API_TABLE *MsQuic;   // tabla de funciones cargada con MsQuicOpen2
static HQUIC  Registration;
static HQUIC  Configuration;
static HQUIC  Listener;

static Client clients[MAX_CLIENTS];

static QUIC_STATUS QUIC_API
StreamCallback(HQUIC stream, void *ctx, QUIC_STREAM_EVENT *e);

// ── Helpers de la lista de clientes ──────────────────────────────────

// Devuelve un puntero al slot del cliente, o NULL si no está registrado.
static Client *find_client(HQUIC conn) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].connection == conn) return &clients[i];
    return NULL;
}

// Registra una nueva conexión en el primer slot libre.
static Client *add_client(HQUIC conn) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].connection) {
            clients[i].connection  = conn;
            clients[i].subscribed  = false;
            clients[i].topic[0]    = '\0';
            return &clients[i];
        }
    }
    return NULL;   // sin slots disponibles
}

// Libera el slot cuando el cliente se desconecta.
static void remove_client(HQUIC conn) {
    Client *c = find_client(conn);
    if (c) memset(c, 0, sizeof(*c));
}

// ── Envio de un mensaje a un cliente ─────────────────────────────────

typedef struct {
    QUIC_BUFFER buffer;
    uint8_t *payload;
} SendContext;

// send_msg: abre un stream unidireccional y envia el mensaje.
// Patron tomado del sample oficial (ServerSend en sample.c):
//   MsQuic->StreamOpen -> StreamStart -> StreamSend(FIN) -> StreamClose en SEND_COMPLETE
//
// El buffer se copia en heap porque MsQuic lo necesita valido hasta
// el evento SEND_COMPLETE (ver docs/API.md §StreamSend).
static void send_msg(HQUIC conn, const char *msg) {
    HQUIC stream = NULL;
    SendContext *sendContext = NULL;
    // Abre el stream; UNIDIRECTIONAL = solo servidor -> cliente
    if (QUIC_FAILED(MsQuic->StreamOpen(conn, QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL,
                                        StreamCallback, NULL, &stream))) return;
    if (QUIC_FAILED(MsQuic->StreamStart(stream, QUIC_STREAM_START_FLAG_NONE))) {
        MsQuic->StreamClose(stream); return;
    }

    size_t   len  = strlen(msg);
    sendContext = (SendContext *)calloc(1, sizeof(*sendContext));
    if (!sendContext) { MsQuic->StreamClose(stream); return; }

    sendContext->payload = malloc(len);
    if (!sendContext->payload) {
        free(sendContext);
        MsQuic->StreamClose(stream);
        return;
    }

    memcpy(sendContext->payload, msg, len);
    sendContext->buffer.Length = (uint32_t)len;
    sendContext->buffer.Buffer = sendContext->payload;

    // FIN=true cierra el stream tras este unico envio.
    // El contexto se recupera como ClientContext en SEND_COMPLETE.
    if (QUIC_FAILED(MsQuic->StreamSend(stream, &sendContext->buffer, 1, QUIC_SEND_FLAG_FIN, sendContext))) {
        free(sendContext->payload);
        free(sendContext);
        MsQuic->StreamClose(stream);
    }
}

// ── Logica PUB/SUB ───────────────────────────────────────────────────

// Envia el mensaje a todos los suscriptores del topic indicado.
static void relay(const char *topic, const char *msg) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].subscribed && strcmp(clients[i].topic, topic) == 0)
            send_msg(clients[i].connection, msg);
    }
}

// Parsea y ejecuta un comando SUB o PUB recibido de un cliente.
static void process(Client *c, const char *text) {
    char topic[64], payload[1024];

    if (!c || !text || text[0] == '\0') {
        return;
    }

    if (sscanf(text, "SUB %63s", topic) == 1) {
        strncpy(c->topic, topic, sizeof(c->topic) - 1);
        c->topic[sizeof(c->topic) - 1] = '\0';
        c->subscribed = true;
        printf("[SUB] topic=%s\n", topic);
        return;
    }

    if (sscanf(text, "PUB %63s %1023[^\n]", topic, payload) == 2) {
        printf("[PUB] topic=%s msg=%s\n", topic, payload);
        relay(topic, payload);
    }
}

// Callback de stream 

// StreamCallback: recibe eventos del stream abierto por el cliente.
// Estructura identica a ServerStreamCallback del sample oficial.
//https://github.com/microsoft/msquic/blob/main/src/tools/sample/sample.c
static QUIC_STATUS QUIC_API
StreamCallback(HQUIC stream, void *ctx, QUIC_STREAM_EVENT *e)
{
    Client *c = (Client *)ctx;

    if (e->Type == QUIC_STREAM_EVENT_RECEIVE) {
        uint64_t received = e->RECEIVE.TotalBufferLength;

        for (uint32_t i = 0; i < e->RECEIVE.BufferCount; i++) {
            uint32_t len = e->RECEIVE.Buffers[i].Length;
            if (len == 0) {
                continue;
            }

            char tmp[2048];
            uint32_t n = len < sizeof(tmp) - 1 ? len : sizeof(tmp) - 1;
            memcpy(tmp, e->RECEIVE.Buffers[i].Buffer, n);
            tmp[n] = '\0';
            printf("[stream] recv: %s\n", tmp);
            process(c, tmp);
        }

        MsQuic->StreamReceiveComplete(stream, received);
    }

    else if (e->Type == QUIC_STREAM_EVENT_SEND_COMPLETE) {
        // MsQuic termino de enviar; liberamos el buffer que pasamos a StreamSend
        SendContext *sendContext = (SendContext *)e->SEND_COMPLETE.ClientContext;
        free(sendContext->payload);
        free(sendContext);
    }

    else if (e->Type == QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE) {
        MsQuic->StreamClose(stream);
    }

    return QUIC_STATUS_SUCCESS;
}

// ── Callback de conexion ─────────────────────────────────────────────

// ConnectionCallback: gestiona el ciclo de vida de cada conexion.
// Tomado como referencia de ServerConnectionCallback en sample.c.
//https://github.com/microsoft/msquic/blob/main/src/tools/sample/sample.c
static QUIC_STATUS QUIC_API
ConnectionCallback(HQUIC conn, void *ctx, QUIC_CONNECTION_EVENT *e)
{
    (void)ctx;

    if (e->Type == QUIC_CONNECTION_EVENT_CONNECTED) {
        // Handshake TLS completado; enviamos el ticket de reanudacion (0-RTT futuro)
        printf("[conn] cliente conectado\n");
        MsQuic->ConnectionSendResumptionTicket(conn, QUIC_SEND_RESUMPTION_FLAG_NONE, 0, NULL);
    }

    else if (e->Type == QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED) {
        // El cliente abrio un stream nuevo; lo asociamos a StreamCallback
        Client *c = find_client(conn);
        printf("[conn] peer stream started\n");
        MsQuic->SetCallbackHandler(e->PEER_STREAM_STARTED.Stream, (void *)StreamCallback, c);
        MsQuic->StreamReceiveSetEnabled(e->PEER_STREAM_STARTED.Stream, TRUE);
    }

    else if (e->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE) {
        // Conexion completamente cerrada; liberamos el slot
        printf("[conn] cliente desconectado\n");
        remove_client(conn);
        MsQuic->ConnectionClose(conn);
    }

    return QUIC_STATUS_SUCCESS;
}

// ── Callback del listener ────────────────────────────────────────────

// ListenerCallback: acepta conexiones entrantes.
// Patron directo de ServerListenerCallback en sample.c:
//   SetCallbackHandler -> ConnectionSetConfiguration
//https://github.com/microsoft/msquic/blob/main/src/tools/sample/sample.c
static QUIC_STATUS QUIC_API
ListenerCallback(HQUIC listener, void *ctx, QUIC_LISTENER_EVENT *e)
{
    (void)listener; (void)ctx;

    if (e->Type != QUIC_LISTENER_EVENT_NEW_CONNECTION)
        return QUIC_STATUS_SUCCESS;

    HQUIC conn = e->NEW_CONNECTION.Connection;

    Client *c = add_client(conn);

    if (!c) return QUIC_STATUS_CONNECTION_REFUSED;   // sin slots

    // Registramos el callback ANTES de llamar a ConnectionSetConfiguration
    MsQuic->SetCallbackHandler(conn, (void *)ConnectionCallback, NULL);
    MsQuic->ConnectionSetConfiguration(conn, Configuration);

    return QUIC_STATUS_SUCCESS;
}

// ── main ─────────────────────────────────────────────────────────────

int main(void)
{
    QUIC_STATUS status;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // 1. Inicializar la API de MsQuic: devuelve la tabla de funciones.
    //    MsQuicOpen2 es la version moderna de MsQuicOpenVersion(2, ...).
    //   https://github.com/microsoft/msquic/blob/main/docs/API.md
    if (QUIC_FAILED(MsQuicOpen2(&MsQuic))) {
        fprintf(stderr, "MsQuicOpen2 fallo\n");
        return 1;
    }

    // 2. Registro: contexto de ejecucion de toda la app
    const QUIC_REGISTRATION_CONFIG reg = { "broker", QUIC_EXECUTION_PROFILE_LOW_LATENCY };
    status = MsQuic->RegistrationOpen(&reg, &Registration);
    if (QUIC_FAILED(status)) {
        fprintf(stderr, "RegistrationOpen fallo, status=0x%x\n", status);
        MsQuicClose(MsQuic);
        return 1;
    }

    // 3. Config: TLS + parametros QUIC
    //    El ALPN debe coincidir entre cliente y servidor para aceptar la conexion.
    QUIC_BUFFER alpn = { sizeof(ALPN) - 1, (uint8_t *)ALPN };

    QUIC_SETTINGS settings = {0};
    settings.IdleTimeoutMs           = 30000;
    settings.IsSet.IdleTimeoutMs     = TRUE;
    settings.PeerBidiStreamCount     = 16;   // streams que puede abrir el cliente
    settings.IsSet.PeerBidiStreamCount = TRUE;
    settings.PeerUnidiStreamCount    = 16;   // streams unidireccionales desde cliente (SUB/PUB)
    settings.IsSet.PeerUnidiStreamCount = TRUE;

    status = MsQuic->ConfigurationOpen(Registration, &alpn, 1, &settings, sizeof(settings), NULL, &Configuration);
    if (QUIC_FAILED(status)) {
        fprintf(stderr, "ConfigurationOpen fallo, status=0x%x\n", status);
        MsQuic->RegistrationClose(Registration);
        MsQuicClose(MsQuic);
        return 1;
    }

    // Certificado TLS: usa los certificados del laboratorio en ../quic.
    QUIC_CERTIFICATE_FILE cert  = { KEY_FILE, CERT_FILE };
    QUIC_CREDENTIAL_CONFIG cred = {0};
    cred.Type            = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
    cred.CertificateFile = &cert;

    status = MsQuic->ConfigurationLoadCredential(Configuration, &cred);
    if (QUIC_FAILED(status)) {
        fprintf(stderr, "ConfigurationLoadCredential fallo, status=0x%x\n", status);
        fprintf(stderr, "Revisa rutas de certificado/llave: %s y %s\n", CERT_FILE, KEY_FILE);
        MsQuic->ConfigurationClose(Configuration);
        MsQuic->RegistrationClose(Registration);
        MsQuicClose(MsQuic);
        return 1;
    }

    // 4. Listener: escucha en 0.0.0.0 (todas las interfaces IPv4) puerto 4433
    status = MsQuic->ListenerOpen(Registration, ListenerCallback, NULL, &Listener);
    if (QUIC_FAILED(status)) {
        fprintf(stderr, "ListenerOpen fallo, status=0x%x\n", status);
        MsQuic->ConfigurationClose(Configuration);
        MsQuic->RegistrationClose(Registration);
        MsQuicClose(MsQuic);
        return 1;
    }

    QUIC_ADDR addr = {0};
    QuicAddrSetFamily(&addr, QUIC_ADDRESS_FAMILY_INET);
    QuicAddrSetPort(&addr, 4433);
    status = MsQuic->ListenerStart(Listener, &alpn, 1, &addr);
    if (QUIC_FAILED(status)) {
        fprintf(stderr, "ListenerStart fallo, status=0x%x\n", status);
        MsQuic->ListenerClose(Listener);
        MsQuic->ConfigurationClose(Configuration);
        MsQuic->RegistrationClose(Registration);
        MsQuicClose(MsQuic);
        return 1;
    }

    printf("Broker escuchando en 0.0.0.0:4433  (ALPN=%s)\n", ALPN);
    printf("Comandos: SUB <topic>  |  PUB <topic> <mensaje>\n");
    printf("Presiona ENTER para salir...\n");

    // 5. MsQuic usa hilos internos para toda la E/S, el main solo espera.
    getchar();

    // 6. Limpieza en orden inverso al de inicializacion (igual que en sample.c)
    MsQuic->ListenerClose(Listener);
    MsQuic->ConfigurationClose(Configuration);
    MsQuic->RegistrationClose(Registration);
    MsQuicClose(MsQuic);

    return 0;
}