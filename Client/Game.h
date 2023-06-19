#pragma once

#include <Shared/Entity.h>
#include <Client/Ui/Element.h>
#include <Client/Socket.h>

#include <Shared/StaticData.h>

struct rr_game_loadout_petal
{
    uint8_t id;
    uint8_t rarity;
};

struct rr_game_squad_client
{
    uint8_t in_use;
    uint8_t ready;
    struct rr_game_loadout_petal loadout[20];
};

struct rr_global_elements
{
    struct rr_ui_element *respawn_label;
    struct rr_ui_element *title_screen;
    struct rr_ui_element *loadout;
    struct rr_ui_element *wave_info;
    struct rr_ui_element *inventory;
    struct rr_ui_element *in_game_squad_info;
    struct rr_ui_element *game_over;
};

struct rr_game
{
    struct rr_websocket socket;
    struct rr_renderer *renderer;
    struct rr_renderer static_petals[rr_petal_id_max][rr_rarity_id_max];
    struct rr_input_data *input_data;
    struct rr_simulation *simulation;
    struct rr_ui_element *global_container;
    struct rr_global_elements ui_elements;
    uint8_t displaying_debug_information:1;
    uint8_t socket_ready:1;
    uint8_t socket_pending:1;
    uint8_t simulation_ready:1;
    int8_t ticks_until_game_start;
    float expanding_circle_radius;
    struct rr_game_squad_client squad_members[4];
    uint32_t inventory[rr_petal_id_max][rr_rarity_id_max];
    //stuff
    struct rr_game_loadout_petal loadout[20];
    uint32_t protocol_state;
};

void rr_game_init(struct rr_game *);
void rr_game_tick(struct rr_game *, float);
void rr_game_connect_socket(struct rr_game *);
