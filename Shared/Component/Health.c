#include <Shared/Component/Health.h>

#include <string.h>

#include <Shared/Entity.h>
#include <Shared/SimulationCommon.h>
#include <Shared/pb.h>

#define FOR_EACH_PUBLIC_FIELD                                                  \
    X(health, float32)                                                         \
    X(flags, uint8)                                                           \
    X(max_health, float32)

enum
{
    state_flags_max_health = 0b001,
    state_flags_flags = 0b010,
    state_flags_health = 0b100,
    state_flags_all = 0b111
};

void rr_component_health_init(struct rr_component_health *this,
                              struct rr_simulation *simulation)
{
    memset(this, 0, sizeof *this);
    this->health = 1;
    this->max_health = 1;
}

void rr_component_health_free(struct rr_component_health *this,
                              struct rr_simulation *simulation)
{
}

#ifdef RR_SERVER
void rr_component_health_write(struct rr_component_health *this,
                               struct proto_bug *encoder, int is_creation,
                               struct rr_component_player_info *client)
{
    uint64_t state = this->protocol_state | (state_flags_all * is_creation);
    proto_bug_write_varuint(encoder, state, "health component state");
    float tmp_health = this->health;
    float tmp_max = this->max_health;
    if (this->flags & 1)
    {
        this->max_health = 0;
        if (this->health != 0)
            this->health = 0.25;
    }
#define X(NAME, TYPE) RR_ENCODE_PUBLIC_FIELD(NAME, TYPE);
    FOR_EACH_PUBLIC_FIELD
#undef X
    this->health = tmp_health;
    this->max_health = tmp_max;
}

void rr_component_health_do_damage(struct rr_component_health *this, float v)
{
    if (this->health == 0)
        return;
    if (v <= this->damage_reduction)
        return;
    rr_component_health_set_flags(this, this->flags | 2);
    v = this->health - (v - this->damage_reduction);
    if (v < 0)
        v = 0;
    this->health = v;
    this->protocol_state |= state_flags_health;
}

void rr_component_health_set_health(struct rr_component_health *this, float v)
{
    if (this->health == 0)
        return;
    if (v > this->max_health)
        v = this->max_health;
    else if (v < 0)
        v = 0;
    if (v < this->health)
        rr_component_health_set_flags(this, this->flags | 2);
    this->protocol_state |= (v != this->health) * state_flags_health;
    this->health = v;
}

RR_DEFINE_PUBLIC_FIELD(health, float, max_health)
RR_DEFINE_PUBLIC_FIELD(health, uint8_t, flags)

#endif

#ifdef RR_CLIENT
void rr_component_health_read(struct rr_component_health *this,
                              struct proto_bug *encoder)
{
    uint64_t state = proto_bug_read_varuint(encoder, "health component state");
#define X(NAME, TYPE) RR_DECODE_PUBLIC_FIELD(NAME, TYPE);
    FOR_EACH_PUBLIC_FIELD
#undef X
}
#endif