#include <Client/Socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Client/Game.h>
#ifndef EMSCRIPTEN
#include <libwebsockets.h>
#else
#include <emscripten.h>
#endif

#include <Shared/Crypto.h>

uint8_t *output_packet;
static uint8_t incoming_data[1024 * 1024];
static uint8_t output_buffer_pool[16 * 1024] = {0};
static uint32_t packet_lengths[32] = {0};
static uint32_t at = 0;

#ifdef EMSCRIPTEN
void rr_on_socket_event_emscripten(struct rr_websocket *this,
                                   enum rr_websocket_event_type type,
                                   void *data, uint64_t data_size)
{
    rr_game_websocket_on_event_function(type, data, this->user_data, data_size);
    //this->on_event(type, data, this->user_data, data_size);
}
#else
int rr_on_socket_event_lws(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t size)
{
    struct rr_websocket *this = lws_context_user(lws_get_context(wsi));

    switch (reason)
    {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        this->on_event(rr_websocket_event_type_open, NULL, this->user_data, 0);
        break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
        this->on_event(rr_websocket_event_type_data, in, this->user_data, size);
        break;
    case LWS_CALLBACK_CLIENT_CLOSED:
        this->on_event(rr_websocket_event_type_close, NULL, this->user_data, size);
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        fputs((char *)in, stderr);
        fputs("\n", stderr);
        abort();
        break;
    default:
        break;
    }
    return 0;
}
#endif

void rr_websocket_init(struct rr_websocket *this)
{
    //void *event = this->on_event;
    memset(this, 0, sizeof *this);
    //this->on_event = event; //cursed
}

void rr_websocket_connect_to(struct rr_websocket *this, char const *link)
{
    printf("connecting to server %s\n", link);
    this->recieved_first_packet = 0;
    this->found_error = 0;
#ifdef EMSCRIPTEN
    EM_ASM(
        {
            let string = "";
            while (Module.HEAPU8[$1])
                string += String.fromCharCode(Module.HEAPU8[$1++]);
            if (Module.socket && Module.socket.readyState < 2)
                Module.socket.close();
            (function() {
                let socket = Module.socket =
                    new WebSocket(string);
                socket.binaryType = "arraybuffer";
                socket.onopen = function()
                {
                    Module._rr_on_socket_event_emscripten($0, 0, 0, 0);
                };
                socket.onclose = function(a, b)
                {
                    console.log("close", a, b);
                    const buf = new TextEncoder().encode(a.reason);
                    HEAPU8.set(buf, $2);
                    Module._rr_on_socket_event_emscripten($0, 1, $2, a.code);
                };
                socket.onerror = function(a, b) { 
                       console.log("error", a, b); 
                };
                socket.onmessage = function(event)
                {
                    HEAPU8.set(new Uint8Array(event.data), $2);
                    Module._rr_on_socket_event_emscripten(
                        $0, 2, $2, new Uint8Array(event.data).length);
                };
            })();
        },
        this, link, incoming_data);
#else
    struct lws_context_creation_info info;
    struct lws_protocols protocols[2] = {{"g", rr_on_socket_event_lws, 0, 0},
                                         {NULL, NULL, 0, 0}};
    memset(&info, 0, sizeof info);
    protocols[0].callback = rr_on_socket_event_lws;
    protocols[0].name = "g";

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.user = this;
    info.pt_serv_buf_size = 1024 * 1024;

    this->socket_context = lws_create_context(&info);

    struct lws_client_connect_info connection_info;
    memset(&connection_info, 0, sizeof connection_info);
    connection_info.context = this->socket_context;
    connection_info.address = host;
    connection_info.port = port;
    connection_info.host = lws_canonical_hostname(this->socket_context);
    connection_info.origin = "ggez";
    connection_info.protocol = "g";
    this->socket = lws_client_connect_via_info(&connection_info);

#endif
}

void rr_websocket_disconnect(struct rr_websocket *this, struct rr_game *game)
{
#ifdef EMSCRIPTEN
    EM_ASM({
        if (Module.socket)
            Module.socket.close();
    });
#else
#endif
    //free(this->rivet_player_token);
    free(this->curr_link);
    game->socket_ready = 0;
    game->simulation_ready = 0;
}

void rr_websocket_send(struct rr_websocket *this, uint32_t length)
{
    rr_encrypt(output_packet, length, this->serverbound_encryption_key);
    this->serverbound_encryption_key =
        rr_get_hash(rr_get_hash(this->serverbound_encryption_key));
// printf("pooling packet of length %d at ptr %p\n", length, output_packet);
#ifndef EMSCRIPTEN
    lws_write(this->socket, output_packet, length, LWS_WRITE_BINARY);
#else
    EM_ASM({ Module.socket.send(Module.HEAPU8.subarray($0, $0 + $1)); },
           output_packet, length);
#endif
    // packet_lengths[at] = length;
    // output_packet += length;
    //++at;
}

void rr_websocket_send_all(struct rr_websocket *this)
{
    uint8_t *offset = &output_buffer_pool[0];
    for (uint32_t i = 0; i < at; ++i)
    {
        // printf("sending packret of length %d at ptr %p\n", packet_lengths[i],
        // offset);
        offset += packet_lengths[i];
    }
    at = 0;
    output_packet = &output_buffer_pool[0];
}