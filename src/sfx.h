#ifndef SFX_H
#define SFX_H

/* Runtime-generated combat SFX (no external audio assets required). */
void sfx_init(void);
void sfx_shutdown(void);

/* Play impact SFX for a combat contact event.
 * damage: move damage used for light/medium/heavy tiering.
 * blocked: non-zero if the hit was blocked.
 * is_throw: non-zero for throw impact.
 * is_launcher: non-zero for launcher-style contacts.
 * is_wall_bounce: non-zero when contact is a wall-bounce style hit. */
void sfx_play_impact(int damage, int blocked, int is_throw, int is_launcher, int is_wall_bounce);

#endif /* SFX_H */
