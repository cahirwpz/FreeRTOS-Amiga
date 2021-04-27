#include <sys/cdefs.h>
#include <copper.h>
#include <sprite.h>

static __bsschip SprDat_t _empty_spr[] = {SPREND()};

void SpriteUpdatePos(Sprite_t *spr, short x, short y) {
  spr->data[0] = SPRHDR(x, y, 0, spr->height);
  if (spr->attached)
    spr->attached[0] = SPRHDR(x, y, 1, spr->height);
}

CopIns_t *CopLoadSprite(CopList_t *list, int num, Sprite_t *spr) {
  return CopMove32(list, sprpt[num], spr ? spr->data : _empty_spr);
}
