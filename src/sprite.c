#include "sprite.h"
#include <string.h>
#include <stdio.h>

#ifndef TESTING_HEADLESS

/* Map manifest animation name to AnimId. Returns -1 if unknown. */
static int anim_name_to_id(const char *name) {
    if (strcmp(name, "idle") == 0)       return ANIM_IDLE;
    if (strcmp(name, "walk_fwd") == 0)   return ANIM_WALK_FWD;
    if (strcmp(name, "walk_back") == 0)  return ANIM_WALK_BACK;
    if (strcmp(name, "crouch") == 0)     return ANIM_CROUCH;
    if (strcmp(name, "jump") == 0)       return ANIM_JUMP;
    if (strcmp(name, "dash_fwd") == 0)   return ANIM_DASH_FWD;
    if (strcmp(name, "dash_back") == 0)  return ANIM_DASH_BACK;
    if (strcmp(name, "hitstun") == 0)    return ANIM_HITSTUN;
    if (strcmp(name, "blockstun") == 0)  return ANIM_BLOCKSTUN;
    if (strcmp(name, "knockdown") == 0)  return ANIM_KNOCKDOWN;
    /* Attack normals: "5L"=0, "5M"=1, "5H"=2, "2L"=3, etc. */
    if (strcmp(name, "5L") == 0)  return ANIM_ATTACK_BASE + 0;
    if (strcmp(name, "5M") == 0)  return ANIM_ATTACK_BASE + 1;
    if (strcmp(name, "5H") == 0)  return ANIM_ATTACK_BASE + 2;
    if (strcmp(name, "2L") == 0)  return ANIM_ATTACK_BASE + 3;
    if (strcmp(name, "2M") == 0)  return ANIM_ATTACK_BASE + 4;
    if (strcmp(name, "2H") == 0)  return ANIM_ATTACK_BASE + 5;
    if (strcmp(name, "jL") == 0)  return ANIM_ATTACK_BASE + 6;
    if (strcmp(name, "jM") == 0)  return ANIM_ATTACK_BASE + 7;
    if (strcmp(name, "jH") == 0)  return ANIM_ATTACK_BASE + 8;
    return -1;
}

void sprite_load(SpriteSet *set, const char *char_name) {
    memset(set, 0, sizeof(SpriteSet));
    set->loaded = 0;

    /* Build paths */
    char sheet_path[256];
    char manifest_path[256];
    snprintf(sheet_path, sizeof(sheet_path), "assets/%s/sheet.png", char_name);
    snprintf(manifest_path, sizeof(manifest_path), "assets/%s/manifest.txt", char_name);

    /* Check if sheet file exists before loading */
    if (!FileExists(sheet_path)) {
        TraceLog(LOG_WARNING, "SPRITE: Sheet not found: %s", sheet_path);
        return;
    }

    set->sheet = LoadTexture(sheet_path);
    if (set->sheet.id == 0) {
        TraceLog(LOG_WARNING, "SPRITE: Failed to load texture: %s", sheet_path);
        return;
    }

    /* Parse manifest */
    char *text = LoadFileText(manifest_path);
    if (!text) {
        TraceLog(LOG_WARNING, "SPRITE: Manifest not found: %s (using sheet without anims)", manifest_path);
        set->loaded = 1;
        return;
    }

    /* Parse line by line */
    char *line = text;
    while (*line) {
        /* Skip leading whitespace */
        while (*line == ' ' || *line == '\t') line++;

        /* Skip comments and empty lines */
        if (*line == '#' || *line == '\n' || *line == '\r' || *line == '\0') {
            while (*line && *line != '\n') line++;
            if (*line == '\n') line++;
            continue;
        }

        /* Parse: anim_name x y frame_w frame_h frame_count frame_duration looping */
        char name[64];
        int x, y, fw, fh, fc, fd, loop;
        int matched = sscanf(line, "%63s %d %d %d %d %d %d %d",
                             name, &x, &y, &fw, &fh, &fc, &fd, &loop);
        if (matched == 8) {
            int id = anim_name_to_id(name);
            if (id >= 0 && id < ANIM_COUNT) {
                set->anims[id].x = x;
                set->anims[id].y = y;
                set->anims[id].frame_width = fw;
                set->anims[id].frame_height = fh;
                set->anims[id].frame_count = fc;
                set->anims[id].frame_duration = fd;
                set->anims[id].looping = loop;
            }
        }

        /* Advance to next line */
        while (*line && *line != '\n') line++;
        if (*line == '\n') line++;
    }

    UnloadFileText(text);
    set->loaded = 1;
    TraceLog(LOG_INFO, "SPRITE: Loaded sprite set for '%s'", char_name);
}

void sprite_unload(SpriteSet *set) {
    if (set->loaded) {
        UnloadTexture(set->sheet);
        set->loaded = 0;
    }
}

void sprite_draw(const SpriteSet *set, int anim_id, int frame_index,
                 int x, int y, int facing, int dest_w, int dest_h) {
    if (!set->loaded) return;
    if (anim_id < 0 || anim_id >= ANIM_COUNT) return;

    const Animation *anim = &set->anims[anim_id];
    if (anim->frame_count == 0) return;  /* anim not defined — caller falls back */

    /* Clamp frame index */
    int frame = frame_index;
    if (frame >= anim->frame_count) frame = anim->frame_count - 1;
    if (frame < 0) frame = 0;

    /* Source rect: row of frames in sheet */
    float src_w = (float)anim->frame_width;
    float src_h = (float)anim->frame_height;
    float src_x = (float)(anim->x + frame * anim->frame_width);
    float src_y = (float)anim->y;

    /* SFA3 source sprites face right; flip for facing left */
    if (facing == -1) src_w = -src_w;

    Rectangle src = { src_x, src_y, src_w, src_h };

    /* Dest rect: anchor bottom-center to character position.
     * Character's (x, y) is top-left of their pushbox.
     * We want sprite bottom-center = pushbox bottom-center.
     * Pushbox bottom-center = (x + dest_w/2, y + dest_h).
     * Sprite dest top-left = (center_x - dest_w/2, bottom_y - dest_h).
     * Which simplifies to (x, y) — same as pushbox top-left. */
    Rectangle dst = { (float)x, (float)y, (float)dest_w, (float)dest_h };

    DrawTexturePro(set->sheet, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
}

#endif /* TESTING_HEADLESS */

void sprite_sync_anim(const SpriteSet *set, int current_anim,
                      int *anim_frame, int *anim_timer,
                      int *anim_frame_duration, int *anim_frame_count,
                      int *anim_looping) {
    if (!set || !set->loaded) return;
    if (current_anim < 0 || current_anim >= ANIM_COUNT) return;

    const Animation *a = &set->anims[current_anim];
    if (a->frame_count == 0) return;  /* anim not in manifest */

    /* Only re-sync when set_anim() reset frame_count to 0 (anim changed) */
    if (*anim_frame_count == 0) {
        *anim_frame_duration = a->frame_duration;
        *anim_frame_count = a->frame_count;
        *anim_looping = a->looping;
        /* Don't overwrite anim_frame — set_anim() already resets it to 0,
         * and callers may have set it to a specific value (e.g. 999 for
         * crouch recovery to skip to last frame). */
        *anim_timer = -1;
    }
}
