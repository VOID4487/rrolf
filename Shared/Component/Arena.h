#pragma once

#include <Shared/Component/Common.h>
#include <Shared/Entity.h>
#include <Shared/Utilities.h>

struct rr_simulation;
struct rr_encoder;
RR_CLIENT_ONLY(struct rr_renderer;)

struct rr_component_arena
{
                   EntityIdx parent_id;
                   float radius;
    RR_SERVER_ONLY(uint64_t protocol_state;)
};

void rr_component_arena_init(struct rr_component_arena *);
void rr_component_arena_free(struct rr_component_arena *);

RR_SERVER_ONLY(void rr_component_arena_write(struct rr_component_arena *, struct rr_encoder *, int is_creation);)
RR_CLIENT_ONLY(void rr_component_arena_read(struct rr_component_arena *, struct rr_encoder *);)

RR_DECLARE_PUBLIC_FIELD(arena, float, radius)